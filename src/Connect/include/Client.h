#ifndef CLIENT_H
#define CLIENT_H

#include <unistd.h> // for close()
#include <functional>
#include <string>
#include <sys/epoll.h>

class Client {
public:
    using ReadCallback = std::function<void(Client*, const char*, ssize_t)>;
    using WriteCompleteCallback = std::function<void(Client*)>;
    using ErrorCallback = std::function<void(Client*)>;

    explicit Client(int fd) 
        : fd_(fd), 
          revents_(0),
          events_(EPOLLIN | EPOLLPRI) // 默认监听读事件
    {}
    
    ~Client() { 
        if (fd_ >= 0) {
            close(fd_); 
        }
    }
    
    // 禁止拷贝，只允许移动
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    
    Client(Client&& other) noexcept 
        : fd_(other.fd_), 
          revents_(other.revents_),
          events_(other.events_),
          outputBuffer_(std::move(other.outputBuffer_)),
          readCallback_(std::move(other.readCallback_)),
          writeCompleteCallback_(std::move(other.writeCompleteCallback_)),
          errorCallback_(std::move(other.errorCallback_))
    {
        other.fd_ = -1;
        other.revents_ = 0;
        other.events_ = 0;
    }
    
    Client& operator=(Client&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) close(fd_);
            fd_ = other.fd_;
            revents_ = other.revents_;
            events_ = other.events_;
            outputBuffer_ = std::move(other.outputBuffer_);
            readCallback_ = std::move(other.readCallback_);
            writeCompleteCallback_ = std::move(other.writeCompleteCallback_);
            errorCallback_ = std::move(other.errorCallback_);
            other.fd_ = -1;
            other.revents_ = 0;
            other.events_ = 0;
        }
        return *this;
    }
    
    int getFd() const { return fd_; }
    bool isValid() const { return fd_ >= 0; }
    
    // epoll事件相关
    void setRevents(uint32_t revents) { revents_ = revents; }
    uint32_t getRevents() const { return revents_; }
    
    void setEvents(uint32_t events) { events_ = events; }
    uint32_t getEvents() const { return events_; }
    
    // 回调函数设置
    void setReadCallback(const ReadCallback& cb) { readCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setErrorCallback(const ErrorCallback& cb) { errorCallback_ = cb; }
    
    // 写缓冲区操作
    void appendToOutputBuffer(const std::string& data) { outputBuffer_.append(data); }
    void appendToOutputBuffer(const char* data, size_t len) { outputBuffer_.append(data, len); }
    const std::string& getOutputBuffer() const { return outputBuffer_; }
    void clearOutputBuffer(size_t len) { outputBuffer_.erase(0, len); }
    bool hasDataToWrite() const { return !outputBuffer_.empty(); }
    
    // 启用/禁用写事件监听
    void enableWriting() { events_ |= EPOLLOUT; }
    void disableWriting() { events_ &= ~EPOLLOUT; }
    bool isWriting() const { return events_ & EPOLLOUT; }
    
    // 回调触发
    void handleRead(const char* data, ssize_t len) {
        if (readCallback_) readCallback_(this, data, len);
    }
    void handleWriteComplete() {
        if (writeCompleteCallback_) writeCompleteCallback_(this);
    }
    void handleError() {
        if (errorCallback_) errorCallback_(this);
    }
    
private:
    int fd_;
    uint32_t revents_; // epoll返回的活动事件
    uint32_t events_;  // 当前监听的事件
    
    std::string outputBuffer_; // 写缓冲区
    
    ReadCallback readCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ErrorCallback errorCallback_;
};

#endif