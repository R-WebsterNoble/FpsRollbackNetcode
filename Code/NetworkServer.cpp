#include "NetworkServer.h"

#include <sysinfoapi.h>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <CrySystem/ISystem.h>
#include <flatbuffers/minireflect.h>

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

void CNetUdpServer::Send(const char* buff, const int len, sockaddr_in* to)
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

int CNetUdpServer::Receive(char* buff, const int len, sockaddr_in* si_other)
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

void CNetworkServer::UpdateServerData(const FlatBuffPacket::ClientToServerUpdate* clientToServerUpdate)
{
	m_playerInputsSynchronizers[0].UpdateFromPacket(clientToServerUpdate->player_1_synchronizer());
	m_playerInputsSynchronizers[1].UpdateFromPacket(clientToServerUpdate->player_2_synchronizer());
	// size_t offset = 0;
	// const char* buff = clientToServerUpdate.ticks.synchronizers[0].buff;
	//
	// for (int i = 0; i < NUM_PLAYERS; ++i)
	// {
	// 	
	// }
	//
	//
	// CPlayerInputsSynchronizer &playerInputsSynchronizer = m_playerInputsSynchronizers[playerNumber];
	// const size_t synchronizerSize = len - (sizeof ClientToServerUpdateBytesUnion - sizeof PlayerInputsSynchronizerPacket);
	// playerInputsSynchronizer.UpdateFromPacket(clientToServerUpdate.ticks.synchronizers[playerNumber].buff, synchronizerSize);
}

bool CNetworkServer::BuildResponsePacket(const char playerNumber, ServerToClientUpdateBytesUnion* packet, size_t& bytesWritten)
{
	bytesWritten = 0;
	// for (int i = 0; i < NUM_PLAYERS; ++i)
	// {
	// 	size_t size;
	// 	if (!m_playerInputsSynchronizers[i].GetPaket(packet->ticks.synchronizersBytes + bytesWritten, size))
	// 		return false;
	//
	// 	packet->ticks.synchronizerPacketSizes[i] = size;
	// 	bytesWritten += size;
	// }
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
	else 
	{
		flatbuffers::Verifier verifier(reinterpret_cast<uint8_t*>(recvBuffer), len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(reinterpret_cast<uint8_t*>(recvBuffer));

			const auto debugOut = flatbuffers::FlatBufferToString(reinterpret_cast<uint8_t*>(recvBuffer), FlatBuffPacket::ClientToServerUpdateTypeTable());

			LARGE_INTEGER t;
			QueryPerformanceCounter(&t);
			gEnv->pLog->LogToFile("NetworkServer: %llu Receive: %s int expectedLen = %i;", t.QuadPart, debugOut.c_str(), len);

			// std::stringstream sb;
			// sb << clientToServerUpdate.ticks;
			// LARGE_INTEGER t;
			// QueryPerformanceCounter(&t);
			// gEnv->pLog->LogToFile("NetworkServer: %llu Receive: %s int expectedLen = %i;", t.QuadPart, sb.str().c_str(), len);

			const char playerNumber = clientToServerUpdate.ticks.playerNum;

			UpdateServerData(update);


			// ServerToClientUpdate serverToClientUpdate;
			// ServerToClientUpdateBytesUnion* packet = reinterpret_cast<ServerToClientUpdateBytesUnion*>(&serverToClientUpdate);
			// size_t bytesWritten;
			// if (!BuildResponsePacket(playerNumber, packet, bytesWritten))
			// 	return;
			//
			// ServerToClientUpdateDebugViewer debug = ServerToClientUpdateDebugViewer(serverToClientUpdate);

			// std::stringstream sb2;
			// sb2 << debug;
			// QueryPerformanceCounter(&t);
			// gEnv->pLog->LogToFile("NetworkServer: %llu Sending: %s int expectedLen = %zu;", t.QuadPart, sb2.str().c_str(), bytesWritten);
			// m_networkServerUdp->Send(packet->buff, len, &si_other);
		}

	}

}

void CNetworkServer::Start()
{
}
