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

/*
==================== 项目关键点备忘（接入 -> 会话 -> 事件） ====================

1. 几个入口：
   - main.cpp: 程序入口，初始化日志、数据库连接池、EventLoop，启动登录服务器监听。
   - Login/RecvProc.cpp: 处理新连接
   - Game/GameRecvProc.cpp: 处理游戏消息。

2. 重要函数
    发消息给客户端的函数 
        - send_json_response  登录线程里给 HTTP 客户端回一次响应：现在用 send_json_response 最省事。
        - EventLoop::sendToClient 已经交给 g_eventLoop 管理的连接：更推荐只用 sendToClient（避免混用阻塞直写与 EventLoop 写缓冲）


3. 写事件全流程
   目标：业务线程只“排队发送”，真正 write 在 EventLoop 线程里分次完成。

   (1) 业务线程/回调线程调用：
       `g_eventLoop->sendToClient(fd, data, cb)`

   (2) sendToClient 内部：
       - 如果当前就在 EventLoop 线程：直接执行 sendToClientInLoop
       - 如果不在 EventLoop 线程：`queueInLoop(...)` 投递任务 + `wakeup()` 唤醒 epoll

   (3) EventLoop 被唤醒：
       - `epoll_wait` 返回 wakeupFd_ 的 EPOLLIN
       - `handleRead()` 读取 eventfd 清除事件
       - `doPendingFunctors()` 执行刚刚投递的 sendToClientInLoop

   (4) sendToClientInLoop 做的事：
       - 找到该 fd 对应的 `Client`
       - `Client::appendToOutputBuffer(data)` 把数据追加到输出缓冲
       - `Client::enableWriting()` 打开 EPOLLOUT
       - `updateClient(client)` 调用 epoll_ctl(MOD) 更新监听事件

   (5) 内核可写时：
       - epoll 返回该 fd 的 EPOLLOUT
       - `EventLoop::handleClient` 的写分支执行：
           `n = write(fd, out.data(), out.size())`

   (6) 处理写结果：
       - n > 0：`clearOutputBuffer(n)`（处理部分写）
       - 仍有剩余：保持 EPOLLOUT，等待下一次可写继续写
       - 已写完：`disableWriting()` + `updateClient(client)` 取消 EPOLLOUT
                然后触发 `handleWriteComplete()`（若设置了写完成回调）

   (7) 关闭连接（可选）：
       - `sendAndClose(fd, data)` = `sendToClient(..., 写完后 removeClient(fd))`
*/