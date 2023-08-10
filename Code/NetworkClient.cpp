#include "NetworkClient.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <CrySystem/ISystem.h>

#include <sstream>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

// #define BUFLEN 512	//Max length of buffer
#define PORT 20100	//The port on which to listen for incoming data

// class CNetUdpClient : public CNetUdpClientInterface
// {
// 	void Send(char* buff, int len) override;
// 	void Receive(char* buff, int len) override;
// };


CNetUdpClient::~CNetUdpClient()
{
	closesocket(m_Socket);

	shutdown(m_Socket, SD_BOTH);

	WSACleanup();
}

CNetUdpClient::CNetUdpClient()
{
	WSADATA wsa;

	//Initialise winsock
	CryLog("RollbackNetClient: Initialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		const auto e = WSAGetLastError();
		CryFatalError("RollbackNetClient: Failed. Error Code : %d", e);
		return;
		//exit(EXIT_FAILURE);
	}
	CryLog("RollbackNetClient: Initialised.");

	//Prepare the m_serverAddress structure
	m_serverAddress.sin_family = AF_INET;
	m_serverAddress.sin_port = htons(PORT);

	InetPton(AF_INET, "127.0.0.1", &m_serverAddress.sin_addr.S_un.S_addr);

	m_Socket = socket(AF_INET, SOCK_DGRAM, 0);
	//Create a socket
	if (m_Socket == INVALID_SOCKET)
	{
		const auto e = WSAGetLastError();
		CryFatalError("RollbackNetServer: Could not create server socket : %d", e);
	}
	CryLog("RollbackNetServer: Server socket created.");

}

void CNetUdpClient::Send(const char* buff, int len)
{
	if (sendto(m_Socket, buff, len, 0, reinterpret_cast<sockaddr*>(&m_serverAddress), sizeof m_serverAddress) == SOCKET_ERROR)
	{
		const auto e = WSAGetLastError();
		CryFatalError("RollbackNetClient: sendto() failed with error code : %d", e);
		return;
		// exit(EXIT_FAILURE);
	}
}

int CNetUdpClient::Receive(char* buff, int len)
{
	sockaddr_in si_other;
	int recv_len;
	int slen = sizeof(si_other);

	if ((recv_len = recvfrom(m_Socket, buff, len, 0, reinterpret_cast<sockaddr*>(&si_other), &slen)) ==
		SOCKET_ERROR)
	{
		auto e = WSAGetLastError();
		CryFatalError("NetworkClient: recvfrom() failed with error code : %d", e);
		return recv_len;
		// exit(EXIT_FAILURE);
	}
	return recv_len;
}


void CNetworkClient::Start()
{
	const char c[] = { 'c', '\0' };

	m_networkClientUdp->Send(c, sizeof(c));
}

void CNetworkClient::DoWork()
{	
	char buf[sizeof(ServerToClientUpdate)];	

	// CryLog("NetworkClient: Waiting for data...");

	//clear the buffer by filling null, it might have previously received data
	memset(buf, '\0', BUFLEN);

	//try to receive some data, this is a blocking call
	int len = m_networkClientUdp->Receive(buf, BUFLEN);

	if(buf[0] == 'p')
	{
		m_playerNumber = buf[1];
		char buffer[24];		
		sprintf(buffer, "RollbackClient%iLog.log", m_playerNumber);
		gEnv->pLog->SetFileName(buffer);
	}
	else if(buf[0] == 's')
	{
		const StartBytesUnion* serverUpdate = reinterpret_cast<StartBytesUnion*>(&buf);
		m_gameStartTime = serverUpdate->start.gameStartTimestamp;
	}
	else if (buf[0] == 'r')
	{
		const ServerToClientUpdateBytesUnion* serverUpdate = reinterpret_cast<ServerToClientUpdateBytesUnion*>(&buf);

		std::stringstream sb;	
		sb << serverUpdate->ticks;
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		gEnv->pLog->LogToFile("NetworkClient: %llu Receive: %s int expectedLen = %i;", t.QuadPart, sb.str().c_str(), len);
		

		m_serverUpdateNumber = tiny::make_optional<int, INT_MIN>(serverUpdate->ticks.updateNumber);
		m_serverAckedTick = tiny::make_optional<int, INT_MIN>(serverUpdate->ticks.ackClientTickNum);


		for (int i = 0, p = 0, o = 0; i < NUM_PLAYERS; ++i)
		{
			if (i == m_playerNumber)
				continue;
						
			tiny::optional<int, INT_MIN> optPlayerInputsTickNum = serverUpdate->ticks.playerInputsTickNums[p];
			if (optPlayerInputsTickNum.has_value())
			{
				const int firstTickToUpdate = m_clientUpdatesReceivedTickNumbers[p].has_value() ? m_clientUpdatesReceivedTickNumbers[p].value() + 1 : 0;
				const int lastTickToUpdate = optPlayerInputsTickNum.value();

				const int numTicksToAdd = lastTickToUpdate - firstTickToUpdate;
				if (numTicksToAdd > 0)
				{
					const int playerInputsTickCount = serverUpdate->ticks.playerInputsTickCounts[p];

					if (numTicksToAdd > playerInputsTickCount)
						CryFatalError("CNetworkClient : Somehow trying to update more ticks than we have in this packet");

					const int skip = playerInputsTickCount - numTicksToAdd; // skip inputs that we have already received
					const int count = numTicksToAdd - skip;

					o += skip;
					for (int k = 0; k < count; ++k)
					{
						const CPlayerInput playerInput = serverUpdate->ticks.playerInputs[o++];
						const int tickNum = firstTickToUpdate + k;

						(*m_playerInputsReceived.GetAt(tickNum))[p] = playerInput;

						STickInput tickInput = STickInput();
						tickInput.tickNum = tickNum;
						tickInput.playerNum = i;
						tickInput.inputs = playerInput;
						m_newPlayerInputsQueue.try_enqueue(tickInput);
					}

					m_clientUpdatesReceivedTickNumbers[p] = tiny::make_optional<int, INT_MIN>(lastTickToUpdate);
				}
			}

			p++;
		}	
		
	}

	//print details of the client/peer and the data received
	// CryLog("NetworkClient: Received packet from %s:%d\n", si_other.sin_addr, ntohs(si_other.sin_port));
	//CryLog("NetworkClient: Packet Data: %s", buf);
	
}

void CNetworkClient::EnqueueTick(const int tickNum, const CPlayerInput& playerInput)
{
	*m_playerInputsToSend.GetAt(tickNum) = playerInput;
}

void CNetworkClient::SendTicks(const int tickNum)
{
	int lastTickNumAckedByServer = m_serverAckedTick.value_or(0);
	const char ticksToSend = static_cast<char>(tickNum - lastTickNumAckedByServer);

	ClientToServerUpdateBytesUnion packet;
	packet.ticks.packetTypeCode = 't';
	packet.ticks.playerNum = m_playerNumber;
	packet.ticks.tickNum = tickNum;
	packet.ticks.tickCount = ticksToSend;
	packet.ticks.ackServerUpdateNumber = m_serverUpdateNumber;

	const int nextTickNumToSendToServer = lastTickNumAckedByServer + 1;
	for (int i = 0; i < ticksToSend; ++i)
	{
		packet.ticks.playerInputs[i] = *m_playerInputsToSend.GetAt(nextTickNumToSendToServer + i);
	}

	size_t len = sizeof(ClientToServerUpdateBytesUnion) - (sizeof(CPlayerInput) * (MAX_TICKS_TO_SEND - ticksToSend));

	std::stringstream sb;
	sb << packet.ticks;
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	gEnv->pLog->LogToFile("NetworkClient: %llu Sending: %s int expectedLen = %i;", t.QuadPart, sb.str().c_str(), len);
	m_networkClientUdp->Send(packet.buff, len);	
}
