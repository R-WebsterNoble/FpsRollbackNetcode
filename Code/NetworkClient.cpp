#include "NetworkClient.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <CrySystem/ISystem.h>

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
	m_networkClientUdp->Receive(buf, BUFLEN);	

	if(buf[0] == 'p')
	{
		m_playerNumber = buf[1];
	}
	else if(buf[0] == 's')
	{
		const StartBytesUnion* serverUpdate = reinterpret_cast<StartBytesUnion*>(&buf);
		m_gameStartTime = serverUpdate->start.gameStartTimestamp;
	}
	else if (buf[0] == 'r')
	{
		const ServerToClientUpdateBytesUnion* serverUpdate = reinterpret_cast<ServerToClientUpdateBytesUnion*>(&buf);
		m_serverUpdateNumber = serverUpdate->ticks.updateNumber;
		m_serverAckedTick = serverUpdate->ticks.ackClientTickNum;


		for (int i = 0, p = 0, o = 0; i < NUM_PLAYERS; ++i)
		{
			if (i == m_playerNumber)
				continue;

			const int lastTickUpdated = m_clientUpdatesReceivedTickNumbers[p];
			const int playerInputsTickNum = serverUpdate->ticks.playerInputsTickNums[p];
			const int playerInputsTickCount = serverUpdate->ticks.playerInputsTickCounts[p];

			if (playerInputsTickCount < 1)
				continue;

			const int latestTickToUpdate = playerInputsTickNum + (playerInputsTickCount-1);
			const int count = latestTickToUpdate - lastTickUpdated;

			const int offset = playerInputsTickNum - (lastTickUpdated+1); // skip inputs that we have already received
			o += offset;
			for (int k = 1; k <= count; ++k)
			{
				const CPlayerInput playerInput = serverUpdate->ticks.playerInputs[o++];
				const int tickNum = lastTickUpdated + k;

				(*m_playerInputsReceived.GetAt(tickNum))[p] = playerInput;

				STickInput tickInput = STickInput();
				tickInput.tickNum = tickNum;
				tickInput.playerNum = i;
				tickInput.inputs = playerInput;
				m_newPlayerInputsQueue.try_enqueue(tickInput);				
			}

			m_clientUpdatesReceivedTickNumbers[p] = playerInputsTickNum + latestTickToUpdate;

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
	m_playerInputsToSend.Rotate();	
}

void CNetworkClient::SendTicks(const int tickNum)
{
	const char ticksToSend = static_cast<char>(tickNum - m_serverAckedTick);

	ClientToServerUpdateBytesUnion packet;
	packet.ticks.packetTypeCode = 't';
	packet.ticks.playerNum = m_playerNumber;
	packet.ticks.tickNum = tickNum;
	packet.ticks.tickCount = ticksToSend;
	packet.ticks.ackServerUpdateNumber = m_serverUpdateNumber;

	for (int i = 0; i < ticksToSend; ++i)
	{
		packet.ticks.playerInputs[i] = *m_playerInputsToSend.GetAt(m_serverAckedTick + 1 + i);
	}

	size_t len = sizeof(ClientToServerUpdateBytesUnion) - (sizeof(CPlayerInput) * (MAX_TICKS_TO_SEND - ticksToSend));
	m_networkClientUdp->Send(packet.buff, len);	
}
