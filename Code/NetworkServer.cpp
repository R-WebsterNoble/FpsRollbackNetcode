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

void CNetUdpServer::Send(char* buff, const int len, sockaddr_in* to)
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
	for (int i = 0; i < NUM_PLAYERS; ++i)
	{
		m_playerInputsSynchronizers[i].UpdateFromPacket(clientToServerUpdate->player_synchronizers()->Get(i));
	}
	// size_t offset = 0;
	// char* buff = clientToServerUpdate.ticks.synchronizers[0].buff;
	//
	// for (int i = 0; i < NUM_PLAYERS; ++i)
	// {
	// 	
	// }
	//
	//
	// CPlayerInputsSynchronizer &playerInputsSynchronizer = m_playerInputsSynchronizers[playerNumber];
	// const size_t synchronizerSize = len - (sizeof ClientToServerUpdateBytesUnion - sizeof CPlayerInputsSynchronizerPacket);
	// playerInputsSynchronizer.UpdateFromPacket(clientToServerUpdate.ticks.synchronizers[playerNumber].buff, synchronizerSize);
}

bool CNetworkServer::BuildResponsePacket(flatbuffers::FlatBufferBuilder& builder, const FlatBuffPacket::ClientToServerUpdate* update, OUT flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate>& serverToClientUpdate)
{
	flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];
	for (int i = 0; i < NUM_PLAYERS; ++i)
	{
		flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> p1;
		OptInt lastTickAcked(update->player_synchronizers()->Get(i)->tick_num()->i());
		if (!m_playerInputsSynchronizers[0].GetPaket(builder, s[i], &lastTickAcked))
			return false;
	}
	const flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>>	playerSynchronizers = builder.CreateVector(s, NUM_PLAYERS);
	serverToClientUpdate = FlatBuffPacket::CreateClientToServerUpdate(builder, playerSynchronizers);

	return true;
}

void CNetworkServer::DoWork()
{
	sockaddr_in si_other;

	// ClientToServerUpdateBytesUnion clientToServerUpdate;
	char recvBuffer[BUFFER_SIZE];


	//CryLog("NetworkServer: Waiting for data...");

	//clear the buffer by filling null, it might have previously received data
	memset(recvBuffer, '\0', BUFFER_SIZE);
	int len = m_networkServerUdp->Receive(recvBuffer, BUFFER_SIZE, &si_other);

	//try to receive some data, this is a blocking call


	if (recvBuffer[0] == 'c')
	{
		char c[] = { 'p', m_clientConnectionCounter, '\0' };

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
		uint8_t* buffer = reinterpret_cast<uint8_t*>(recvBuffer);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);

			const auto debugOut = flatbuffers::FlatBufferToString(buffer, FlatBuffPacket::ClientToServerUpdateTypeTable());

			LARGE_INTEGER t;
			QueryPerformanceCounter(&t);
			gEnv->pLog->LogToFile("NetworkServer: %llu Receive: %s; int expectedLen = %i;", t.QuadPart, debugOut.c_str(), len);

			// std::stringstream sb;
			// sb << clientToServerUpdate.ticks;
			// LARGE_INTEGER t;
			// QueryPerformanceCounter(&t);
			// gEnv->pLog->LogToFile("NetworkServer: %llu Receive: %s int expectedLen = %i;", t.QuadPart, sb.str().c_str(), len);

			// const char playerNumber = clientToServerUpdate.ticks.playerNum;

			UpdateServerData(update);


			flatbuffers::FlatBufferBuilder builder;

			flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> serverToClientUpdate;

			BuildResponsePacket(builder, update, serverToClientUpdate);

			builder.Finish(serverToClientUpdate);
			uint8_t* bufferPointer = builder.GetBufferPointer();

			const flatbuffers::FlatBufferBuilder::SizeT len2 = builder.GetSize();
			if(len2 > BUFFER_SIZE)
				CryFatalError("NetworkServer: Attempted to send %i bytes when BUFFER_SIZE = %i", len2, BUFFER_SIZE);
			m_networkServerUdp->Send(reinterpret_cast<char*>(bufferPointer), len2, &si_other);

			const auto debugOut2 = flatbuffers::FlatBufferToString(bufferPointer, FlatBuffPacket::ClientToServerUpdateTypeTable());
			
			QueryPerformanceCounter(&t);
			gEnv->pLog->LogToFile("NetworkServer: %llu Sending: %s; int expectedLen = %zu;", t.QuadPart, debugOut2.c_str(), len2);


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
