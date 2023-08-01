#include "ThreadRunner.h"

void CThreadRunner::ThreadEntry()
{
	m_runnableToRun->Start();

	while (!m_bStop)
	{
		m_runnableToRun->DoWork();
	}
}


void CThreadRunner::SignalStopWork()
{
	if (m_bStop)
		return;

	m_bStop = true;	
}