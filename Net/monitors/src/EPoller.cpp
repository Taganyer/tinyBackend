//
// Created by taganyer on 3/23/24.
//

#include "../EPoller.hpp"
#include "Base/Log/Log.hpp"
#include "Net/error/errors.hpp"
#include "Net/functions/poll_interface.hpp"

static_assert(EPOLLIN == POLLIN, "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI, "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT, "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP, "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR, "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP, "epoll uses same flag values as poll");

Net::EPoller::EPoller() {
    if ((_epfd = ops::epoll_create()) < 0) {
        error_ = {error_types::Epoll_create, errno};
        G_FATAL << "EPoller create failed in " << _tid;
    }
}

Net::EPoller::~EPoller() {
    G_WARN << "Poller force close " << _fds.size();
    for (auto fd: _fds) {
        if (fd > 0)
            operate(EPOLL_CTL_DEL, fd, 0);
    }
    _fds.clear();
    if (ops::epoll_close(_epfd) < 0) {
        G_FATAL << "EPoller " << _epfd << ' ' << ops::get_close_error(errno);
    }
}

int Net::EPoller::get_aliveEvent(int timeoutMS, Net::Monitor::EventList &list) {
    assert_in_right_thread("EPoller::poll ");
    int active = ops::epoll_wait(_epfd, activeEvents.data(),
                                 (int) activeEvents.capacity(), timeoutMS);
    if (active > 0) {
        G_TRACE << "EPoller::poll " << _tid << " get " << active << " events";
        get_events(list, active);
    } else if (active == 0) {
        G_INFO << "EPoller::poll " << _tid << " timeout " << timeoutMS << " ms";
    } else {
        error_ = {error_types::Epoll_wait, errno};
        G_FATAL << "EPoller::poll " << _tid << ' ' << ops::get_epoll_wait_error(errno);
    }
    return active;
}

bool Net::EPoller::add_fd(Net::Event event) {
    assert_in_right_thread("EPoller::add_fd ");
    if (_fds.find(event.fd) != _fds.end())
        return false;
    if (event.is_NoEvent())
        _fds.insert(-event.fd);
    else if (!operate(EPOLL_CTL_ADD, event.fd, event.event))
        return false;
    else
        _fds.insert(event.fd);
    activeEvents.emplace_back();
    G_TRACE << "EPoller add " << event.fd;
    return true;
}

void Net::EPoller::remove_fd(int fd) {
    assert_in_right_thread("EPoller::remove_fd ");
    auto iter = _fds.find(fd);
    if (iter == _fds.end()) return;
    operate(EPOLL_CTL_DEL, fd, 0);
    _fds.erase(iter);
    activeEvents.pop_back();
    G_TRACE << "EPoller remove " << fd;
}

void Net::EPoller::remove_all() {
    assert_in_right_thread("EPoller::remove_all ");
    G_WARN << "Poller force close " << _fds.size();
    for (auto fd: _fds) {
        if (fd > 0)
            operate(EPOLL_CTL_DEL, fd, 0);
    }
    _fds.clear();
    activeEvents.clear();
}

void Net::EPoller::update_fd(Net::Event event) {
    assert_in_right_thread("EPoller::update_channel ");
    auto iter = _fds.find(event.fd);
    if (iter == _fds.end()) {
        if (!event.is_NoEvent() || !_fds.erase(-event.fd))
            return;
        _fds.insert(event.fd);
        operate(EPOLL_CTL_ADD, event.fd, event.event);
    } else if (event.is_NoEvent()) {
        _fds.erase(iter);
        _fds.insert(-event.fd);
        operate(EPOLL_CTL_DEL, event.fd, event.event);
    } else {
        operate(EPOLL_CTL_MOD, event.fd, event.event);
    }
    G_TRACE << "EPoller update " << event.fd << " events to " << event.event;
}

void Net::EPoller::get_events(EventList &list, int size) {
    list.reserve(size + list.size());
    for (int i = 0; i < size; ++i)
        list.push_back({activeEvents[i].data.fd, (int) activeEvents[i].events});
}

bool Net::EPoller::operate(int mod, int fd, int events) {
    if (unlikely(ops::epoll_ctl(_epfd, mod, fd, events) < 0)) {
        error_ = {error_types::Epoll_ctl, errno};
        if (mod == EPOLL_CTL_DEL) {
            G_ERROR << "EPoller DEL " << fd << ' ' << ops::get_epoll_ctl_error(errno);
        } else if (mod == EPOLL_CTL_ADD) {
            G_FATAL << "epoll_ctl ADD " << fd << ' ' << ops::get_epoll_ctl_error(errno);
        } else {
            G_FATAL << "epoll_ctl MOD " << fd << ' ' << ops::get_epoll_ctl_error(errno);
        }
        return false;
    }
    return true;
}
