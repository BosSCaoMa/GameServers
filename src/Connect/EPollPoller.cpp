#include "EPollPoller.h"
#include "EventLoop.h"
#include "LogM.h"
#include <unistd.h>
#include <errno.h>

EPollPoller::EPollPoller(EventLoop *loop)
    : loop_(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (epollfd_ < 0) {
        LOG_ERROR("EPollPoller::EPollPoller epoll_create1 error");
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

void EPollPoller::poll(int timeoutMs, ClientList *activeClients)
{
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), 
                                static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;

    if (numEvents > 0) {
        LOG_DEBUG("%d events happened", numEvents);
        fillActiveClients(numEvents, activeClients);
        
        // 如果返回的事件数等于events_数组大小，说明可能有更多事件，扩容
        if (numEvents == static_cast<int>(events_.size()) || events_.size() < 64) { // 必须限制最大容量，避免无限增长
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_DEBUG("EPollPoller::poll timeout, no events");
    } else {
        if (saveErrno != EINTR) {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() error");
        }
    }
}

void EPollPoller::addClient(std::shared_ptr<Client> client, uint32_t events)
{
    int fd = client->getFd();
    
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    
    if (::epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) < 0) {
        LOG_ERROR("EPollPoller::addClient epoll_ctl ADD error for fd=%d", fd);
        return;
    }
    
    clients_[fd] = client;
    LOG_DEBUG("EPollPoller::addClient fd=%d events=%u", fd, events);
}

void EPollPoller::updateClient(int fd, uint32_t events)
{
    epoll_event event;
    event.events = events;
    event.data.fd = fd;
    
    if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event) < 0) {
        LOG_ERROR("EPollPoller::updateClient epoll_ctl MOD error for fd=%d", fd);
    }
    
    LOG_DEBUG("EPollPoller::updateClient fd=%d events=%u", fd, events);
}

void EPollPoller::removeClient(int fd)
{
    epoll_event event; // kernel < 2.6.9需要传入一个event，虽然会被忽略
    
    if (::epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &event) < 0) {
        LOG_ERROR("EPollPoller::removeClient epoll_ctl DEL error for fd=%d", fd);
    }
    
    clients_.erase(fd);
    LOG_DEBUG("EPollPoller::removeClient fd=%d", fd);
}

std::shared_ptr<Client> EPollPoller::getClient(int fd) const
{
    auto it = clients_.find(fd);
    if (it != clients_.end()) {
        return it->second;
    }
    return nullptr;
}

void EPollPoller::fillActiveClients(int numEvents, ClientList* activeClients)
{
    for (int i = 0; i < numEvents; ++i) {
        int fd = events_[i].data.fd;
        uint32_t revents = events_[i].events;
        
        auto it = clients_.find(fd);
        if (it != clients_.end()) {
            it->second->setRevents(revents);  // 设置活动事件
            activeClients->push_back(it->second);
        } else {
            LOG_ERROR("EPollPoller::fillActiveClients fd=%d not found in clients_", fd);
        }
    }
}
