#pragma once

#include "NetworkClient.h"

#include "ThreadRunner.h"

#include <WinSock2.h>

#include "lib/readerwritercircularbuffer.h"
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


struct STickInput
{
    int tickNum;
    int playerNum;
    CPlayerInput inputs;
};

class CNetworkClientInterface
{
	virtual void EnqueueTick(int tickNum, const CPlayerInput& playerInput) = 0;
	virtual void SendTicks(int tickNum) = 0;
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
    
    char m_playerNumber = 0;
    LARGE_INTEGER m_gameStartTime;
    tiny::optional<int, INT_MIN> m_serverUpdateNumber;// = tiny::make_optional<int, INT_MIN>();
    tiny::optional<int, INT_MIN> m_serverAckedTick;// = tiny::make_optional<int, INT_MIN>();


    tiny::optional<int, INT_MIN> m_clientUpdatesReceivedTickNumbers[NUM_PLAYERS - 1];// = { tiny::make_optional<int, INT_MIN>() };
    RingBuffer<CPlayerInput[NUM_PLAYERS-1]> m_playerInputsReceived;
    RingBuffer<CPlayerInput> m_playerInputsToSend;
    // std::atomic<int> m_playerLatestTicks[NUM_PLAYERS];
    moodycamel::BlockingReaderWriterCircularBuffer<STickInput> m_newPlayerInputsQueue = moodycamel::BlockingReaderWriterCircularBuffer<STickInput>(64);
};
