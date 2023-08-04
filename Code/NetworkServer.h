#pragma once

#include "NetworkServer.h"

#include "ThreadRunner.h"

#include <WinSock2.h>

#include "Rollback/GameState.h"

class CNetUdpServerInterface
{
protected:
    ~CNetUdpServerInterface() = default;

public:
    virtual void Send(const char* buff, int len, sockaddr_in* to) = 0;
    virtual int Receive(char* buff, int len, sockaddr_in* si_other) = 0;
};

class CNetUdpServer : public CNetUdpServerInterface
{
public:
    virtual ~CNetUdpServer();
    CNetUdpServer();
    void Send(const char* buff, int len, sockaddr_in* to) override;
    int Receive(char* buff, int len, sockaddr_in* si_other) override;
private:
    SOCKET m_ListenSocket;
};

class CNetworkServer : public CThreadRunnableInterface
{
public:

    CNetworkServer(CNetUdpServerInterface* networkServerUdp)
        : m_networkServerUdp(networkServerUdp)
    {
	    for (int i = 0; i < NUM_PLAYERS; ++i)
	    {
            m_latestTickNumber[i] = -1;
            m_clientUpdateNumber[i] = -1;
            int* clientUpdateTickNumber = (*m_clientUpdatesTickNumbersBuffers[i].GetAt(0));
            for (int j = 0; j < NUM_PLAYERS - 1; ++j)
            {
                clientUpdateTickNumber[j] = -1;
            }
	    }
    }

    void DoWork() override;
    void Start() override;

private:
    CNetUdpServerInterface *m_networkServerUdp;


    char m_clientConnectionCounter = 0;

    int m_latestTickNumber[NUM_PLAYERS];
    int m_clientUpdateNumber[NUM_PLAYERS];
    RingBuffer<int[NUM_PLAYERS-1]> m_clientUpdatesTickNumbersBuffers[NUM_PLAYERS];
    RingBuffer<CPlayerInput[NUM_PLAYERS]> m_clientInputsBuffer;

    sockaddr_in m_clientSockets[NUM_PLAYERS];
};
