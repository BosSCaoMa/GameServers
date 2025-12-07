#include "UserSessionCB.h"
#include <memory>

using namespace std;
unordered_map<string, shared_ptr<UserSessionCB>> g_userSessions;

UserSessionCB::UserSessionCB(const std::string &token)
{
    std::scoped_lock lk(mu_);
    data_.token = token;
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