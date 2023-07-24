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

    // Once the thread is up and running it will enter this method
    void ThreadEntry() override;

    // Signal the thread to stop working
    void SignalStopWork();



private:
    volatile bool m_bStop; // Member variable to signal thread to break out of work loop

    
    SOCKET m_ListenSocket;

    int m_latestTickNumber[NUM_PLAYERS];
    int m_clientUpdateNumber[NUM_PLAYERS];
    RingBuffer<int[NUM_PLAYERS-1]> m_clientUpdatesTickNumbersBuffers[NUM_PLAYERS];
    RingBuffer<CPlayerInput[NUM_PLAYERS]> m_clientInputsBuffer;
};
