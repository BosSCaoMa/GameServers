#ifndef CLIENT_H
#define CLIENT_H

#include <unistd.h> // for close()

class Client {
public:
    explicit Client(int fd) : fd_(fd), revents_(0) {}
    ~Client() { 
        if (fd_ >= 0) {
            close(fd_); 
        }
    }
    
    // 禁止拷贝，只允许移动
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    
    Client(Client&& other) noexcept : fd_(other.fd_), revents_(other.revents_) {
        other.fd_ = -1; // 转移所有权
        other.revents_ = 0;
    }
    
    Client& operator=(Client&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) close(fd_);
            fd_ = other.fd_;
            revents_ = other.revents_;
            other.fd_ = -1;
            other.revents_ = 0;
        }
        return *this;
    }
    
    int getFd() const { return fd_; }
    bool isValid() const { return fd_ >= 0; }
    
    // epoll事件相关
    void setRevents(uint32_t revents) { revents_ = revents; }
    uint32_t getRevents() const { return revents_; }
    
private:
    int fd_;
    uint32_t revents_; // epoll返回的活动事件
};

#endif