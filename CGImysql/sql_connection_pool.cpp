#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection.h"

using namespace std;

connection_pool::connection_pool()
{
    m_CurConn = 0;
    m_FreeConn = 0;
}

connection_pool::~connection_pool()
{
    DestroyPool();
}

connection_pool *connection_pool::GetInstance()
{
    static connection_pool connPool;
    return &connPool;
}

// 构造初始化,创建数据库连接队列
void connection_pool::init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log)
{
    m_url = url;
    m_User = User;
    m_PassWord = PassWord;
    m_Databasename = DataBaseName;
    m_Port = Port;
    m_close_log = close_log;

    for (int i = 0; i < MaxConn; ++i)
    {
        MYSQL *con = NULL;
        con = mysql_init(con);

        if (con == NULL)
        {
            LOG_ERROR("MYSQL ERROR");
            exit(1);
        }

        con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DataBaseName.c_str(), Port, NULL, 0); // 建立数据库连接

        if (con == NULL)
        {
            LOG_ERROR("MYSQL ERROR");
            exit(1);
        }

        connList.push_back(con); // 数据库连接队列
        ++m_FreeConn;            // 成功创建数据库连接的个数
    }

    reserve = sem(m_FreeConn);

    m_MaxConn = m_FreeConn;
}

// 从数据库连接队列中获取一个可用的数据库连接
MYSQL *connection_pool::GetConnection()
{
    MYSQL *con = NULL;
    if (0 == connList.size())
    {
        return NULL;
    }

    reserve.wait();
    lock.lock();

    con = connList.front();
    connList.pop_front();

    --m_FreeConn;
    ++m_CurConn;

    lock.unlock();

    return con;
}

// 释放当前连接
bool connection_pool::ReleaseConnection(MYSQL *con)
{

    if (NULL == con)
    {
        return NULL;
    }

    lock.lock();

    connList.push_back(con);
    ++m_FreeConn;
    --m_CurConn;

    lock.unlock();

    reserve.post();
    return true;
}

// 销毁数据库连接池
void connection_pool::DestroyPool()
{

    lock.lock();

    if (connList.size() > 0)
    {
        list<MYSQL *>::iterator it;
        for (it = connList.begin(); it != connList.end(); ++it)
        {
            MYSQL *con = *it;
            mysql_close(con);
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }

    lock.unlock();
}

// 获取当前空闲的连接数
int connection_pool::GetFreeConn()
{
    return this->m_FreeConn;
}

connnectionRAII::connnectionRAII(MYSQL **SQL, connection_pool *connPool)
{
    *SQL = connPool->GetConnection();

    conRAII = *SQL;
    poolRAII = connPool;
}

connnectionRAII::~connnectionRAII()
{
    poolRAII->ReleaseConnection(conRAII);
}