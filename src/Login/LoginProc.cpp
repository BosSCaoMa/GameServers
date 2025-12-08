#include "LoginProc.h"
#include "LogM.h"
#include "http_response.h"
#include "SafetyPwd.h"
#include "Client.h"
#include "EventLoop.h"
#include "UserSessionCB.h"
#include "ParseHttp.h"
#include "SafetyPwd.h"
#include "QueryUserData.h"
#include <memory>
using namespace std;
// 全局EventLoop实例 - 在实际项目中可能通过单例或依赖注入管理
extern EventLoop* g_eventLoop;

void BuildSession(HttpRequest& request, std::shared_ptr<Client> client, const std::string& token)
{
    // 这里可以创建会话信息，设置用户状态等
    LOG_DEBUG("Building session for client fd=%d", client->getFd());
    string key = "username";
    UserSessionManager::getInstance().createSession(token, request.getParam(key), client->getFd());
    // 将认证成功的连接交给EventLoop管理
    // EventLoop会接管这个连接的后续读写事件
    // todo 未来会有多个EventLoop，通过负载均衡算法进行选择
    if (g_eventLoop) {
        g_eventLoop->addClient(client);
        LOG_DEBUG("Client fd=%d added to EventLoop", client->getFd());
    }
}

void SendLoginSuccessResponse(int client_fd, const std::string& token)
{
    nlohmann::json responseJson = {
        {"status", "success"},
        {"message", "Login successful"},
        {"token", token}
    };
    
    send_json_response(client_fd, 200, responseJson, true);
}

bool ProcLoginRequest(HttpRequest& request, std::shared_ptr<Client> client)
{
    int client_fd = client->getFd();
    string username = request.getParam("username");
    string password = request.getParam("password");
    
    if (verifyPassword(password, queryUserPwd(username))) {
        // ⚠️ 重要：先发送响应，再交给EventLoop
        // 原因：交给EventLoop后，handle_client线程就结束了，
        // 如果先addClient，可能EventLoop还没poll到，但响应还没发送
        string token = generateToken(username);
        SendLoginSuccessResponse(client_fd, token);
        BuildSession(request, client, token);
        return true; // 连接已交给EventLoop管理
    }

    // 发送失败响应给客户端
    send_json_response(client_fd, 401, {{"error", "Invalid username or password"}}, false);
    return false; // 认证失败，连接应该关闭
}