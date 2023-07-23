#pragma once

#include "NetworkServer.h"

#include <WinSock2.h>
#include <CryThreading/IThreadManager.h>

#include "Rollback/GameState.h"

class CNetworkServer : public IThread
{
public:
    CNetworkServer()
        : m_bStop(false)
    {
    }

    // Once the thread is up and running it will enter this method
    void ThreadEntry() override;

    // Signal the thread to stop working
    void SignalStopWork();



private:
    volatile bool m_bStop; // Member variable to signal thread to break out of work loop

    
    SOCKET m_ListenSocket;

    int m_latestTickNumber;
    int m_playerLatestTicks[NUM_PLAYERS];
    RingBuffer<CPlayerInput[NUM_PLAYERS]> m_playerInputsBuffer;
};
