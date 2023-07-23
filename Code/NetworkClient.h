#pragma once

#include "NetworkClient.h"

#include <WinSock2.h>
#include <CryThreading/IThreadManager.h>

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



private:
    volatile bool m_bStop; // Member variable to signal thread to break out of work loop

    

    SOCKET m_Socket;
};