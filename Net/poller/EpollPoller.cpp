//
// Created by taganyer on 24-3-4.
//

#include "EpollPoller.hpp"
#include "../functions/poll_interface.hpp"
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
    if ((_epfd = ops::epoll_create()) < 0) {
        G_ERROR << "EpollPoller create failed in " << _tid;
    }
}

EpollPoller::~EpollPoller() {
    ops::epoll_close(_epfd);
}

int EpollPoller::poll(int timeoutMS, ChannelList &list) {
    assert_in_right_thread("EpollPoller::poll ");
    int active = ops::epoll_wait(_epfd, _eventsQueue.data(),
                                 _eventsQueue.capacity(), timeoutMS);
    if (active > 0) {
        G_TRACE << "EpollPoller::poll " << _tid << " get " << active << " events";
        get_events(list, active);
    } else if (active == 0) {
        G_INFO << "EpollPoller::poll " << _tid << " timeout " << timeoutMS << " ms";
    } else {
        G_ERROR << "EpollPoller::poll " << _tid << " error";
    }
    return active;
}

void EpollPoller::add_channel(Channel *channel) {
    assert_in_right_thread("EpollPoll::add_channel ");
    if (_eventsQueue.capacity() == _channels.size()) {
        _eventsQueue.reserve(_eventsQueue.capacity() << 1);
    }
    assert(_channels.find(channel->fd()) == _channels.end());
    operate(EPOLL_CTL_ADD, channel);
    assert(_channels.insert({channel->fd(), channel}).second);
}

void EpollPoller::remove_channel(int fd) {
    assert_in_right_thread("EpollPoller::remove_channel ");
    auto iter = _channels.find(fd);
    assert(iter != _channels.end());
    operate(EPOLL_CTL_DEL, iter->second);
    _channels.erase(iter);
}

void EpollPoller::update_channel(Channel *channel) {
    assert_in_right_thread("EpollPoller::update_channel ");
    int fd = channel->fd();
    auto iter = _channels.find(fd);
    assert(iter != _channels.end());
    if (iter->second == nullptr) {
        operate(EPOLL_CTL_ADD, channel);
        iter->second = channel;
    } else if (!channel->is_nonevent()) {
        operate(EPOLL_CTL_MOD, channel);
    } else {
        operate(EPOLL_CTL_DEL, channel);
        iter->second = nullptr;
    }
}

void EpollPoller::operate(int operation, Channel *channel) {
    int fd = channel->fd();
    if (ops::epoll_ctl(_epfd, operation, fd, channel->events()) < 0) {
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

void EpollPoller::get_events(Poller::ChannelList &list, int size) {
    assert_in_right_thread("EpollPoller::get_event ");
    list.reserve(size + list.size());
    for (int i = 0; i < size; ++i) {
        auto iter = _channels.find(_eventsQueue[i].data.fd);
        assert(iter != _channels.end());
        iter->second->set_revents((short) _eventsQueue[i].events);
        list.push_back(iter->second);
    }
}
