#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_queue.h"

using namespace std;

class Log
{
public:
    //单例模式实例化类
    static Log *get_instance()
    {
        //类实例化,不使用new,保存在栈上,系统会自动回收内存;使用new保存在堆中,需要delete删除
        static Log instance;
        return &instance;
    }

    //调用私有函数async_write_log的接口
    static void *flush_log_thread(void *args)
    {
        Log::get_instance()->async_write_log();
    }

    //初始化可设置的参数有日志文件、日志缓冲区大小、最大行数以及最长日志阻塞队列
    bool init(const char *fliename, int close_log, int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);

    //标准化日志输出内容格式
    void write_log(int level, const char *format, ...);

    //强制刷新缓冲区,避免被覆盖
    void flush(void);

private:
    char dir_name[128]; // 路径名
    char log_name[128]; // log文件名
    int m_split_lines;  // 日志最大行数
    int m_log_buf_size; // 日志缓冲区大小
    long long m_count;  // 日志行数记录
    int m_today;        // 记录当前时间所属天
    FILE *m_fp;         // 打开log的文件指针
    char *m_buf;        //写入缓冲区
    block_queue<string> *m_log_queue; // string类型阻塞队列
    bool m_is_async;                  // 是否同步标志
    locker m_mutex;
    int m_close_log; // 关闭日志

private:
    Log();
    virtual ~Log();

    //异步写日志方法,从阻塞队列中取出日志,并写入相应的文件
    void *async_write_log()
    {
        string single_log;
        while (m_log_queue->pop(single_log))
        {
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }
};

//定义四个级别日志内容标志化格式
#define LOG_DEBUG(format, ...)                                    \
    if (0 == m_close_log)                                         \
    {                                                             \
        Log::get_instance()->write_log(0, format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                             \
    }

#define LOG_INFO(format, ...)                                     \
    if (0 == m_close_log)                                         \
    {                                                             \
        Log::get_instance()->write_log(1, format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                             \
    }

#define LOG_WARN(format, ...)                                      \
    if (0 == m_close_log)                                          \
    {                                                              \
        Log::get_instance()->write_log(2, format, ##__VA_ARGS___); \
        Log::get_instance()->flush();                              \
    }

#define LOG_ERROR(format, ...)                                    \
    if (0 == m_close_log)                                         \
    {                                                             \
        Log::get_instance()->write_log(3, format, ##__VA_ARGS__); \
        Log::get_instance()->flush();                             \
    }

#endif