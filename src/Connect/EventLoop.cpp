#include "EventLoop.h"
#include "EPollPoller.h"
#include "Client.h"
#include "LogM.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include "GameRecvProc.h"
EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      threadId_(std::this_thread::get_id()),
      poller_(std::make_unique<EPollPoller>(this)),
      wakeupFd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))
{
    if (wakeupFd_ < 0) {
        LOG_ERROR("EventLoop::EventLoop eventfd error");
    } else {
        wakeupClient_ = std::make_shared<Client>(wakeupFd_);
        // 注册wakeup事件到epoll，用于跨线程唤醒
        poller_->addClient(wakeupClient_, EPOLLIN);
    }
}

EventLoop::~EventLoop()
{
    if (wakeupFd_ >= 0) {
        ::close(wakeupFd_);
    }
}

void EventLoop::loop()
{
    if (looping_) {
        LOG_ERROR("EventLoop is already looping");
        return;
    }
    
    if (!isInLoopThread()) {
        LOG_ERROR("EventLoop::loop() called from wrong thread");
        return;
    }
    
    looping_ = true;
    quit_ = false;
    
    LOG_DEBUG("EventLoop started looping");
    
    while (!quit_) {
        ClientList activeClients;
        poller_->poll(kPollTimeMs, &activeClients);
        
        // 处理活动的客户端
        for (auto& client : activeClients) {
            if (client->getFd() == wakeupFd_) {
                handleRead(); // 处理wakeup事件
            } else {
                handleClient(client); // 处理客户端事件
            }
        }
        
        doPendingFunctors(); // 处理跨线程任务
    }
    
    LOG_DEBUG("EventLoop stopped looping");
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread()) {
        wakeup(); // 如果在其他线程调用，需要唤醒EventLoop
    }
}

void EventLoop::addClient(std::shared_ptr<Client> client)
{
    if (isInLoopThread()) {
        // 在EventLoop线程中直接添加
        poller_->addClient(client, EPOLLIN | EPOLLPRI); // 监听读事件和优先级事件
    } else {
        // 在其他线程中，加入队列等待处理
        runInLoop([this, client]() {
            poller_->addClient(client, EPOLLIN | EPOLLPRI);
        });
    }
}

void EventLoop::removeClient(int fd)
{
    if (isInLoopThread()) {
        poller_->removeClient(fd);
    } else {
        runInLoop([this, fd]() {
            poller_->removeClient(fd);
        });
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread()) {
        cb(); // 在EventLoop线程中直接执行
    } else {
        queueInLoop(std::move(cb)); // 在其他线程中加入队列
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    
    // 如果不在EventLoop线程或者正在处理pending函数，需要唤醒
    if (!isInLoopThread()) {
        wakeup();
    }
}

/* 处理wakeup事件，读取eventfd以清除事件 */
void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n);
    }
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    
    for (const Functor& functor : functors) {
        functor();
    }
}

/*
    wakeupFd_是eventfd，wakeup函数向其写入数据以唤醒阻塞在epoll_wait的EventLoop线程
*/
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::wakeup() writes %ld bytes instead of 8", n);
    }
}

void EventLoop::handleClient(std::shared_ptr<Client> client)
{
    uint32_t revents = client->getRevents();
    int fd = client->getFd();
    
    // 错误事件 - 优先处理
    if (revents & (EPOLLERR | EPOLLHUP)) {
        LOG_ERROR("Client fd=%d error or hangup event", fd);
        removeClient(fd);
        return;
    }
    
    // 读事件（包括对端关闭）
    if (revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        char buffer[8192];
        ssize_t n = ::read(fd, buffer, sizeof(buffer));
        
        if (n > 0) {
            // 收到数据，处理游戏协议
            LOG_DEBUG("EventLoop received %ld bytes from fd=%d", n, fd);
            // TODO: 解析游戏协议，分发到相应的处理器
            // 例如：handleGameMessage(client, buffer, n);
            handleGameMessage(client, buffer, n);
        } else if (n == 0) {
            // 对端关闭连接
            LOG_DEBUG("Client fd=%d disconnected", fd);
            removeClient(fd);
        } else {
            // 读取错误（errno会被设置）
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR("EventLoop::handleClient read error for fd=%d, errno=%d", fd, errno);
                removeClient(fd);
            }
        }
    }
    
    // 注意：当前只监听EPOLLIN，不会收到EPOLLOUT事件
    // 如果将来需要处理写缓冲，可以在这里添加EPOLLOUT处理
}