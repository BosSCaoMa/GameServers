#ifndef USERSESSIONCB_H
#define USERSESSIONCB_H

#include <string>
#include <chrono>
#include <unordered_map>
#include <variant>
#include <cstdint>

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
    using Clock = SessionData::Clock;
    using TimePoint = SessionData::TimePoint;

    explicit UserSessionCB(const std::string& token);
    ~UserSessionCB() = default;

    bool isExpired(TimePoint now = Clock::now()) const;

    void touch(TimePoint now = Clock::now());
private:
    mutable std::mutex mu_;
    UserSessionData data_;
};

#endif // USERSESSIONCB_H