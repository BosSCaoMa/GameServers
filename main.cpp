#include <iostream>
#include <thread>
#include "LogM.h"
#include "DBConnPool.h"
#include "RecvProc.h"
#include "EventLoop.h"
#include <sodium.h>

using namespace std;

EventLoop* g_eventLoop = nullptr;

int main()
{
    if (sodium_init() < 0) {
        LOG_ERROR("libsodium initialization failed");
        return 1;
    }
    
    // 设置日志级别
    LogM::getInstance().setLevel(DEBUG);
    
    // 数据库连接是个较慢的操作，建议程序启动时就初始化连接池
    string pwd;
    cout<<"请输入数据库密码:"<<endl;
    cin>>pwd;
    
    DBConnInfo userDbInfo{"tcp://127.0.0.1:3306", "root", pwd, "userdb"};
    auto& userPool = GetUserDBPool(userDbInfo);
    
    DBConnInfo gameDbInfo{"tcp://127.0.0.1:3306", "root", pwd, "gamedb"};
    auto& gamePool = GetGameDBPool(gameDbInfo);

    EventLoop eventLoop;
    g_eventLoop = &eventLoop;
    std::thread eventThread([&eventLoop]() {
        eventLoop.loop();
    });

    // 启动登录服务器
    ProcLoginReq(9000);
    
    eventThread.join();
    return 0;
}