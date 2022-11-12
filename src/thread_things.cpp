#include "thread_things.h"

        /*  locks   */
locks::locks()
{
    pthread_mutex_init(&m_lock, nullptr);
}

locks::~locks()
{
    pthread_mutex_destroy(&m_lock);
}

bool locks::lock()
{
    return pthread_mutex_lock(&m_lock) == 0;
}

bool locks::unlock()
{
    return pthread_mutex_unlock(&m_lock) == 0;
}

        /*  sems   */
sems::sems(int i)
{
    sem_init(&m_sem, 0, i);
}

sems::~sems()
{
    sem_destroy(&m_sem);
}

bool sems::post()
{
    return sem_post(&m_sem) == 0;
}

bool sems::wait()
{
    return sem_wait(&m_sem) == 0;
}

bool sems::time_wait(const timespec* time)
{
    return sem_timedwait(&m_sem, time);
}