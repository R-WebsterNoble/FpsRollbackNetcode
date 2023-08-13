#include "NetworkServer.h"

#include <sysinfoapi.h>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <CrySystem/ISystem.h>

#include "NetworkClient.h"
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

void CNetworkServer::UpdateServerData(ClientToServerUpdateBytesUnion& clientToServerUpdate, int len, const char playerNumber)
{
	RingBuffer<CPlayerInput> &playerInputsBuffer = m_playerInputsBuffer[playerNumber];
	CPlayerInputsSynchronizer &playerInputsSynchronizerReceive = m_playerInputsSynchronizersReceive[playerNumber];
	const size_t synchronizerSize = len - (sizeof ClientToServerUpdateBytesUnion - sizeof PlayerInputsSynchronizerPacket);
	auto [tickNum, count] = playerInputsSynchronizerReceive.LoadPaket(clientToServerUpdate.ticks.synchronizer.buff, synchronizerSize, playerInputsBuffer);


	for (int i = 0, p = 0; i < NUM_PLAYERS; i++)
	{
		if(i == playerNumber)
			continue;

		CPlayerInputsSynchronizer(*inputsSynchronizersSend)[NUM_PLAYERS-1] = &m_playerInputsSynchronizersSend[p];
		for (int j = 0; j < count; ++j)
		{
			inputsSynchronizersSend[i]->Enqueue(tickNum + j, *playerInputsBuffer.GetAt(tickNum + j));
		}
		p++;
	}
}

bool CNetworkServer::BuildResponsePacket(const char playerNumber, ServerToClientUpdateBytesUnion* packet, size_t& bytesWritten)
{
	bytesWritten = 0;
	for (int i = 0; i < NUM_PLAYERS-1; ++i)
	{
		size_t size;
		if (!m_playerInputsSynchronizersSend[playerNumber][i].GetPaket(packet->ticks.synchronizersBytes + bytesWritten, size))
			return false;

		packet->ticks.synchronizerPacketSizes[i] = size;
		bytesWritten += size;
	}
	return true;
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

		const char playerNumber = clientToServerUpdate.ticks.playerNum;

		UpdateServerData(clientToServerUpdate, len, playerNumber);


		ServerToClientUpdate serverToClientUpdate;
		ServerToClientUpdateBytesUnion* packet = reinterpret_cast<ServerToClientUpdateBytesUnion*>(&serverToClientUpdate);
		size_t bytesWritten;
		if(!BuildResponsePacket(playerNumber, packet, bytesWritten))
			return;

		ServerToClientUpdateDebugViewer debug = ServerToClientUpdateDebugViewer(serverToClientUpdate);

		std::stringstream sb2;
		sb2 << debug;
		QueryPerformanceCounter(&t);
		gEnv->pLog->LogToFile("NetworkServer: %llu Sending: %s int expectedLen = %zu;", t.QuadPart, sb2.str().c_str(), bytesWritten);
		m_networkServerUdp->Send(packet->buff, len, &si_other);

	}

}

void CNetworkServer::Start()
{
}
