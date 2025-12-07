#include "LoginProc.h"
#include "LogM.h"
#include "HttpResponse.h"
#include "SafetyPwd.h"
#include "Client.h"
#include "EventLoop.h"

// 全局EventLoop实例 - 在实际项目中可能通过单例或依赖注入管理
extern EventLoop* g_eventLoop;

void BuildSession(const HttpRequest& request, std::shared_ptr<Client> client)
{
    // 这里可以创建会话信息，设置用户状态等
    LOG_DEBUG("Building session for client fd=%d", client->getFd());
    
    // 将认证成功的连接交给EventLoop管理
    // EventLoop会接管这个连接的后续读写事件
    if (g_eventLoop) {
        g_eventLoop->addClient(client);
        LOG_DEBUG("Client fd=%d added to EventLoop", client->getFd());
    }
}

bool ProcLoginRequest(const HttpRequest& request, std::shared_ptr<Client> client)
{
    int client_fd = client->getFd();
    string username = request.getParam("username");
    string password = request.getParam("password");
    
    if (verifyPassword(password, queryUserPwd(username))) {
        BuildSession(request, client);
        SendLoginSuccessResponse(client);
        return true; // 连接已交给EventLoop管理
    }

    // 发送失败响应给客户端
    send_json_response(client_fd, 401, {{"error", "Invalid username or password"}}, false);
    return false; // 认证失败，连接应该关闭
}