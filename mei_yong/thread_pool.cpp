#include "thread_pool.h"

#include <iostream>

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