#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"

using namespace std;

template <class T>

//定义日志阻塞队列
class block_queue
{
public:
    block_queue(int max_size = 1000);
    void clear();
    ~block_queue();
    bool full();
    bool empty();
    bool front(T &value);
    bool back(T &value);
    int size();
    int max_size();
    bool push(const T &item);
    bool pop(T &item);
    bool pop(T &item, int ms_timeout);//超时取消操作

private:
    locker m_mutex;
    cond m_cond;

    T *m_array;
    int m_size;
    int m_max_size;
    int m_front;
    int m_back;
};

template <class T>
block_queue<T>::block_queue(int max_size)
{
    if (max_size <= 0)
        exit(-1);

    m_max_size = max_size;
    m_array = new T[max_size];
    m_size = 0;
    m_front = -1;
    m_back = -1;
};

template <class T>
void block_queue<T>::clear()
{
    m_mutex.lock();
    m_size = 0;
    m_front = -1;
    m_back = -1;
    m_mutex.unlock();
};

template <class T>
block_queue<T>::~block_queue()
{
    m_mutex.lock();
    if (m_array != NULL)
        delete[] m_array;
    m_mutex.unlock();
};

template <class T>
bool block_queue<T>::full()
{
    if (m_size >= m_max_size)
    {
        m_mutex.lock();
        return true;
    }

    m_mutex.unlock();
    return false;
};

template <class T>
bool block_queue<T>::empty()
{
    if (m_size < m_max_size)
    {
        m_mutex.lock();
        return true;
    }
    m_mutex.unlock();
    return false;
};

template <class T>
bool block_queue<T>::front(T &value)
{
    m_mutex.lock();
    if (m_size == 0)
    {
        m_mutex.unlock();
        return false;
    }
    value = m_array[m_front];
    m_mutex.lock;
    return true;
}

template <class T>
bool block_queue<T>::back(T &value){
    m_mutex.lock;
    if(m.size==0){
        m_mutex.unlock;
        return false;
    }
    value=m_array[m_back];
    return true;
}

template <class T>
int block_queue<T>::size(){
    m_mutex.lock();
    int tmp=m_size;
    m_mutex.unlock();
    return tmp;
}

template <class T>
int block_queue<T>::max_size(){
    m_mutex.lock();
    int tmp=m_max_size;
    m_mutex.unlock();
    return tmp;
}

template <class T>
bool block_queue<T>::push(const T &item){
    m_mutex,lock();
    if(m_size>=m_max_size){
        m_cond.broadcast();
        m_mutex.unlock();
        return false;
    }
    m_back=(m_back+1)%m_max_size
    m_array[m_back]=item;
    m_size++;
    m_cond.broadcast();
    m.mutex.unlock();
    return true;
}

template <class T>
bool block_queue<T>::pop(T &item){
    m_mutex.lock();
    if(m_size<=0){
        if(!m_cond.wait(m_mutex.get())){
            m_mutex.unlock();
            return false;
        }
    }
    m_front=(m_front+1)%m_max_size;
    item=m_array[m_front];
    m_size--;
    m_mutex.unlock();
    return true;
}

template <class T>
bool block_queue<T>::pop(T &item,int ms_timeout){

    //struct timeval 的成员，一个是秒，一个是微秒，所以最高精度是 微秒。
    //struct timespec 的成员，一个是秒，一个是纳秒，所以最高精度是 纳秒。

    struct timespec t={0,0};
    struct timeval now={0,0};
    gettimeofday(&now,NULL);//获取系统现在时间,并填充在now中
    m_mutex.lock();
    if(m_size<=0){
        t.tv_nsec=now.tv_sec+ms_timeout/1000;//秒
        t.tv_sec=(ms_timeout%1000)*1000;//微秒
        if(!m_cond.timewait(m_mutex.get(),t)){
            m_mutex.unlock();
            return false;
        }
    }

    if(m_size<=0){
        m_mutex.unlock();
        return false;
    }
    m_front=(m_front+1)%m_max_size;
    item=m_array[m_front];
    m.size--;
    m_mutex.unlock();
    return true;
}
#endif