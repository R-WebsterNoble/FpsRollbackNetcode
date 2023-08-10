#include "NetworkServer.h"

#include <sysinfoapi.h>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <CrySystem/ISystem.h>

#include "Rollback/GameState.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BUFLEN 1216	//Max length of buffer
#define PORT 20100	//The port on which to listen for incoming data

CNetUdpServer::~CNetUdpServer()
{
}

CNetUdpServer::CNetUdpServer()
{
	sockaddr_in server;
	WSADATA wsa;

	//Initialise winsock
	CryLog("NetworkServer: Initialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		const auto e = WSAGetLastError();
		CryFatalError("NetworkServer: Failed. Error Code : %d", e);
		return;
		//exit(EXIT_FAILURE);
	}
	CryLog("NetworkServer: Initialised.");

	m_ListenSocket = socket(AF_INET, SOCK_DGRAM, 0);
	//Create a socket
	if (m_ListenSocket == INVALID_SOCKET)
	{
		const auto e = WSAGetLastError();
		CryFatalError("NetworkServer: Could not create socket : %d", e);
	}
	CryLog("NetworkServer: Socket created.");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT);

	//Bind
	if (bind(m_ListenSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		const auto e = WSAGetLastError();
		CryFatalError("NetworkServer: Bind failed with error code : %d", e);
		return;
		// exit(EXIT_FAILURE);
	}
	CryLog("NetworkServer: Bind done");
}

void CNetUdpServer::Send(const char* buff, int len, sockaddr_in* to)
{
	const int slen = sizeof(sockaddr_in);
	if (sendto(m_ListenSocket, buff, len, 0, reinterpret_cast<sockaddr*>(to), slen) == SOCKET_ERROR)
	{
		const auto e = WSAGetLastError();
		CryFatalError("RollbackNetClient: sendto() failed with error code : %d", e);
		return;
		// exit(EXIT_FAILURE);
	}
}

int CNetUdpServer::Receive(char* buff, int len, sockaddr_in* si_other)
{
	int slen = sizeof(sockaddr_in);

	int recv_len;
	if ((recv_len = recvfrom(m_ListenSocket, buff, len, 0, reinterpret_cast<sockaddr*>(si_other), &slen)) ==
		SOCKET_ERROR)
	{
		const auto e = WSAGetLastError();
		CryFatalError("NetworkServer: recvfrom() failed with error code : %d", e);
		return recv_len;
		// exit(EXIT_FAILURE);
	}
	return recv_len;
}

void CNetworkServer::DoWork()
{
	sockaddr_in si_other;

	ClientToServerUpdateBytesUnion clientToServerUpdate;
	char* recvBuffer = clientToServerUpdate.buff;


	//CryLog("NetworkServer: Waiting for data...");

	//clear the buffer by filling null, it might have previously received data
	memset(recvBuffer, '\0', BUFLEN);
	int len = m_networkServerUdp->Receive(recvBuffer, BUFLEN, &si_other);

	//try to receive some data, this is a blocking call


	if (recvBuffer[0] == 'c')
	{
		const char c[] = { 'p', m_clientConnectionCounter, '\0' };

		sockaddr_in* client = &m_clientSockets[m_clientConnectionCounter];

		*client = si_other;

		m_networkServerUdp->Send(c, sizeof(c), client);

		m_clientConnectionCounter++;
		if (m_clientConnectionCounter == NUM_PLAYERS)
		{
			StartBytesUnion start;
			start.start.packetTypeCode = 's';
			QueryPerformanceCounter(&start.start.gameStartTimestamp);
			//QueryPerformanceFrequency()
			constexpr long long frequency = 10000000;
			start.start.gameStartTimestamp.QuadPart += (frequency * 5); // start after 2 seconds to give clients time to settle.

			// send start game signal to clients
			for (auto clientSocket : m_clientSockets)
			{
				m_networkServerUdp->Send(start.buff, sizeof(start.buff), &clientSocket);
			}
		}
	}
	else if (recvBuffer[0] == 't')
	{
		std::stringstream sb;
		sb << clientToServerUpdate.ticks;
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		gEnv->pLog->LogToFile("NetworkServer: %llu Receive: %s int expectedLen = %i;", t.QuadPart, sb.str().c_str(), len);
		
		const char playerNum = clientToServerUpdate.ticks.playerNum;
		const int updateLastTickNum = clientToServerUpdate.ticks.tickNum;

		tiny::optional<int, INT_MIN> serverLatestTickReceived = m_latestTickNumbers[playerNum];

		if (!serverLatestTickReceived.has_value() || updateLastTickNum > serverLatestTickReceived)
		{
			const char updateTickCount = clientToServerUpdate.ticks.tickCount;
			int firstTickNumToUpdate;
			char numTicksToAdd;
			if (serverLatestTickReceived.has_value())
			{
				int serverLatestTickReceivedValue = serverLatestTickReceived.value();
				firstTickNumToUpdate = serverLatestTickReceivedValue + 1;
				numTicksToAdd = updateLastTickNum - serverLatestTickReceivedValue;
			}
			else
			{
				firstTickNumToUpdate = 0;
				numTicksToAdd = updateTickCount;
			}

			if (numTicksToAdd > updateTickCount)
				CryFatalError("CNetworkServer : Somehow trying to update more ticks than we have in this packet");

			if (numTicksToAdd > 0)
			{
				const int skip = updateTickCount - numTicksToAdd;

				for (int i = 0; i < numTicksToAdd; ++i)
				{
					(*m_clientInputsBuffer.GetAt(firstTickNumToUpdate + i))[playerNum] = clientToServerUpdate.ticks.playerInputs[skip + i];
				}

				m_latestTickNumbers[playerNum] = tiny::make_optional<int, INT_MIN>(updateLastTickNum);
			}
		}
		
		RingBuffer<int[NUM_PLAYERS - 1]>* playerUpdatesTickNumbersBuffer = &m_clientUpdatesTickNumbersBuffers[playerNum];
		tiny::optional<int(*)[NUM_PLAYERS - 1]> ackedClientTickNumbers;
		if (clientToServerUpdate.ticks.ackServerUpdateNumber.has_value())
		{
			int ackedServerUpdateNumber = clientToServerUpdate.ticks.ackServerUpdateNumber.value();
			ackedClientTickNumbers = tiny::optional<int(*)[NUM_PLAYERS - 1]>(playerUpdatesTickNumbersBuffer->GetAt(ackedServerUpdateNumber));
		}
		// else
		// 	ackedClientTickNumbers = tiny::make_optional<int(*)[NUM_PLAYERS - 1]>();


		int thisUpdateNumber;
		if(m_latestClientUpdateNumbersAcked[playerNum].has_value())
		{
			thisUpdateNumber = m_latestClientUpdateNumbersAcked[playerNum].value();
			m_latestClientUpdateNumbersAcked[playerNum] = tiny::make_optional<int, INT_MIN>(thisUpdateNumber + 1);
		}
		else
		{
			m_latestClientUpdateNumbersAcked[playerNum] = tiny::make_optional<int, INT_MIN>(0);
			thisUpdateNumber = 0;
		}
		int(*thisPlayerUpdatesTickNumbersBuffer)[NUM_PLAYERS - 1] = playerUpdatesTickNumbersBuffer->GetAt(thisUpdateNumber);

		ServerToClientUpdateBytesUnion serverToClientUpdate;
		serverToClientUpdate.ticks.packetTypeCode = 'r';
		serverToClientUpdate.ticks.ackClientTickNum = updateLastTickNum;
		serverToClientUpdate.ticks.updateNumber = thisUpdateNumber;
		int nInputs = 0;
		for (int i = 0, p = 0; i < NUM_PLAYERS; ++i)
		{
			if (i == playerNum)
				continue;

			if (m_latestTickNumbers[i].has_value())
			{
				const int firstTickToSend = ackedClientTickNumbers.has_value() ? ((*ackedClientTickNumbers.value())[p]) + 1 : 0;
				const int lastTickToSend = m_latestTickNumbers[i].value();
				const int count = lastTickToSend - firstTickToSend;

				serverToClientUpdate.ticks.playerInputsTickNums[p] = tiny::make_optional<int, INT_MIN>(lastTickToSend);
				serverToClientUpdate.ticks.playerInputsTickCounts[p] = count;

				for (int k = 0; k < count; ++k)
				{
					serverToClientUpdate.ticks.playerInputs[nInputs++] = (*m_clientInputsBuffer.GetAt(firstTickToSend + k))[i];
				}

				(*thisPlayerUpdatesTickNumbersBuffer)[p] = lastTickToSend; // m_clientUpdatesTickNumbersBuffers[playerNum][thisUpdateNumber][p] = lastTickToSend
			}
			else
			{
				serverToClientUpdate.ticks.playerInputsTickNums[p] = tiny::make_optional<int, INT_MIN>();
				serverToClientUpdate.ticks.playerInputsTickCounts[p] = 0;
			}
			p++;
		}

		std::stringstream sb2;
		sb2 << serverToClientUpdate.ticks;
		// CryLogAlways("NetworkClient: Sending: %s", sb2.str().c_str());
		const size_t len = sizeof(ServerToClientUpdate) - (sizeof(serverToClientUpdate.ticks.playerInputs) - sizeof(CPlayerInput) * nInputs);
		
		QueryPerformanceCounter(&t);
		gEnv->pLog->LogToFile("NetworkServer: %llu Sending: %s int expectedLen = %i;", t.QuadPart, sb2.str().c_str(), len);
		m_networkServerUdp->Send(serverToClientUpdate.buff, len, &si_other);

	}
	//CryLog("NetworkServer: Packet Data:  %s", recvBuffer);

}

void CNetworkServer::Start()
{
}
