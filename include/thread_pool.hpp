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


template<typename T>
thread_pool<T>::thread_pool(int nums, int max_req)
    : POOL_LEN(nums), thread_status(false), threads(nullptr), req_size(max_req)
{

    if (POOL_LEN <= 4)
    {
        std::cerr << " 池长度设置有误 \n";
        return;
    }

    threads = new pthread_t[POOL_LEN];
    for (int i = 0; i != POOL_LEN; ++i)
    {
        if (pthread_create(threads + i, nullptr, work, this))
        {
            delete[] threads;
            std::cerr << " 线程创建失败 \n";
        }

        if (pthread_detach(threads[i]))
        {
            delete[] threads;
            std::cerr << " 线程创建失败 \n";
        }
    }
}

template<typename T>
thread_pool<T>::~thread_pool()
{
    delete[] threads;
    thread_status = true;
}

template<typename T>
bool thread_pool<T>::add_work(T* work)
{
    list_lock.lock();
    if (req_list.size() > req_size)
    {
        list_lock.unlock();
        return false;
    }
    req_list.push_back(work);
    list_sem.post();
    list_lock.unlock();

    return true;
}

template<typename T>
void* thread_pool<T>::work(void* work)
{
    thread_pool* pool = (thread_pool*)work;
    pool->run();

    return pool;
}

template<typename T>
void thread_pool<T>::run()
{
    while(!thread_status)
    {
        list_sem.wait();
        list_lock.lock();
        if(req_list.empty())
        {
            list_lock.unlock();
            continue;
        }

        T* req = req_list.front();
        req_list.pop_back();
        list_lock.unlock();

        if(req)
        {
            req->run();
        }
    }
}

#endif