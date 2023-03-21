#include<mutex>
#include<string>

using namespace std;
class LOG{
public:
    void init(LOGLEVEL loglevel,LOGTARGET logTarget);
    void uninit();
    int createFile();
    static LOG* getInstance();
    LOGLEVEL getLoglevel();
    void setLoglevel();

    LOGTARGET getLogTarget();
    void setLogTarget();

    static int writeLog(
        LOGLEVEL loglevel,
        unsigned cha
    )
private:
    LOG();
    ~LOG();
    static LOG* Log;
    static mutex log_mutex;
    static string logBuffer;
    LOGLEVEL loglevel;
    LOGTAGET logTarget;
    static HANDLE mFileHandle;
};