#pragma once

#include "NetworkClient.h"

#include <WinSock2.h>
#include <CryThreading/IThreadManager.h>

#include "Rollback/GameState.h"


class CNetworkClient : public IThread
{
public:
    CNetworkClient()
        : m_bStop(false)
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

    void SendTick(int tickNum, CPlayerInput& playerInput);

private:
    volatile bool m_bStop; // Member variable to signal thread to break out of work loop

    
    SOCKET m_Socket;

    char m_playerNumber = 0;
    LARGE_INTEGER m_gameStartTime;
    int m_serverUpdateNumber;
    int m_serverAckedTick = -1;

    sockaddr_in m_serverAddress;

    int m_clientUpdatesReceivedTickNumbers[NUM_PLAYERS-1];
    RingBuffer<CPlayerInput[NUM_PLAYERS-1]> m_playerInputsReceived;
    RingBuffer<CPlayerInput> m_playerInputsToSend;
    // std::atomic<int> m_playerLatestTicks[NUM_PLAYERS];
};
