#include "QueryUserData.h"
#include "DBConnPool.h"
#include "LogM.h"
#include <memory>
#include <chrono>
std::string queryUserPwd(const std::string &username)
{
    ConnectionPoolAgent dbAgent(&GetUserDBPool());
    if (!dbAgent) {
        LOG_ERROR("Failed to get database connection");
        return "";
    }
    
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            dbAgent->prepareStatement(
                "SELECT password_hash FROM sys_user WHERE username = ?"
            )
        );
        pstmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> resultSet(pstmt->executeQuery());
        if (resultSet->next()) {
            return resultSet->getString("password_hash");
        } else {
            return ""; // 用户不存在
        }
    } catch (const std::exception& e) {
        // 处理异常，例如记录日志
        LOG_ERROR("Database query error: %s", e.what());
        return "";
    }
}

bool IsUserExists(const std::string &username)
{
    ConnectionPoolAgent dbAgent(&GetUserDBPool());
    if (!dbAgent) {
        LOG_ERROR("Failed to get database connection");
        return false;
    }
    
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            dbAgent->prepareStatement(
                "SELECT 1 FROM sys_user WHERE username = ?"
            )
        );
        pstmt->setString(1, username);
        std::unique_ptr<sql::ResultSet> resultSet(pstmt->executeQuery());
        return resultSet->next();
    } catch (const std::exception& e) {
        // 处理异常，例如记录日志
        LOG_ERROR("Database query error: %s", e.what());
        return false;
    }
}

bool InsertUserInfo(const string& username, const string pwd, const string invCode)
{
    auto now = std::chrono::system_clock::now();
    string nowStr = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()
        ).count()
    );
    ConnectionPoolAgent dbAgent(&GetUserDBPool());
    if (!dbAgent) {
        LOG_ERROR("Failed to get database connection");
        return false;
    }
    
    try {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            dbAgent->prepareStatement(
                "INSERT INTO sys_user (username, password_hash, inv_code, signUp_time) VALUES (?, ?, ?, ?)"
            )
        );
        pstmt->setString(1, username);
        pstmt->setString(2, pwd);
        pstmt->setString(3, invCode);
        pstmt->setString(4, nowStr);
        int affectedRows = pstmt->executeUpdate();
        return affectedRows > 0;
    } catch (const std::exception& e) {
        // 处理异常，例如记录日志
        LOG_ERROR("Database insert error: %s", e.what());
        return false;
    }
}
