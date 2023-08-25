#pragma once

#include "NetworkServer.h"

#include "ThreadRunner.h"

#include <WinSock2.h>

#include "NetworkClient.h"
#include "Net/PlayerInputsSynchronizer.h"
#include "Rollback/GameState.h"

class CNetUdpServerInterface
{
protected:
    ~CNetUdpServerInterface() = default;

public:
    virtual void Send(char* buff, int len, sockaddr_in* to) = 0;
    virtual int Receive(char* buff, int len, sockaddr_in* si_other) = 0;
};

class CNetUdpServer : public CNetUdpServerInterface
{
public:
    virtual ~CNetUdpServer();
    CNetUdpServer();
    void Send(char* buff, int len, sockaddr_in* to) override;
    int Receive(char* buff, int len, sockaddr_in* si_other) override;
private:
    SOCKET m_ListenSocket;
};

struct ServerToClientUpdate
{
	char packetTypeCode = 'r';
	size_t synchronizerPacketSizes[NUM_PLAYERS - 1] = {0};
	char synchronizersBytes[sizeof CPlayerInputsSynchronizerPacket * (NUM_PLAYERS-1)] = {0};
};


union ServerToClientUpdateBytesUnion
{
	ServerToClientUpdate ticks;
	char buff[sizeof(ServerToClientUpdate)] = { 0 };
};

struct ServerToClientUpdateDebugViewer
{
	ServerToClientUpdateDebugViewer(const ServerToClientUpdate &u)
	{
		memcpy(buff, u.synchronizersBytes, sizeof u.synchronizersBytes);

		size_t offset = 0;
		for (int i = 0; i < NUM_PLAYERS - 1; ++i)
		{
			CPlayerInputsSynchronizerPacket* p = reinterpret_cast<CPlayerInputsSynchronizerPacket*>(buff + offset);
			s[i] = p;
			offset += synchronizerPacketSizes[i] = u.synchronizerPacketSizes[i];
		}
	}
	char packetTypeCode = 'r';
	size_t synchronizerPacketSizes[NUM_PLAYERS - 1] = { 0 };
	char buff[sizeof CPlayerInputsSynchronizerPacket * (NUM_PLAYERS - 1)] = { 0 };
	CPlayerInputsSynchronizerPacket* s[NUM_PLAYERS - 1] = {};
};


inline std::ostream& operator<<(std::ostream& out, const ServerToClientUpdateDebugViewer& rhs) {

	out << "ServerToClientUpdate{ "
		<< "/*packetTypeCode*/ '" << rhs.packetTypeCode << "', "

		<< "{ ";
	for (int i = 0; i < NUM_PLAYERS - 1; ++i)
	{
		out << *rhs.s[i];
		if (i < NUM_PLAYERS - 2)
			out << ", ";
		else
			out << " ";
	}
	return out;
}

inline std::ostream& operator<<(std::ostream& out, const ServerToClientUpdate &rhs) {
	return out << ServerToClientUpdateDebugViewer(rhs);
}


class CNetworkServer : public CThreadRunnableInterface
{
public:

    CNetworkServer(CNetUdpServerInterface* networkServerUdp)
        : m_networkServerUdp(networkServerUdp)
    {
	    // for (int i = 0; i < NUM_PLAYERS; ++i)
	    // {
     //        m_latestTickNumbers[i] = OptInt();
     //        m_latestClientUpdateNumbersAcked[i] = OptInt();
	    // }
    }

    void DoWork() override;
    void Start() override;

private:
    CNetUdpServerInterface *m_networkServerUdp;

    char m_clientConnectionCounter = 0;

    sockaddr_in m_clientSockets[NUM_PLAYERS] = {};

    CPlayerInputsSynchronizer m_playerInputsSynchronizers[NUM_PLAYERS];


    void UpdateServerData(const FlatBuffPacket::ClientToServerUpdate* clientToServerUpdate);
    bool BuildResponsePacket(flatbuffers::FlatBufferBuilder& builder, const FlatBuffPacket::ClientToServerUpdate* update, OUT flatbuffers::Offset<FlatBuffPacket::ClientToServerUpdate>& serverToClientUpdate, int
                             playerNum);
};
