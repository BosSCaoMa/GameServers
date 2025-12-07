#include "SafetyPwd.h"

#include <sodium.h>
#include <string>
#include <stdexcept>

std::string hashPassword(const std::string& password) {
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

bool verifyPassword(const std::string& password, const std::string& storedHash) {
    // 返回 0 表示验证通过
    return crypto_pwhash_str_verify(
               storedHash.c_str(),
               password.c_str(),
               password.size()
           ) == 0;
}