//
// Created by taganyer on 3/23/24.
//

#include "../Poller.hpp"
#include "Base/Log/Log.hpp"
#include "Net/error/errors.hpp"
#include "Net/functions/poll_interface.hpp"

using namespace Net;

using namespace Base;

Poller::~Poller() {
    G_WARN << "Poller force clear " << _fds.size();
    _mapping.clear();
    _fds.clear();
}


int Poller::get_aliveEvent(int timeoutMS, EventList &list) {
    assert_in_right_thread("Poller::get_aliveEvent ");
    int active = ops::poll(_fds.data(), _fds.size(), timeoutMS);
    if (active > 0) {
        get_events(list, active);
        G_TRACE << "Poller::poll " << _tid << " get " << active << " events";
    } else if (active == 0) {
        G_INFO << "Poller::poll " << _tid << " timeout " << timeoutMS << " ms";
    } else {
        error_ = {error_types::Poll, errno};
        G_ERROR << "Poller::poll " << _tid << ' ' << ops::get_poll_error(errno);
    }
    return active;
}

bool Poller::add_fd(Event event) {
    assert_in_right_thread("Poller::add_fd ");
    if (!_mapping.try_emplace(event.fd, _fds.size()).second)
        return false;
    _fds.push_back({event.fd, (short) event.event, 0});
    return true;
}

void Poller::remove_fd(int fd) {
    assert_in_right_thread("Poller::remove_fd ");
    auto iter = _mapping.find(fd);
    if (iter == _mapping.end()) return;
    _fds[iter->second] = _fds.back();
    _fds.pop_back();
    _mapping.erase(iter);
    G_TRACE << "Poller::remove_channel " << fd;
}

void Poller::remove_all() {
    assert_in_right_thread("Poller::remove_all ");
    G_WARN << "Poller force clear " << _fds.size();
    _mapping.clear();
    _fds.clear();
}

void Poller::update_fd(Event event) {
    assert_in_right_thread("Poller::update_fd ");
    auto iter = _mapping.find(event.fd);
    if (iter == _mapping.end()) return;
    _fds[iter->second].events = (short) event.event;
    G_TRACE << "Poller update " << event.fd << " events to " << event.event;
}

uint64 Poller::fd_size() const {
    return _fds.size();
}

void Poller::get_events(EventList &list, int size) {
    list.reserve(size + list.size());
    for (auto const &i: _fds) {
        if (i.revents <= 0)
            continue;
        list.push_back({i.fd, i.revents});
    }
}