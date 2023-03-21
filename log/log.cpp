#include<string.h>
#include<time.h>
#include<sys/time.h>
#include<stdarg.h>
#include"log.h"
#include<pthread.h>

using namespace std;

Log::Log(){
    m_count=0;
    m_is_async=false;
}

Log::~Log(){
    if(m_fp!=NULL){
        fclose(m_fp);
    }
}

bool Log::init(const char *file_name,int close_log,int log_buf_size,int split_lines,int max_queue_size){
    if(max_queue_size>=1){
        m_is_async=true;
        m_log_queue=new block_queue<string>(max_queue_size);
        pthread_t tid;
        pthread_create(&tid,NULL,flush_log_thread,NULL);
    }

    m_close_log=close_log;
    m_log_buf_size=log_buf_size;
    m_buf=new char[m_log_buf_size];
    memset(m_buf,'\0',m_log_buf_size);
    m_split_lines=split_lines;

    time_t t=time(NULL);//获取系统时间,单位为秒
    struct tm *sys_tm=localtime(&t);//使用 t 的值来填充 tm 结构
    struct tm my_tm=*sys_tm;

    const char *p=strrchr(file_name,'/');// 在参数 file_name 所指向的字符串中搜索最后一次出现字符 '/'（一个无符号字符）的位置
    char log_full_name[256]={0};

    if(p==NULL){
        snprintf(log_full_name,255,"%d_%02d_%02d_%s",my_tm.tm_year+1900,my_tm.tm_mon+1,my_tm.tm_mday,file_name);
    }
    else{
        strcpy(log_name,p+1);
        strncpy(dir_name,file_name,p-file_name+1);//函数将指定字节的字符从源字符复制到目标。该strncpy()函数接受三个参数：dest，src和count。最多复制 n 个字符
        snprintf(log_full_name,255,"%d_%02d_%02d_%s",my_tm.tm_year+1900,my_tm.tm_mon+1,my_tm.tm_mday,log_name);
    }

    m_today=my_tm.tm_mday;
    
    m_fp=fopen(log_full_name,"a");
    if(m_fp==NULL) return false;

    return true;
}

void Log::write_log(int level,const char *format,...){
    struct timeval now={0,0};
    gettimeofday(&now,NULL);
    time_t t=now.tv_sec;
    struct tm *sys_tm=localtime(&t);
    struct tm my_tm=*sys_tm;
    char s[16]={0};
    switch(level){
        case 0:
        strcpy(s,"[debug]:");
        break;
        case 1:
        strcpy(s,"[info]:");
        break;
        case 2:
        strcpy(s,"[warn]:");
        break;
        case 3:
        strcpy(s,"[error]:");
        break;
        default:
        strcpy(s,"[info]:");
        break;
    }

    m_mutex.lock();
    m_count++;

    if(m_today!=my_tm.tm_mday||m_count%m_split_lines==0){
        char new_log[256]={0};
        fflush(m_fp);
        fclose(m_fp);
        char tail[16]={0};

        snprintf(tail,16,"%d_%02d_%02d_",my_tm.tm_year+1990,my_tm.tm_mon+1,my_tm.tm_mday);

        if(m_today!=my_tm.tm_mday){
           snprintf(new_log,255,"%s%s%s",dir_name,tail,log_name);
           m_today=my_tm.tm_mday;
           m_count=0;
        }
        else{
            snprintf(new_log,255,"%s%s%s.%lld",dir_name,tail,log_name,m_count/m_split_lines);
        }
        m_fp=fopen(new_log,"a");
    }

    m_mutex.unlock();

    va_list valst;//可变参数指针
    va_start(valst,format);//指向第一个变参format的地址
    string log_str;

    m_mutex.lock();

    int n=snprintf(m_buf,48,"%d-%02d-%02d %02d:%02d:%02d.%06ld %s",my_tm.tm_year+1990,my_tm.tm_mon+1,my_tm.tm_mday,my_tm.tm_hour,my_tm.tm_min,my_tm.tm_sec,now.tv_usec,s);
    int m=vsnprintf(m_buf+n,m_log_buf_size-n-1,format,valst);//使用vsnprintf()用于向一个字符串缓冲区打印格式化字符串，且可以限定打印的格式化字符串的最大长度
    m_buf[m+n]='\n';
    m_buf[m+n+1]='\0';
    log_str=m_buf;

    m_mutex.unlock();

    if(m_is_async&&!m_log_queue->full()){
        m_log_queue->push(log_str);
    }
    else {
        m_mutex.lock();
        fputs(log_str.c_str(),m_fp);
        m_mutex.unlock();
    }

    va_end(valst);
}

void Log::flush(void){
    m_mutex.lock();
    fflush(m_fp);
    m_mutex.unlock();
}
