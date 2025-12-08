#include "UserSessionCB.h"
#include <memory>
#include <sstream>
#include "LogM.h"
#include "EventLoop.h"
#include "Client.h"
#include "json.hpp"
// 前置声明并使用全局 EventLoop 指针
using namespace std;
using json = nlohmann::json;
extern EventLoop* g_eventLoop;

UserSessionCB::UserSessionCB(const std::string& token, const std::string& username, int clientFd)
        : clientFd_(clientFd)
{
    std::scoped_lock lk(mu_);
    data_.token = token;
    data_.username = username;
    auto now = Clock::now();
    data_.createdAt = now;
    data_.lastAccessAt = now;
    data_.expireAt = now + std::chrono::hours(1); // 默认1小时过期
}

bool UserSessionCB::isExpired(TimePoint now) const
{
    return now >= data_.expireAt;
}

void UserSessionCB::touch(TimePoint now) {
    std::scoped_lock lk(mu_);
    data_.lastAccessAt = now;
}

UserSessionManager& UserSessionManager::getInstance()
{
    static UserSessionManager instance;
    return instance;
}

UserSessionManager::UserSessionManager()
{
    thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::minutes(5));
            auditSessions();
        }
    }).detach();
}

std::shared_ptr<UserSessionCB> UserSessionManager::createSession(const std::string& token,
                                                const std::string& username,
                                                int clientFd)
{
    std::lock_guard<std::mutex> lk(mu_);
    auto ses = std::make_shared<UserSessionCB>(token, username, clientFd);
    sessions_[token] = ses;
    ++sessionCounter_;
    return ses;
}

std::shared_ptr<UserSessionCB> UserSessionManager::getSession(const std::string& token)
{
    std::lock_guard<std::mutex> lk(mu_);
    auto it = sessions_.find(token);
    if (it != sessions_.end()) return it->second;
    return nullptr;
}

void UserSessionManager::auditSessions()
{
    std::scoped_lock lk(mu_);
    auto now = UserSessionCB::Clock::now();
    for (auto it = sessions_.begin(); it != sessions_.end(); ) {
        if (it->second->isExpired(now)) {
            LOG_INFO("Auditing: removing expired session token=%s", it->first.c_str());
            
            if (g_eventLoop) {
                int clientFd = it->second->getClientFd();
                
                // 构造会话过期通知响应
                std::string expireNotice = buildSessionExpiredResponse(it->first);
                
                // 使用便捷函数：发送并关闭连接
                g_eventLoop->sendAndClose(clientFd, expireNotice);
                
                LOG_DEBUG("Scheduled session expiry notice for fd=%d", clientFd);
            }
            
            // 从会话列表中移除
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

// 构造会话过期通知的 HTTP 响应
std::string UserSessionManager::buildSessionExpiredResponse(const std::string& token)
{
    // 构造 JSON 响应体
    json responseBody;
    responseBody["error"] = "session_expired";
    responseBody["message"] = "Your session has expired. Please login again.";
    responseBody["token"] = token;
    
    std::string jsonStr = responseBody.dump();
    
    // 构造完整的 HTTP 响应
    std::ostringstream response;
    response << "HTTP/1.1 401 Unauthorized\r\n";
    response << "Content-Type: application/json\r\n";
    response << "Content-Length: " << jsonStr.length() << "\r\n";
    response << "Connection: close\r\n"; // 发送后关闭连接
    response << "\r\n";
    response << jsonStr;
    
    return response.str();
}
