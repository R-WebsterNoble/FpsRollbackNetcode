#pragma once

#include <CryThreading/IThreadManager.h>


class CThreadRunnableInterface
{
public:
	virtual ~CThreadRunnableInterface() = default;
	virtual void Start() = 0;
	virtual void DoWork() = 0;
};

class CThreadRunner : public IThread
{
public:
	CThreadRunner(CThreadRunnableInterface* networkClientUdp)
		: m_runnableToRun(networkClientUdp)
	{
	}

    // Once the thread is up and running it will enter this method
    void ThreadEntry() override;

    // Signal the thread to stop working
    void SignalStopWork();

private:
    volatile bool m_bStop = false; // Member variable to signal thread to break out of work loop
    CThreadRunnableInterface* m_runnableToRun;
};
