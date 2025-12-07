#include "SafetyPwd.h"

#include <sodium.h>
#include <string>
#include <stdexcept>
#include <chrono>
#include <atomic>
#include <random>
#include <sstream>
#include <iomanip>
std::string hashPassword(const std::string& password)
{
    char hash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(
            hash,
            password.c_str(),
            password.size(),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
        // 一般是内存不够之类的
        throw std::runtime_error("crypto_pwhash_str failed");
    }

    return std::string(hash);  // 直接存到数据库里
}

bool verifyPassword(const std::string& password, const std::string& storedHash)
{
    // 返回 0 表示验证通过
    return crypto_pwhash_str_verify(
               storedHash.c_str(),
               password.c_str(),
               password.size()
           ) == 0;
}

std::string generateToken(const std::string& username)
{
    using namespace std::chrono;

    // 1) 微秒级时间戳
    uint64_t ts = duration_cast<microseconds>(
        system_clock::now().time_since_epoch()
    ).count();

    // 2) 进程内单调递增计数器（保证同一微秒内也不重复）
    static std::atomic<uint64_t> counter{0};
    uint64_t c = counter.fetch_add(1, std::memory_order_relaxed);

    // 3) 少量随机扰动（让外观更“散”）
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    uint64_t r = rng();

    // 4) 组合后转 hex
    // 可把 username 混进去（不影响唯一性，只是区分度更明显）
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << ts
        << std::setw(16) << std::setfill('0') << c
        << std::setw(16) << std::setfill('0') << r;

    // 如果你希望 token 和用户名有关联，可再附加一个简短hash
    // 这里用 std::hash 做轻量混合（不保证安全，只加特征）
    size_t u = std::hash<std::string>{}(username);
    oss << std::setw(sizeof(size_t)*2) << std::setfill('0') << u;

    return oss.str();
}