#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection.h"

template <class T>
class threadpool
{
public:
    threadpool(int actor_model, connection_pool *connPool, int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    bool append(T *request, int state); //请求队列插入任务请求,加入参数识别读写操作
    bool append_p(T *request);  //请求队列插入任务请求

private:
    static void *worker(void *arg); // 不断从工作队列中取出任务并执行
    void run();

private:
    int m_thread_number;         // 线程池中线程数量
    int m_max_requests;          // 请求队列中允许的最大请求数
    pthread_t *m_threads;        // 保存线程id的数组
    std::list<T *> m_workqueue;  // 请求队列
    locker m_queuelocker;        // 请求队列的互斥锁
    sem m_queuestat;             // 是否有任务需要处理
    connection_pool *m_connPool; // 数据库连接池
    int m_actor_model;           // 模型切换
};
#endif