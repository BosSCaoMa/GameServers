#include "QueryUserData.h"
#include "DBConnPool.h"
#include "LogM.h"
std::string queryUserPwd(const std::string &username)
{
    ConnectionPoolAgent dbAgent(*GetUserDBPool());
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
        LOG_ERROR("Database query error: {}", e.what());
        return "";
    }
}