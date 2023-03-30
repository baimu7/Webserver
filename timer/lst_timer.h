#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <error.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"

class util_timer;

// 连接资源
struct client_data
{
    sockaddr_in address; // 客服端socket地址
    int sockfd;          // socket文件描述符
    util_timer *timer;   // 定时器
};

// 定时器类
class util_timer
{
public:
    util_timer() : prev(NULL), next(NULL) {}

public:
    time_t expire; // 超时时间

    void (*cb_func)(client_data *); // 回调函数
    client_data *user_data;         // 连接资源
    util_timer *prev;               // 前向定时器
    util_timer *next;               // 后继定时器
};

// 定时器容器类
class sort_timer_lst
{
public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer *timer); // 添加定时器

    void adjust_timer(util_timer *timer); // 调整定时器,当任务发生变化时,调整定时器在链表中的位置
    void del_timer(util_timer *timer);    // 删除定时器
    void tick();

private:
    void add_timer(util_timer *timer, util_timer *lst_head); // 调整链表内的节点

    util_timer *head; // 头节点
    util_timer *tail; // 尾节点
};

//
class Utils
{
public:
    Utils();
    ~Utils();

    void init(int timeslot);

    void addfd(int epollfd, int fd, bool one, int TRIGMode);
    static void sig_handler(int sig);
    void addsig(int sig, void(handler)(int), bool restart = true);

    void timer_handler();
    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    sort_timer_lst m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};

#endif