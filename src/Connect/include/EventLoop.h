#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <sys/epoll.h>

class EPollPoller;
class Client;

class EventLoop {
public:
    using Functor = std::function<void()>;
    using ClientList = std::vector<std::shared_ptr<Client>>;

    EventLoop();
    ~EventLoop();

    // 主循环
    void loop();
    void quit();

    // 线程安全的方式添加连接到EventLoop  
    void addClient(std::shared_ptr<Client> client);
    void removeClient(int fd);
    void updateClient(std::shared_ptr<Client> client); // 更新客户端监听的事件
    std::shared_ptr<Client> getClient(int fd); // 获取指定fd的Client

    // 便捷函数：发送数据到客户端
    void sendToClient(int fd, const std::string& data, 
                     std::function<void()> writeCompleteCallback = nullptr);
    
    // 便捷函数：发送数据后关闭连接
    void sendAndClose(int fd, const std::string& data);

    // 在EventLoop线程中执行函数
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);

    bool isInLoopThread() const { return threadId_ == std::this_thread::get_id(); }

private:
    void handleRead(); // 处理wakeup
    void doPendingFunctors();
    void wakeup();
    void handleClient(std::shared_ptr<Client> client); // 处理客户端事件

    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    
    const std::thread::id threadId_;
    std::unique_ptr<EPollPoller> poller_;
    
    int wakeupFd_;
    std::shared_ptr<Client> wakeupClient_;

    // 跨线程调用
    mutable std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;
    
    static const int kPollTimeMs = 10000; // 10秒超时
};

#endif // EVENTLOOP_H