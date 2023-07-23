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

    bool GameStarted()
    {
	    return m_gameStarted;
    }

    bool PlayerNumber()
    {
        return m_playerNumber;
    }

    void SendTick(int tickNum, CPlayerInput& playerInput);

private:
    volatile bool m_bStop; // Member variable to signal thread to break out of work loop

    
    SOCKET m_Socket;

    char m_playerNumber = 0;
    bool m_gameStarted = false;


    sockaddr_in m_serverAddress;
};
