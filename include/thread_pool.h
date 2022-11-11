#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <list>

#include "thread_things.h"

template<typename T>
class thread_pool
{
    // 池大小
    const int POOL_LEN;

public:
    thread_pool(int = 8, int = 1000);
    ~thread_pool();

private:
    // 线程池
    pthread_t* threads;

    // 请求队列
    std::list<T*> req_list;
    int req_size;

    // 线程锁
    locks list_lock;

    // 信号量
    sems list_sem;

    // 线程状态
    bool thread_status;

public:
    // 事物添加进线程池
    bool add_work(T*);

    // 线程工作函数
    static void* work(void*);
    void run();

private:

};

#endif