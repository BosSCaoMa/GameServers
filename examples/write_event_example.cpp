/**
 * 写事件处理完整示例
 * 
 * 这个文件演示了如何使用写事件处理机制，
 * 包括会话过期通知的完整实现。
 */

#include "EventLoop.h"
#include "Client.h"
#include "UserSessionCB.h"
#include "LogM.h"
#include <iostream>
#include <string>

// 全局 EventLoop（在实际应用中应该更好地管理）
EventLoop* g_eventLoop = nullptr;

/**
 * 示例 1: 简单的写事件使用
 */
void example1_simpleWrite() {
    std::cout << "\n=== 示例 1: 简单的写事件使用 ===\n";
    
    // 假设我们有一个已连接的客户端
    int fakeFd = 10; // 实际应用中这是真实的 socket fd
    auto client = std::make_shared<Client>(fakeFd);
    
    // 准备要发送的数据
    std::string response = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 13\r\n"
                          "\r\n"
                          "Hello, World!";
    
    // 步骤 1: 添加到写缓冲区
    client->appendToOutputBuffer(response);
    std::cout << "✓ 数据已添加到写缓冲区\n";
    
    // 步骤 2: 启用写事件
    client->enableWriting();
    std::cout << "✓ 已启用写事件监听\n";
    
    // 步骤 3: 设置写完成回调
    client->setWriteCompleteCallback([](Client* c) {
        std::cout << "✓ 写完成回调被触发！数据已发送完毕。\n";
    });
    
    // 步骤 4: 通知 EventLoop 更新
    // g_eventLoop->updateClient(client);
    std::cout << "✓ 已通知 EventLoop 更新事件监听\n";
    
    std::cout << "=> EventLoop 会在 socket 可写时自动发送数据\n";
}

/**
 * 示例 2: 带错误处理的写事件
 */
void example2_writeWithErrorHandling() {
    std::cout << "\n=== 示例 2: 带错误处理的写事件 ===\n";
    
    int fakeFd = 11;
    auto client = std::make_shared<Client>(fakeFd);
    
    // 设置错误回调
    client->setErrorCallback([](Client* c) {
        std::cout << "❌ 错误回调被触发！fd=" << c->getFd() << "\n";
        // 在这里执行清理工作
    });
    
    // 设置写完成回调
    client->setWriteCompleteCallback([](Client* c) {
        std::cout << "✓ 数据发送成功！fd=" << c->getFd() << "\n";
    });
    
    // 发送数据
    std::string data = "Some important data";
    client->appendToOutputBuffer(data);
    client->enableWriting();
    
    std::cout << "=> 如果发生错误，errorCallback 会被调用\n";
    std::cout << "=> 如果成功，writeCompleteCallback 会被调用\n";
}

/**
 * 示例 3: 会话过期通知（真实场景）
 */
void example3_sessionExpiry() {
    std::cout << "\n=== 示例 3: 会话过期通知（真实场景）===\n";
    
    // 模拟会话管理器检测到过期会话
    std::string expiredToken = "abc123def456";
    int clientFd = 12;
    
    std::cout << "后台审计线程发现会话过期: token=" << expiredToken << "\n";
    
    // 这是 auditSessions() 中实际执行的逻辑：
    
    // 1. 获取 Client 对象
    // auto client = g_eventLoop->getClient(clientFd);
    std::cout << "1. 获取 Client 对象 (fd=" << clientFd << ")\n";
    
    // 2. 构造过期通知
    std::cout << "2. 构造会话过期响应:\n";
    std::cout << "   HTTP/1.1 401 Unauthorized\n";
    std::cout << "   Content-Type: application/json\n";
    std::cout << "   {\"error\":\"session_expired\",\"message\":\"...\"}\n";
    
    // 3. 添加到写缓冲区并启用写事件
    std::cout << "3. 添加到写缓冲区并启用写事件\n";
    
    // 4. 设置写完成回调
    std::cout << "4. 设置写完成回调（发送后关闭连接）\n";
    
    // 5. 更新 EventLoop
    std::cout << "5. 通知 EventLoop 开始监听 EPOLLOUT\n";
    
    std::cout << "\n执行流程:\n";
    std::cout << "  → EventLoop 检测到 EPOLLOUT 事件\n";
    std::cout << "  → 调用 write() 发送数据\n";
    std::cout << "  → 数据发送完毕\n";
    std::cout << "  → 触发 writeCompleteCallback\n";
    std::cout << "  → 在回调中调用 removeClient() 关闭连接\n";
    std::cout << "  → 客户端收到 401 响应后知道会话已过期\n";
}

/**
 * 示例 4: 分块发送大数据
 */
void example4_chunkedData() {
    std::cout << "\n=== 示例 4: 分块发送大数据 ===\n";
    
    int fakeFd = 13;
    auto client = std::make_shared<Client>(fakeFd);
    
    // 假设我们有很多数据要发送
    std::vector<std::string> chunks = {
        "Chunk 1: First part of data\n",
        "Chunk 2: Second part of data\n",
        "Chunk 3: Third part of data\n"
    };
    
    // 可以一次性添加所有数据到缓冲区
    for (const auto& chunk : chunks) {
        client->appendToOutputBuffer(chunk);
    }
    
    std::cout << "✓ 所有数据块已添加到缓冲区\n";
    std::cout << "  总大小: " << client->getOutputBuffer().size() << " 字节\n";
    
    // 启用写事件
    client->enableWriting();
    
    // EventLoop 会自动处理部分写入的情况
    std::cout << "=> EventLoop 会处理:\n";
    std::cout << "   - 如果一次 write() 没写完，保留剩余数据\n";
    std::cout << "   - 继续等待 EPOLLOUT\n";
    std::cout << "   - 直到所有数据发送完毕\n";
}

/**
 * 示例 5: 服务器推送/广播
 */
void example5_broadcast() {
    std::cout << "\n=== 示例 5: 服务器推送/广播 ===\n";
    
    // 假设我们有多个连接的客户端
    std::vector<std::shared_ptr<Client>> clients;
    for (int i = 0; i < 3; i++) {
        clients.push_back(std::make_shared<Client>(100 + i));
    }
    
    // 要广播的消息
    std::string broadcast = "NOTIFICATION: Server will restart in 5 minutes\n";
    
    std::cout << "广播消息到 " << clients.size() << " 个客户端:\n";
    
    // 给每个客户端发送通知
    for (auto& client : clients) {
        client->appendToOutputBuffer(broadcast);
        client->enableWriting();
        // g_eventLoop->updateClient(client);
        
        std::cout << "  → 客户端 fd=" << client->getFd() << " 已加入发送队列\n";
    }
    
    std::cout << "=> EventLoop 会并发处理所有客户端的写事件\n";
}

/**
 * 关键概念总结
 */
void printSummary() {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "写事件处理关键概念总结\n";
    std::cout << std::string(60, '=') << "\n\n";
    
    std::cout << "【核心 API】\n";
    std::cout << "  Client::appendToOutputBuffer(data)  - 添加数据到写缓冲区\n";
    std::cout << "  Client::enableWriting()             - 启用写事件监听\n";
    std::cout << "  Client::setWriteCompleteCallback()  - 设置写完成回调\n";
    std::cout << "  EventLoop::updateClient(client)     - 更新事件监听\n\n";
    
    std::cout << "【工作流程】\n";
    std::cout << "  1. 准备数据 → appendToOutputBuffer()\n";
    std::cout << "  2. 启用监听 → enableWriting()\n";
    std::cout << "  3. 设置回调 → setWriteCompleteCallback()\n";
    std::cout << "  4. 通知更新 → updateClient()\n";
    std::cout << "  5. [EventLoop] 检测 EPOLLOUT\n";
    std::cout << "  6. [EventLoop] 调用 write() 发送\n";
    std::cout << "  7. [EventLoop] 发送完毕触发回调\n\n";
    
    std::cout << "【优势】\n";
    std::cout << "  ✓ 非阻塞 - 不会阻塞调用线程\n";
    std::cout << "  ✓ 线程安全 - 所有 I/O 在 EventLoop 线程\n";
    std::cout << "  ✓ 可靠性 - 自动处理部分写入\n";
    std::cout << "  ✓ 优雅关闭 - 确保数据发送完毕\n\n";
    
    std::cout << "【应用场景】\n";
    std::cout << "  • 会话过期通知（如本例）\n";
    std::cout << "  • 服务器推送/广播\n";
    std::cout << "  • 大文件传输\n";
    std::cout << "  • 流式响应\n";
    std::cout << "  • 心跳/Keep-Alive\n\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║         写事件处理完整示例和教程                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    
    // 运行所有示例
    example1_simpleWrite();
    example2_writeWithErrorHandling();
    example3_sessionExpiry();
    example4_chunkedData();
    example5_broadcast();
    
    // 打印总结
    printSummary();
    
    std::cout << "注: 这些是演示代码，实际使用需要配合真实的 EventLoop 和 socket\n\n";
    
    return 0;
}
