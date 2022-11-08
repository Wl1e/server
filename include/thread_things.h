#ifndef _THREAD_THINGS_H_
#define _THREAD_THINGS_H_

#include <pthread.h>
#include<semaphore.h>

class locks
{
    pthread_mutex_t m_lock;

public:
    locks();
    ~locks();

    bool lock();
    bool unlock();
};

class sems
{
    sem_t m_sem;

public:
    sems(int = 0);
    ~sems();

    bool post();
    bool wait();
    bool time_wait(const timespec*);
};

#endif