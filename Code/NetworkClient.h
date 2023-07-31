#pragma once

#include "NetworkClient.h"

#include <WinSock2.h>
#include <CryThreading/IThreadManager.h>

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

class CNetworkClientInterface
{
	virtual void EnqueueTick(int tickNum, const CPlayerInput& playerInput) = 0;
	virtual void SendTicks(int tickNum) = 0;
};

class CNetworkClient : public CNetworkClientInterface, public IThread
{
public:
	CNetworkClient(CNetUdpClientInterface* networkClientUdp)
        : m_networkClientUdp(networkClientUdp)
    {
    }

    // Once the thread is up and running it will enter this method
    void ThreadEntry() override;

    // Signal the thread to stop working
    void SignalStopWork();

    LARGE_INTEGER StartTime() const
    {
        return m_gameStartTime;
    }

    bool PlayerNumber() const
    {
        return m_playerNumber;
    }

    // int LatestLocalPlayerTickAcknowledgedByServer()
    // {
    //     return m_playerLatestTicks[m_playerNumber];
    // }

    void EnqueueTick(int tickNum, const CPlayerInput& playerInput) override;
    void SendTicks(int tickNum) override;

private:
    volatile bool m_bStop = false; // Member variable to signal thread to break out of work loop

    CNetUdpClientInterface *m_networkClientUdp;
    
    char m_playerNumber = 0;
    LARGE_INTEGER m_gameStartTime;
    int m_serverUpdateNumber;
    int m_serverAckedTick = -1;


    int m_clientUpdatesReceivedTickNumbers[NUM_PLAYERS-1];
    RingBuffer<CPlayerInput[NUM_PLAYERS-1]> m_playerInputsReceived;
    RingBuffer<CPlayerInput> m_playerInputsToSend;
    // std::atomic<int> m_playerLatestTicks[NUM_PLAYERS];
};
