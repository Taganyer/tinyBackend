//
// Created by taganyer on 24-3-4.
//

#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "EpollPoller.hpp"
#include "../../Base/Log/Log.hpp"
#include "../Channel.hpp"

static_assert(EPOLLIN == POLLIN, "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI, "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT, "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP, "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR, "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP, "epoll uses same flag values as poll");

using namespace Net;

EpollPoller::EpollPoller() {
    _eventsQueue.reserve(1);
    if ((_epfd = ::epoll_create1(EPOLL_CLOEXEC)) < 0) {
        G_ERROR << "EpollPoller create failed in " << _tid;
    }
}

EpollPoller::~EpollPoller() {
    ::close(_epfd);
}

void EpollPoller::poll(int timeoutMS) {
    assert_in_right_thread("EpollPoller::poll ");
    if (_begin != _end) return;
    int active = ::epoll_wait(_epfd, _eventsQueue.data(),
                              _eventsQueue.capacity(), timeoutMS);
    if (active > 0) {
        _begin = 0;
        _end = active;
        G_TRACE << "EpollPoller::poll " << _tid << " get " << active << " events";
    } else if (active == 0) {
        G_INFO << "EpollPoller::poll " << _tid << " timeout " << timeoutMS << " ms";
    } else {
        G_ERROR << "EpollPoller::poll " << _tid << " error";
    }
}

Channel *EpollPoller::get_events() {
    assert_in_right_thread("EpollPoller::get_event ");
    if (_begin == _end) return nullptr;
    auto iter = _channels.find(_eventsQueue[_begin].data.fd);
    assert(iter != _channels.end());
    iter->second->set_revents((short) _eventsQueue[_begin].events);
    ++_begin;
    return iter->second;
}

void EpollPoller::add_channel(ConnectionsManger *manger, Channel *channel) {
    assert_in_right_thread("EpollPoll::add_channel ");
    if (_eventsQueue.capacity() == _channels.size()) {
        ActiveEvents temp;
        temp.reserve(_eventsQueue.capacity() << 1);
        _eventsQueue.swap(temp);
    }
    assert(_channels.find(channel->fd()) == _channels.end());
    operate(EPOLL_CTL_ADD, channel);
    _channels.insert({channel->fd(), channel});
}

void EpollPoller::remove_channel(ConnectionsManger *manger, int fd) {
    assert_in_right_thread("EpollPoller::remove_channel ");
    auto iter = _channels.find(fd);
    assert(iter != _channels.end());
    operate(EPOLL_CTL_DEL, iter->second);
    _channels.erase(iter);
}

void EpollPoller::update_channel(Channel *channel) {
    assert_in_right_thread("EpollPoller::remove_channel ");
    int fd = channel->fd();
    auto iter = _channels.find(fd);
    assert(iter != _channels.end());
    if (iter->second == nullptr) {
        operate(EPOLL_CTL_ADD, channel);
        iter->second = channel;
    } else if (channel->has_event()) {
        operate(EPOLL_CTL_MOD, channel);
    } else {
        operate(EPOLL_CTL_DEL, channel);
        iter->second = nullptr;
    }
}

void EpollPoller::operate(int operation, Channel *channel) {
    struct epoll_event event{};
    event.events = channel->events();
    int fd = channel->fd();
    if (::epoll_ctl(_epfd, operation, fd, &event) < 0) {
        if (operation == EPOLL_CTL_DEL) {
            G_ERROR << "epoll_ctl DEl failed " << fd;
        } else if (operation == EPOLL_CTL_ADD) {
            G_FATAL << "epoll_ctl ADD failed " << fd;
        } else {
            G_FATAL << "epoll_ctl MOD failed " << fd;
        }
    } else {
        if (operation == EPOLL_CTL_DEL) {
            G_TRACE << "epoll_ctl DEl " << fd;
        } else if (operation == EPOLL_CTL_ADD) {
            G_TRACE << "epoll_ctl ADD " << fd;
        } else {
            G_TRACE << "epoll_ctl MOD " << fd;
        }
    }
}
