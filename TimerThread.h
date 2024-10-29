#ifndef __TIMER_THREAD_H_
#define __TIMER_THREAD_H_

#include <unistd.h>
#include "util/tc_monitor.h"
#include "util/tc_thread.h"
#include "globe.h"

using namespace tars;
using namespace std;

class ActivityServantImp;

/**
*
* 定时器,用于imp线程里插入自定义消息
*
**/
class TimerThread : public TC_Thread, public TC_ThreadLock
{
public:
    TimerThread();
    virtual ~TimerThread();

    //
    void initialize(tars::Servant *p);
    //
    void terminate();

protected:
    //
    virtual void run();

protected:
    //服务接口
    ActivityServantImp *m_servant;
    //定时器心跳间隔时间,秒
    int m_iInterval;
    //是否停止
    bool m_bShutdown;
};

#endif



