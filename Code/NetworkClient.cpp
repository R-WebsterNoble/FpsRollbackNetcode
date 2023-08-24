#include "NetworkClient.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <CrySystem/ISystem.h>

#include <sstream>

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/minireflect.h"
#include "Net/ClientToServerUpdate_generated.h"
// #include "Net/ClientToServerUpdate_generated.h"

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
	else 
	{
		const uint8_t* buffer = reinterpret_cast<uint8_t*>(buf);
		flatbuffers::Verifier verifier(buffer, len, 10, (NUM_PLAYERS * MAX_TICKS_TO_TRANSMIT) + 10);
		if (FlatBuffPacket::VerifyClientToServerUpdateBuffer(verifier))
		{
			const FlatBuffPacket::ClientToServerUpdate* update = FlatBuffPacket::GetClientToServerUpdate(buffer);
			

			const auto debugOut = flatbuffers::FlatBufferToString(buffer, FlatBuffPacket::ClientToServerUpdateTypeTable());

			LARGE_INTEGER t;
			QueryPerformanceCounter(&t);
			gEnv->pLog->LogToFile(	"NetworkClient: %llu Receive: %s; int expectedLen = %i;", t.QuadPart, debugOut.c_str(), len);


			for (int i = 0; i < NUM_PLAYERS; ++i)
			{
				const FlatBuffPacket::PlayerInputsSynchronizer* sync = update->player_synchronizers()->Get(i);
				auto [tickNum, count, inputs] = CPlayerInputsSynchronizer::ParsePaket(sync, m_playerInputsSynchronizers[i].GetLastTickAcked());

				if (i == m_playerNumber && count != 0)
					CryFatalError("Error: Received inputs for current player!");

				m_newPlayerInputsQueue.wait_enqueue(STickInput{ i, tickNum, inputs });
				m_playerInputsSynchronizers[i].Ack(OptInt(sync->tick_num()->i()));
			}
		
		}
		
	}

	//print details of the client/peer and the data received
	// CryLog("NetworkClient: Received packet from %s:%d\n", si_other.sin_addr, ntohs(si_other.sin_port));
	//CryLog("NetworkClient: Packet Data: %s", buf);
	
}

void CNetworkClient::EnqueueTick(const int tickNum, const CPlayerInput& playerInput)
{
	m_playerInputsSynchronizers[m_playerNumber].Enqueue(tickNum, playerInput);
}

void CNetworkClient::SendTicks(const int tickNum)
{	

	flatbuffers::FlatBufferBuilder builder;

	flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer> s[NUM_PLAYERS];
	for (int i = 0; i < NUM_PLAYERS; ++i)
	{
		if (!m_playerInputsSynchronizers[i].GetPaket(builder, s[i]))
			return;
	}
	flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>>> offset = builder.CreateVector(s, NUM_PLAYERS);
	const flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate> clientToServerUpdate = FlatBuffPacket::CreateClientToServerUpdate(builder, m_playerNumber, offset);

	builder.Finish(clientToServerUpdate);
	uint8_t* bufferPointer = builder.GetBufferPointer();

	const flatbuffers::FlatBufferBuilder::SizeT len = builder.GetSize();
	m_networkClientUdp->Send(reinterpret_cast<char*>(bufferPointer), len);

	const auto debugOut = flatbuffers::FlatBufferToString(bufferPointer, FlatBuffPacket::ClientToServerUpdateTypeTable());	
	
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	gEnv->pLog->LogToFile("NetworkClient: %llu Sending: %s; int expectedLen = %u;", t.QuadPart, debugOut.c_str(), len);
}
