#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include <string>
#include <memory>
#include <iostream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>

enum class UserRole {
    User = 0,
    Admin = 1
};

struct DBConnInfo
{
    std::string host;
    std::string user;
    std::string password;
    std::string database;
};


struct UserInfo {
    std::string name;
    std::string email;
    std::string passwordHash;
    UserRole role = UserRole::User;
};

DBConnPool& GetUserDBPool(DBConnInfo info);
DBConnPool& GetGameDBPool(DBConnInfo info);
class DBConnPool {
public:
    DBConnPool(const std::string& host, const std::string& user, const std::string& password,
        const std::string& database, int maxConnections = 10, int minConnections = 2);
    ~DBConnPool();
    DBConnPool(const DBConnPool&) = delete;
    DBConnPool& operator=(const DBConnPool&) = delete;

    // 从池中获取一个连接（shared_ptr）
    std::shared_ptr<sql::Connection> getConnection();

    // 归还连接
    void returnConnection(std::shared_ptr<sql::Connection> conn);
private:
    std::string host_;
    std::string user_;
    std::string password_;
    std::string database_;

    bool isRunning_;
    int maxConnections_;
    int minConnections_;
    int currentConnections_;

    std::queue<std::shared_ptr<sql::Connection>> connections_;
    std::mutex mutex_;
    std::condition_variable condVar_;

    sql::Driver* driver_{nullptr};

    shared_ptr<sql::Connection> createConnection(); // 创建新连接

    bool isConnectionValid(std::shared_ptr<sql::Connection> conn); // 验证连接是否有效
};

class ConnectionPoolAgent {
public:
    explicit ConnectionPoolAgent(ConnectionPool* pool,
                                 int retryIntervalMs = 10)
        : pool_(pool)
    {
        while (pool_) {
            conn_ = pool_->getConnection();
            if (conn_) break;

            // 没拿到就睡一会再试，避免空转占满CPU
            std::this_thread::sleep_for(
                std::chrono::milliseconds(retryIntervalMs)
            );
        }
    }

    ~ConnectionPoolAgent() {
        if (pool_ && conn_) {
            pool_->returnConnection(conn_);
        }
    }

    sql::Connection* operator->() { return conn_.get(); }
    explicit operator bool() const { return conn_ != nullptr; }

private:
    ConnectionPool* pool_;
    std::shared_ptr<sql::Connection> conn_;
};


#endif // CONNECTION_POOL_H