#include "DBConnPool.h"
#include "LogM.h"

using namespace std;
DBConnPool::DBConnPool(const std::string &host, const std::string &user, const std::string &password,
    const std::string &database, int maxConnections, int minConnections)
    : host_(host), user_(user), password_(password), database_(database),
      maxConnections_(maxConnections), minConnections_(minConnections),
      currentConnections_(0), isRunning_(true)
{
    driver_ = sql::mysql::get_mysql_driver_instance();

    for (int i = 0; i < minConnections_; ++i) {
        try {
            auto conn = createConnection();
            connections_.push(conn);
            ++currentConnections_;
        } catch (const sql::SQLException& e) {
            LOG_ERROR("Failed to create initial connection: %s, code: %d", e.what(), e.getErrorCode());
        }
    }
}

DBConnPool::~DBConnPool()
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!isRunning_) return;

    isRunning_ = false;
    // 清空队列，shared_ptr 离开作用域会自动析构连接
    while (!connections_.empty()) {
        connections_.pop();
    }

    // 唤醒所有等待中的线程，让它们返回 nullptr
    condVar_.notify_all();
}

std::shared_ptr<sql::Connection> DBConnPool::getConnection()
{
    {
        unique_lock<std::mutex> lock(mutex_);
        if (!connections_.empty()) {
            auto conn = connections_.front();
            connections_.pop();
            return conn;
        }

        if (currentConnections_ < maxConnections_) {
            ++currentConnections_;
        } else {
            condVar_.wait(lock, [this]() {
                return !isRunning_ || !connections_.empty();
            });
            if (!isRunning_) return nullptr;

            auto conn = connections_.front();
            connections_.pop();
            return conn;
        }
    }

    // 在锁外做可能很慢的 IO 操作
    try {
        shared_ptr<sql::Connection> newConn;
        newConn = createConnection(); // 调用驱动 / 连接数据库等
        return newConn;
    } catch (const sql::SQLException& e) {
        LOG_ERROR("Failed to create connection: %s, code: %d", e.what(), e.getErrorCode());
        std::unique_lock<std::mutex> lock(mutex_);
        if (currentConnections_ > 0) {
            --currentConnections_; // 创建失败，要把刚才占掉的“名额”还回去
        }
        condVar_.notify_all(); // 唤醒一下可能在等的线程
        return nullptr;
    }
}


shared_ptr<sql::Connection> DBConnPool::createConnection()
{
    std::shared_ptr<sql::Connection> conn(
        driver_->connect(host_, user_, password_)
    );
    conn->setSchema(database_);

    return conn;
}

void DBConnPool::returnConnection(std::shared_ptr<sql::Connection> conn)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (!isRunning_) {
        // 池已经关闭了，直接丢弃连接并减少计数
        currentConnections_--;
        return;
    }
    if (isConnectionValid(conn)) {
        connections_.push(conn);
        condVar_.notify_one();
    } else {
        currentConnections_--;
        LOG_INFO("Returned connection is invalid, discarding. Current connections: %d", currentConnections_);
    }
}

bool DBConnPool::isConnectionValid(std::shared_ptr<sql::Connection> conn)
{
    if (!conn) return false;
    
    try {
        // 检查连接是否关闭
        if (conn->isClosed()) {
            return false;
        }
        
        // 执行一个简单的查询来验证连接
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT 1"));
        
        // 如果能执行查询并获得结果，连接是有效的
        return res && res->next();
    } catch (const sql::SQLException& e) {
        LOG_INFO("Connection validation failed: %s, code: %d", e.what(), e.getErrorCode());
        return false;
    } catch (...) {
        LOG_INFO("Unknown error during connection validation");
        return false;
    }
}
