
## 跨线程通信机制

问题：
其他线程（比如认证线程）想要将Client添加到EventLoop，但EventLoop在另一个线程中运行。

解决方案：
使用eventfd + pendingFunctors_

```cpp
void EventLoop::addClient(std::shared_ptr<Client> client) {
    if (isInLoopThread()) {
        // 如果就在EventLoop线程，直接执行
        poller_->addClient(client, EPOLLIN | EPOLLPRI);
    } else {
        // 如果在其他线程，投递任务
        runInLoop([this, client]() {
            poller_->addClient(client, EPOLLIN | EPOLLPRI);
        });
    }
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb(); // 直接执行
    } else {
        queueInLoop(std::move(cb)); // 加入队列 + 唤醒
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb)); // 加入任务队列
    }
    wakeup(); // 唤醒EventLoop处理
}
```

### 唤醒机制
eventfd的作用：
```cpp
// 构造时创建eventfd
wakeupFd_ = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
wakeupClient_ = std::make_shared<Client>(wakeupFd_);
poller_->addClient(wakeupClient_, EPOLLIN); // 注册到epoll
```
唤醒过程：
```cpp
// 其他线程调用wakeup()
void EventLoop::wakeup() {
    uint64_t one = 1;
    write(wakeupFd_, &one, sizeof(one)); // 写入数据到eventfd
}

// EventLoop线程收到事件
void EventLoop::handleRead() {
    uint64_t one;
    read(wakeupFd_, &one, sizeof(one)); // 读取数据（清除事件）
}
```