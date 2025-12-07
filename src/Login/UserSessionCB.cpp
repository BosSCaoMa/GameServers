#include "UserSessionCB.h"
#include <memory>
#include "LogM.h"
#include "EventLoop.h"
// 前置声明并使用全局 EventLoop 指针
using namespace std;
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
                g_eventLoop->removeClient(it->second->getClientFd());
            }
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}
