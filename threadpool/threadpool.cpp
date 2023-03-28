#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection.h"
#include "threadpool.h"

template <class T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int thread_number, int max_requests)
    : m_actor_model(actor_model), m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL), m_connPool(connPool)
{   
    //线程池的线程数为0或请求队列最大长度为0,构造失败
    if (thread_number <= 0 || max_requests <= 0)
    {
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_number];

    // 线程ID数组创建失败
    if (!m_threads)
    {
        throw std::exception();
    }

    //创建线程,并设为守护进程
    for (int i = 0; i < thread_number; ++i)
    {   
        //创建线程,处理函数为worker,传递函数参数指针this
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            std::throw exception();
        }

        // 分离线程分离(创建守护进程),不用单独回收
        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            std::throw exception();
        }
    }
}

template <class T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}

template <class T>
bool threadpool<T>::append(T *request, int state)
{
    m_queuelocker.lock();

    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }

    request->m_state = state;
    m_workqueue.push_back(request);

    m_queuelocker.unlock();

    m_queuestat.post();
    return true;
}

template <class T>
bool threadpool<T>::append_p(T *request)
{
    m_queuelocker.lock();

    //请求队列满
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }

    //任务请求入队
    m_workqueue.push_back(request);

    m_queuelocker.unlock();

    //队列信号量+1
    m_queuestat.post();

    return true;
}

template <class T>
void *threadpool<T>::worker(void *arg)
{   
    //将参数强制转换为线程池类,调用成员方法
    threadpool *pool = (threadpoool *)arg;
    pool->run();
    return pool;
}

template <class T>
void threadpool<T>::run()
{
    while (true)
    {
        m_queuestat.wait();

        m_queuelocker.lock();

        //无处理请求
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }

        //取出处理任务
        T *request = m_workqueue.front();
        m_workqueue.pop_front();

        m_queuelocker.unlock();

        if (!request)
        {
            continue;
        }

        //reactor模型
        if (1 == m_actor_model)
        {   
            //使用优雅关闭连接
            if (0 == request->m_state;)
            {   
                //读操作
                if (request->read_once())
                {
                    request->improve = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool); 
                    request->process(); 
                }
                else
                {
                    request->improve = 1;
                    request->timer_flag = 1;
                }
            }
            //不使用优雅关闭关闭连接
            else
            {   
                //写操作
                if (request->write())
                {
                    request->improve = 1;
                }
                else
                {
                    request->improve = 1;
                    request->timer_flag=1;
                }
            }
        }
        //proactor模型
        else
        {
            connnectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}