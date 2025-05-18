//
// Created by taganyer on 3/23/24.
//

#include "../EPoller.hpp"

#include "tinyBackend/Base/GlobalObject.hpp"
#include "tinyBackend/Net/error/errors.hpp"
#include "tinyBackend/Net/functions/poll_interface.hpp"

static_assert(EPOLLIN == POLLIN, "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI, "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT, "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP, "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR, "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP, "epoll uses same flag values as poll");

Net::EPoller::EPoller() {
    if ((_epfd = ops::epoll_create()) < 0) {
        error_ = { error_types::Epoll_create, errno };
        G_FATAL << "EPoller create failed in " << _tid;
    }
}

Net::EPoller::~EPoller() {
    if (_fds.size() > 0)
        G_WARN << "EPoller force remove " << _fds.size();
    if (ops::epoll_close(_epfd) < 0)
        G_FATAL << "EPoller " << _epfd << ' ' << ops::get_close_error(errno);
}

int Net::EPoller::get_aliveEvent(int timeoutMS, EventList& list) {
    int active = ops::epoll_wait(_epfd, activeEvents.data(),
                                 (int) activeEvents.capacity(), timeoutMS);
    if (active > 0) {
        G_TRACE << "EPoller::poll " << _tid << " get " << active << " events";
        get_events(list, active);
    } else if (active == 0) {
        G_INFO << "EPoller::poll " << _tid << " timeout " << timeoutMS << " ms";
    } else {
        error_ = { error_types::Epoll_wait, errno };
        G_FATAL << "EPoller::poll " << _tid << ' ' << ops::get_epoll_wait_error(errno);
    }

    return active;
}

bool Net::EPoller::add_fd(Event event) {
    if (event.fd < 0 || _fds.find(event.fd) != _fds.end()) {
        G_INFO << "EPoller add " << event.fd << " failed.";
        return false;
    }
    auto [iter, success] = _fds.emplace(event.is_NoEvent() ? -event.fd : event.fd, event);
    if (!success || !operate(EPOLL_CTL_ADD, &iter->second))
        return false;
    activeEvents.emplace_back();
    G_INFO << "EPoller add " << event.fd;
    return true;
}

void Net::EPoller::remove_fd(int fd, bool fd_closed) {
    auto iter = _fds.find(fd);
    if (iter == _fds.end()) return;
    if (!fd_closed)
        operate(EPOLL_CTL_DEL, &iter->second);
    _fds.erase(iter);
    activeEvents.pop_back();
    G_INFO << "EPoller remove" << (fd_closed ? " closed fd " : " fd ") << fd;
}

void Net::EPoller::remove_all() {
    if (_fds.size() > 0)
        G_WARN << "EPoller force remove " << _fds.size() << " fds.";
    for (auto [fd, event] : _fds) {
        if (fd > 0) operate(EPOLL_CTL_DEL, &event);
    }
    _fds.clear();
    activeEvents.clear();
}

void Net::EPoller::update_fd(Event event) {
    auto iter = _fds.find(event.fd);
    if (iter == _fds.end()) {
        auto [new_iter, success] = _fds.emplace(event.fd, event);
        if (!success) {
            G_TRACE << "EPoller update " << event.fd << " failed and remove it.";
            return;
        }
        operate(EPOLL_CTL_ADD, &new_iter->second);
        activeEvents.emplace_back();
    } else if (event.is_NoEvent()) {
        _fds.erase(iter);
        operate(EPOLL_CTL_DEL, &iter->second);
        activeEvents.pop_back();
    } else {
        iter->second.extra_data = event.extra_data;
        if (iter->second.event != event.event)
            operate(EPOLL_CTL_MOD, &iter->second);
    }
    G_TRACE << "EPoller update " << event.fd << " events to " << event.event;
}

bool Net::EPoller::exist_fd(int fd) const {
    return _fds.find(fd) != _fds.end();
}

void Net::EPoller::get_events(EventList& list, int size) {
    list.reserve(size + list.size());
    for (int i = 0; i < size; ++i) {
        auto *event = (Event *) activeEvents[i].data.ptr;
        list.push_back({ event->fd, (int) activeEvents[i].events, event->extra_data });
    }
}

bool Net::EPoller::operate(int mod, Event *event) {
    epoll_event _event {};
    _event.events = event->event;
    _event.data.ptr = event;
    if (unlikely(ops::epoll_ctl(_epfd, mod, event->fd, &_event) < 0)) {
        error_ = { error_types::Epoll_ctl, errno };
        if (mod == EPOLL_CTL_DEL) {
            G_ERROR << "EPoller DEL " << event->fd << ' ' << ops::get_epoll_ctl_error(errno);
        } else if (mod == EPOLL_CTL_ADD) {
            G_FATAL << "epoll_ctl ADD " << event->fd << ' ' << ops::get_epoll_ctl_error(errno);
        } else {
            G_FATAL << "epoll_ctl MOD " << event->fd << ' ' << ops::get_epoll_ctl_error(errno);
        }
        return false;
    }
    return true;
}
