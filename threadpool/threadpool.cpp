#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>
#include"../lock/locker.h"
#include"../CGImysql/sql_connection.h"
#include"threadpool.h"

template <class T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int thread_number, int max_requests)
    : m_actor_model(actor_model), m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL), m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
    {
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_number];

    //线程ID数组创建失败
    if(!m_threads){
        throw std::exception();
    }

    for(int i=0;i<thread_number;++i){
        if(pthread_create(m_threads+i,NULL,worker,this)!=0){
            delete [] m_threads;
            std::throw exception();
        }

        //线程分离失败(创建守护进程失败)
        if(pthread_detach(m_threads[i])){
            delete [] m_threads;
            std::throw exception();
        }
    }
}