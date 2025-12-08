#ifndef EPOLL_POLLER_H
#define EPOLL_POLLER_H

#include <unordered_map>
#include <memory>
#include <vector>
#include <sys/epoll.h>
#include "Client.h"

class EventLoop;
class EPollPoller {
public:
    using ClientList = std::vector<std::shared_ptr<Client>>;
    
    EPollPoller(EventLoop* loop);
    ~EPollPoller();

    // 返回活跃的Client列表
    void poll(int timeoutMs, ClientList* activeClients);
    
    // Client管理
    void addClient(std::shared_ptr<Client> client, uint32_t events);
    void updateClient(int fd, uint32_t events);
    void removeClient(int fd);
    std::shared_ptr<Client> getClient(int fd) const; // 获取指定fd的Client
    
private:
    void fillActiveClients(int numEvents, ClientList* activeClients);
    
    static const int kInitEventListSize = 16;
    
    int epollfd_;
    EventLoop* loop_;
    std::unordered_map<int, std::shared_ptr<Client>> clients_;
    std::vector<epoll_event> events_;
};

#endif // EPOLL_POLLER_H