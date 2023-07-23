#include "NetworkClient.h"

#include<winsock2.h>
#include <ws2tcpip.h>
#include <CrySystem/ISystem.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

// #define BUFLEN 512	//Max length of buffer
#define PORT 20100	//The port on which to listen for incoming data

void CNetworkClient::ThreadEntry()
{	
	sockaddr_in si_other;
	int slen, recv_len;
	char buf[sizeof(ServerToClientUpdate)];
	WSADATA wsa;

	slen = sizeof(si_other);

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


	const char c[] = { 'c', '\0' };


	if (sendto(m_Socket, c, sizeof(c), 0, reinterpret_cast<sockaddr*>(&m_serverAddress), sizeof m_serverAddress) == SOCKET_ERROR)
	{
		const auto e = WSAGetLastError();
		CryFatalError("RollbackNetClient: sendto() failed with error code : %d", e);
		return;
		// exit(EXIT_FAILURE);
	}


	int packetCounter = 0;
	//keep listening for data
	while (!m_bStop)
	{
		CryLog("NetworkClient: Waiting for data...");

		//clear the buffer by filling null, it might have previously received data
		memset(buf, '\0', BUFLEN);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(m_Socket, buf, BUFLEN, 0, reinterpret_cast<sockaddr*>(&si_other), &slen)) ==
			SOCKET_ERROR)
		{
			auto e = WSAGetLastError();
			CryFatalError("NetworkClient: recvfrom() failed with error code : %d", e);
			return;
			// exit(EXIT_FAILURE);
		}

		packetCounter++;

		if(buf[0] == 'p')
		{
			m_playerNumber = buf[1];
		}
		else if(buf[0] == 's')
		{
			m_gameStarted = true;
		}
		else if (buf[0] == 'r')
		{
			const ServerToClientUpdateBytesUnion* serverUpdate = reinterpret_cast<ServerToClientUpdateBytesUnion*>(&buf);
			m_oldestTickNum = serverUpdate->ticks.newOldestTickNum;

			m_playerLatestTicks[m_playerNumber] = serverUpdate->ticks.firstTickNum;

			CryLog("NetworkClient: serverUpdate firstTickNum %i, newOldestTickNum %i", serverUpdate->ticks.firstTickNum, serverUpdate->ticks.newOldestTickNum);
		}

		//print details of the client/peer and the data received
		// CryLog("NetworkClient: Received packet from %s:%d\n", si_other.sin_addr, ntohs(si_other.sin_port));
		CryLog("NetworkClient: Packet #%i Data:  %s", packetCounter, buf);
	}

	closesocket(m_Socket);

	WSACleanup();
}


void CNetworkClient::SignalStopWork()
{
	if (m_bStop)
		return;

	m_bStop = true;

	shutdown(m_Socket, SD_BOTH);

	CryLog("NetworkClient: Stopped");
}

void CNetworkClient::SendTick(int tickNum, CPlayerInput& playerInput)
{
	auto input = m_playerInputsToSend.GetAt(tickNum);
	input->mouseDelta = playerInput.mouseDelta;
	input->playerActions = playerInput.playerActions;
	m_playerInputsToSend.Rotate();

	int latestLocalPlayerTick = LatestLocalPlayerTickAcknowledgedByServer();

	if (latestLocalPlayerTick == 0)
		latestLocalPlayerTick = -1;

	int ticksToSend = tickNum - latestLocalPlayerTick;

	ClientToServerUpdateBytesUnion packet;
	packet.ticks.packetTypeCode = 't';
	packet.ticks.playerNum = m_playerNumber;
	packet.ticks.tickNum = tickNum;
	packet.ticks.tickCount = ticksToSend;
	packet.ticks.oldestTickNum = m_oldestTickNum;

	for (int i = 0; i < ticksToSend; ++i)
	{
		packet.ticks.playerInputs[i] = *m_playerInputsToSend.GetAt(latestLocalPlayerTick + 1 + i);
	}

	size_t len = sizeof(ClientToServerUpdateBytesUnion) - (sizeof(CPlayerInput) * (MAX_TICKS_TO_SEND - ticksToSend));
	if (sendto(m_Socket, packet.buff, len, 0, reinterpret_cast<sockaddr*>(&m_serverAddress), sizeof m_serverAddress) == SOCKET_ERROR)
	{
		const auto e = WSAGetLastError();
		CryFatalError("RollbackNetClient: sendto() failed with error code : %d", e);
		return;
		// exit(EXIT_FAILURE);
	}
	
}
