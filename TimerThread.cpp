#include "TimerThread.h"
#include "ActivityServantImp.h"

TimerThread::TimerThread( )
{
    m_bShutdown = false;
    m_iInterval = 20;
    m_servant = NULL;
}

TimerThread::~TimerThread()
{
    terminate();
}

void TimerThread::initialize(tars::Servant *p)
{
    m_servant = (ActivityServantImp *)p;
}

void TimerThread::terminate()
{
    if (!isAlive())
        return;

    m_bShutdown = true;
    LOG_DEBUG << "TimerThread terminated." << endl;
    // TC_ThreadLock::Lock sync(*this);
    // notifyAll();
}

void TimerThread::run()
{
    LOG_DEBUG << "TimerThread start." << endl;

    while (!m_bShutdown)
    {
        __TRY__

        sleep(m_iInterval);
        {
            TC_ThreadLock::Lock sync(*this);
            if (m_servant)
            {
                m_servant->getHandle()->notifyFilter();
            }
        }

        __CATCH__
    }

    LOG_DEBUG << "TimerThread end." << endl;
}


