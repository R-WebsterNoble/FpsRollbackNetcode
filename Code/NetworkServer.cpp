#include "NetworkServer.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <CrySystem/ISystem.h>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BUFLEN 512	//Max length of buffer
#define PORT 20100	//The port on which to listen for incoming data

static constexpr int PLAYER_COUNT = 1;

void CNetworkServer::ThreadEntry()
{
	sockaddr_in server, si_other;
	int slen, recv_len;
	char buf[BUFLEN];
	WSADATA wsa;

	slen = sizeof(si_other);

	char clientConnectionCounter = 0;

	//Initialise winsock
	CryLog("NetworkServer: Initialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		auto e = WSAGetLastError();
		CryFatalError("NetworkServer: Failed. Error Code : %d", e);
		return;
		//exit(EXIT_FAILURE);
	}
	CryLog("NetworkServer: Initialised.");

	m_ListenSocket = socket(AF_INET, SOCK_DGRAM, 0);
	//Create a socket
	if (m_ListenSocket == INVALID_SOCKET)
	{
		auto e = WSAGetLastError();
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
		auto e = WSAGetLastError();
		CryFatalError("NetworkServer: Bind failed with error code : %d", e);
		return;
		// exit(EXIT_FAILURE);
	}
	CryLog("NetworkServer: Bind done");

	int packetCounter = 0;
	//keep listening for data
	while (!m_bStop)
	{
		CryLog("NetworkServer: Waiting for data...");

		//clear the buffer by filling null, it might have previously received data
		memset(buf, '\0', BUFLEN);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(m_ListenSocket, buf, BUFLEN, 0, reinterpret_cast<sockaddr*>(&si_other), &slen)) ==
			SOCKET_ERROR)
		{
			auto e = WSAGetLastError();
			CryFatalError("NetworkServer: recvfrom() failed with error code : %d", e);
			return;
			// exit(EXIT_FAILURE);
		}

		if(buf[0] == 'c')
		{

			const char c[] = { 'p', clientConnectionCounter ,'\0'};
			if (sendto(m_ListenSocket, c, sizeof(c), 0, reinterpret_cast<sockaddr*>(&si_other), slen) == SOCKET_ERROR)
			{
				const auto e = WSAGetLastError();
				CryFatalError("RollbackNetClient: sendto() failed with error code : %d", e);
				return;
				// exit(EXIT_FAILURE);
			}
			clientConnectionCounter++;
		}

		packetCounter++;

		//print details of the client/peer and the data received
		// CryLog("NetworkServer: Received packet from %s:%d\n", si_other.sin_addr, ntohs(si_other.sin_port));
		CryLog("NetworkServer: Packet #%i Data:  %s", packetCounter, buf);
	}

	closesocket(m_ListenSocket);

	WSACleanup();
}


void CNetworkServer::SignalStopWork()
{
	if (m_bStop)
		return;

	m_bStop = true;

	shutdown(m_ListenSocket, SD_BOTH);

	CryLog("NetworkServer: Stopped");
}
