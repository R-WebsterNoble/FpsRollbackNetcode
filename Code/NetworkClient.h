#pragma once

#include "NetworkClient.h"

#include "ThreadRunner.h"

#include <WinSock2.h>

#include "lib/readerwritercircularbuffer.h"
#include "Net/PlayerInputsSynchronizer.h"
#include "Rollback/GameState.h"

class CNetUdpClientInterface
{
protected:
    ~CNetUdpClientInterface() = default;

public:
    virtual void Send(const char* buff, int len) = 0;
    virtual int Receive(char* buff, int len) = 0;
};

class CNetUdpClient : public CNetUdpClientInterface
{
public:
	virtual ~CNetUdpClient();
	CNetUdpClient();
    void Send(const char* buff, int len) override;
    int Receive(char* buff, int len) override;
private:
    SOCKET m_Socket;
    sockaddr_in m_serverAddress;
};


// struct STickInput
// {
//     int tickNum;
//     int playerNum;
//     CPlayerInput inputs;
// };

class CNetworkClientInterface
{
	virtual void EnqueueTick(int tickNum, const CPlayerInput& playerInput) = 0;
	virtual void SendTicks(int tickNum) = 0;
};

struct ClientToServerUpdate
{
    char packetTypeCode = 't';
    char playerNum = 0;
    PlayerInputsSynchronizerPacketBytesUnion synchronizers[NUM_PLAYERS] = { PlayerInputsSynchronizerPacketBytesUnion{} };
};



inline std::ostream& operator<<(std::ostream& out, ClientToServerUpdate const& rhs) {
    //ClientToServerUpdate{ packetTypeCode = 'r', playerNum = 1, tickCount = 1, tickNum = 1, ackServerUpdateNumber = 1, playerInputs = {CPlayerInput{Vec2{0.1, 0.1}, EInputFlag::MoveForward}} };
    /*packetTypeCode*/
    out << "ClientToServerUpdate{ "
        << "/*packetTypeCode*/ '" << rhs.packetTypeCode << "', "
        << "/*playerNum*/ " << (int)rhs.playerNum << ", { ";
        for (int i = 0; i < NUM_PLAYERS - 1; ++i)
        {
            out << rhs.synchronizers[i].packet;
            if (i < NUM_PLAYERS - 2)
                out << ", ";
            else
                out << " ";
        }    
        out << "}}";
    // precise formatting depends on your use case
    return out;
}

union ClientToServerUpdateBytesUnion
{
    char buff[sizeof(ClientToServerUpdate)] = { 0 };
    ClientToServerUpdate ticks;
};

class CNetworkClient : public CNetworkClientInterface, public CThreadRunnableInterface
{
public:
	CNetworkClient(CNetUdpClientInterface* networkClientUdp)
        : m_networkClientUdp(networkClientUdp)
    {
        // for (int i = 0; i < NUM_PLAYERS; ++i)
        // {
        //     m_clientUpdatesReceivedTickNumbers[i] = -1;
        // }
    }   

    LARGE_INTEGER StartTime() const
    {
        return m_gameStartTime;
    }

    char LocalPlayerNumber() const
    {
        return m_playerNumber;
    }

    bool GetInputUpdates(STickInput& update)
	{
        return m_newPlayerInputsQueue.try_dequeue(update);
	}

    // int LatestLocalPlayerTickAcknowledgedByServer()
    // {
    //     return m_playerLatestTicks[m_playerNumber];
    // }

	void Start() override;
    void DoWork() override;

    void EnqueueTick(int tickNum, const CPlayerInput& playerInput) override;
    void SendTicks(int tickNum) override;

private:

    CNetUdpClientInterface *m_networkClientUdp;
    CPlayerInputsSynchronizer m_playerInputsSynchronizers[NUM_PLAYERS];
    
    char m_playerNumber = 0;
    LARGE_INTEGER m_gameStartTime;
    
    moodycamel::BlockingReaderWriterCircularBuffer<STickInput> m_newPlayerInputsQueue = moodycamel::BlockingReaderWriterCircularBuffer<STickInput>(64);
};
