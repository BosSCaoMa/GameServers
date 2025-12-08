# 写事件处理功能完整实现

## 概述

本次实现为 GameServers 项目添加了完整的**写事件处理**功能，并在 `UserSessionManager::auditSessions()` 中展示了实际应用。

## 主要修改

### 1. Client 类增强 (`src/Connect/include/Client.h`)

#### 新增成员变量
- `uint32_t events_` - 当前监听的事件（EPOLLIN、EPOLLOUT等）
- `std::string outputBuffer_` - 写缓冲区
- `ReadCallback readCallback_` - 读事件回调
- `WriteCompleteCallback writeCompleteCallback_` - 写完成回调
- `ErrorCallback errorCallback_` - 错误回调

#### 新增方法
```cpp
// 写缓冲区操作
void appendToOutputBuffer(const std::string& data);
void appendToOutputBuffer(const char* data, size_t len);
bool hasDataToWrite() const;
void clearOutputBuffer(size_t len);

// 事件控制
void enableWriting();
void disableWriting();
bool isWriting() const;
void setEvents(uint32_t events);
uint32_t getEvents() const;

// 回调设置
void setReadCallback(const ReadCallback& cb);
void setWriteCompleteCallback(const WriteCompleteCallback& cb);
void setErrorCallback(const ErrorCallback& cb);

// 回调触发
void handleRead(const char* data, ssize_t len);
void handleWriteComplete();
void handleError();
```

### 2. EventLoop 类增强 (`src/Connect/include/EventLoop.h`)

#### 新增方法
```cpp
// 更新客户端监听的事件
void updateClient(std::shared_ptr<Client> client);

// 获取指定 fd 的 Client 对象
std::shared_ptr<Client> getClient(int fd);
```

#### 修改的方法
- `handleClient()` - 添加了完整的 EPOLLOUT 事件处理逻辑

### 3. EPollPoller 类增强 (`src/Connect/include/EPollPoller.h`)

#### 新增方法
```cpp
// 获取指定 fd 的 Client 对象
std::shared_ptr<Client> getClient(int fd) const;
```

注：`updateClient()` 方法已移为 public（之前是 private）

### 4. UserSessionManager 实现 (`src/Login/UserSessionCB.cpp`)

#### 完善的功能
- `auditSessions()` - 使用写事件向过期会话发送通知
- `buildSessionExpiredResponse()` - 构造会话过期的 HTTP 响应

## 工作原理

### 写事件处理流程

```
┌─────────────────────────────────────────────────────────────┐
│ 1. 用户代码准备要发送的数据                                │
│    client->appendToOutputBuffer(data);                      │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│ 2. 启用写事件监听                                           │
│    client->enableWriting();                                 │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│ 3. (可选) 设置写完成回调                                    │
│    client->setWriteCompleteCallback([](Client* c) {...});  │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│ 4. 通知 EventLoop 更新事件监听                              │
│    eventLoop->updateClient(client);                         │
│    → 调用 epoll_ctl(EPOLL_CTL_MOD, fd, EPOLLOUT)          │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│ 5. EventLoop 等待 EPOLLOUT 事件                             │
│    epoll_wait() 返回可写的 fd                               │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│ 6. EventLoop::handleClient() 处理写事件                     │
│    ssize_t n = write(fd, buffer, size);                    │
│    client->clearOutputBuffer(n);                            │
└────────────────────┬────────────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────────────┐
│ 7. 如果缓冲区还有数据，继续等待 EPOLLOUT                    │
│    如果缓冲区为空，禁用写事件并触发回调                     │
│    client->disableWriting();                                │
│    client->handleWriteComplete();                           │
└─────────────────────────────────────────────────────────────┘
```

### 会话过期通知实现

`auditSessions()` 展示了写事件的完整应用：

```cpp
void UserSessionManager::auditSessions() {
    // 1. 发现过期会话
    if (session->isExpired(now)) {
        
        // 2. 获取 Client 对象
        auto client = g_eventLoop->getClient(clientFd);
        
        // 3. 构造响应消息
        std::string expireNotice = buildSessionExpiredResponse(token);
        
        // 4. 添加到写缓冲区
        client->appendToOutputBuffer(expireNotice);
        
        // 5. 启用写事件
        client->enableWriting();
        
        // 6. 设置写完成回调（发送后关闭连接）
        client->setWriteCompleteCallback([clientFd, eventLoop](Client* c) {
            eventLoop->removeClient(clientFd);
        });
        
        // 7. 更新 EventLoop
        g_eventLoop->updateClient(client);
    }
}
```

## 使用示例

### 示例 1: 发送简单响应

```cpp
void sendResponse(std::shared_ptr<Client> client, EventLoop* loop) {
    std::string response = "HTTP/1.1 200 OK\r\n"
                          "Content-Length: 13\r\n"
                          "\r\n"
                          "Hello, World!";
    
    client->appendToOutputBuffer(response);
    client->enableWriting();
    loop->updateClient(client);
}
```

### 示例 2: 带回调的发送

```cpp
void sendWithCallback(std::shared_ptr<Client> client, EventLoop* loop) {
    client->appendToOutputBuffer("Some data");
    client->enableWriting();
    
    client->setWriteCompleteCallback([](Client* c) {
        LOG_INFO("Data sent to fd=%d", c->getFd());
    });
    
    loop->updateClient(client);
}
```

### 示例 3: 服务器推送

```cpp
void broadcastMessage(const std::string& msg, EventLoop* loop) {
    for (auto& [fd, client] : allClients) {
        client->appendToOutputBuffer(msg);
        client->enableWriting();
        loop->updateClient(client);
    }
}
```

## 关键特性

### 1. 非阻塞 I/O
- 写操作不会阻塞调用线程
- 只在 socket 可写时才执行 write()

### 2. 线程安全
- 所有 I/O 操作在 EventLoop 线程执行
- `updateClient()` 可以从任何线程安全调用

### 3. 可靠性
- 自动处理部分写入（write() 返回 < 请求大小）
- 保持剩余数据，等待下次 EPOLLOUT
- 直到所有数据发送完毕

### 4. 优雅关闭
- 确保数据发送完毕才关闭连接
- 通过 `writeCompleteCallback` 实现

## 测试

运行示例代码：
```bash
# 编译示例
g++ -std=c++17 examples/write_event_example.cpp -I src/Connect/include -o write_example

# 运行示例
./write_example
```

## 文档

详细文档请参考：
- `docx/写事件处理示例.md` - 基础用法和 API 文档
- `docx/会话过期写事件示例.md` - 完整的实际应用案例
- `examples/write_event_example.cpp` - 可运行的示例代码

## API 参考

### Client 类

| 方法 | 说明 |
|------|------|
| `appendToOutputBuffer(data)` | 添加数据到写缓冲区 |
| `enableWriting()` | 启用写事件监听 |
| `disableWriting()` | 禁用写事件监听 |
| `isWriting()` | 检查是否正在监听写事件 |
| `hasDataToWrite()` | 检查缓冲区是否有数据 |
| `setWriteCompleteCallback(cb)` | 设置写完成回调 |
| `setErrorCallback(cb)` | 设置错误回调 |

### EventLoop 类

| 方法 | 说明 |
|------|------|
| `updateClient(client)` | 更新客户端监听的事件 |
| `getClient(fd)` | 获取指定 fd 的 Client 对象 |

## 注意事项

1. **回调生命周期**：确保在回调中捕获的变量生命周期正确
2. **循环引用**：避免 Client 和回调之间的循环引用
3. **错误处理**：始终设置 `errorCallback` 处理异常情况
4. **缓冲区大小**：注意写缓冲区不要无限增长

## 后续改进

- [ ] 添加写缓冲区大小限制
- [ ] 支持写超时检测
- [ ] 添加流量控制
- [ ] 性能优化：零拷贝写入

## 相关文件

### 修改的文件
- `src/Connect/include/Client.h`
- `src/Connect/include/EventLoop.h`
- `src/Connect/include/EPollPoller.h`
- `src/Connect/EventLoop.cpp`
- `src/Connect/EPollPoller.cpp`
- `src/Login/UserSessionCB.cpp`
- `src/Login/include/UserSessionCB.h`

### 新增的文件
- `docx/写事件处理示例.md`
- `docx/会话过期写事件示例.md`
- `examples/write_event_example.cpp`
- `docs/WRITE_EVENT_IMPLEMENTATION.md` (本文件)

## 总结

此实现提供了一个**生产级别的异步写处理机制**，具有以下优势：

✅ **完整性**：从缓冲区管理到事件处理的完整实现  
✅ **可靠性**：自动处理部分写入和错误情况  
✅ **易用性**：简洁的 API，清晰的使用流程  
✅ **实用性**：在会话管理中有真实应用案例  

这为后续的游戏服务器开发奠定了坚实的基础！
