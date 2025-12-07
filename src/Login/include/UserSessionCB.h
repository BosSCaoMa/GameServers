#ifndef USERSESSIONCB_H
#define USERSESSIONCB_H

#include <string>
#include <chrono>
#include <unordered_map>
#include <variant>
#include <cstdint>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
struct UserSessionData {
    std::string username;
    std::string token;
    std::string clientIp;

    // ---- lifecycle ----
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;
    TimePoint createdAt{};
    TimePoint lastAccessAt{};
    TimePoint expireAt{};

    using Attr = std::variant<int64_t, double, bool, std::string>;
    std::unordered_map<std::string, Attr> attrs;
};

class UserSessionCB {
public:
    using Clock = UserSessionData::Clock;
    using TimePoint = UserSessionData::TimePoint;

    explicit UserSessionCB(const std::string& token, const std::string& username, int clientFd);
    ~UserSessionCB() = default;

    bool isExpired(TimePoint now = Clock::now()) const;
    void touch(TimePoint now = Clock::now());
    int getClientFd() const { return clientFd_; }
private:
    mutable std::mutex mu_;
    UserSessionData data_;
    int clientFd_;
};

class UserSessionManager {
public:
    static UserSessionManager& getInstance();

    std::shared_ptr<UserSessionCB> createSession(const std::string& token,
                                                 const std::string& username,
                                                 int clientFd);

    std::shared_ptr<UserSessionCB> getSession(const std::string& token);

private:
    UserSessionManager();

    ~UserSessionManager() = default;
    void auditSessions();
    UserSessionManager(const UserSessionManager&) = delete;
    UserSessionManager& operator=(const UserSessionManager&) = delete;

    std::mutex mu_;
    std::unordered_map<std::string, std::shared_ptr<UserSessionCB>> sessions_;
    std::atomic<uint64_t> sessionCounter_{0};
};

#endif // USERSESSIONCB_H