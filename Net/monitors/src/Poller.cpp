//
// Created by taganyer on 3/23/24.
//

#include "../Poller.hpp"
#include "tinyBackend/Base/GlobalObject.hpp"
#include "tinyBackend/Net/error/errors.hpp"
#include "tinyBackend/Net/functions/poll_interface.hpp"

using namespace Net;

using namespace Base;

Poller::~Poller() {
    if (_fds.size() > 0)
        G_WARN << "Poller force close " << _fds.size();
}


int Poller::get_aliveEvent(int timeoutMS, EventList& list) {
    int active = ops::poll(_fds.data(), _fds.size(), timeoutMS);
    if (active > 0) {
        get_events(list, active);
        G_TRACE << "Poller::poll " << _tid << " get " << active << " events";
    } else if (active == 0) {
        G_INFO << "Poller::poll " << _tid << " timeout " << timeoutMS << " ms";
    } else {
        error_ = { error_types::Poll, errno };
        G_ERROR << "Poller::poll " << _tid << ' ' << ops::get_poll_error(errno);
    }

    return active;
}

bool Poller::add_fd(Event event) {
    if (event.fd < 0 || !_mapping.try_emplace(event.fd, _fds.size()).second) {
        G_INFO << "Poller add " << event.fd << " failed.";
        return false;
    }
    _fds.push_back({ event.fd, (short) event.event, 0 });
    _datas.push_back(event.extra_data);
    G_INFO << "Poller add " << event.fd;
    return true;
}

void Poller::remove_fd(int fd, bool fd_closed) {
    auto iter = _mapping.find(fd);
    if (iter == _mapping.end()) return;
    _mapping[_fds.back().fd] = iter->second;
    _fds[iter->second] = _fds.back();
    _fds.pop_back();
    _datas[iter->second] = _datas.back();
    _datas.pop_back();
    _mapping.erase(iter);
    G_INFO << "Poller remove" << (fd_closed ? " closed fd " : " fd ") << fd;
}

void Poller::remove_all() {
    if (_fds.size() > 0)
        G_WARN << "Poller force remove " << _fds.size() << " fds.";
    _mapping.clear();
    _fds.clear();
    _datas.clear();
}

void Poller::update_fd(Event event) {
    auto iter = _mapping.find(event.fd);
    if (iter == _mapping.end()) return;
    _fds[iter->second].events = (short) event.event;
    _datas[iter->second] = event.extra_data;
    G_INFO << "Poller update " << event.fd << " events to " << event.event;
}

bool Poller::exist_fd(int fd) const {
    return _mapping.find(fd) != _mapping.end();
}

uint64 Poller::fd_size() const {
    return _fds.size();
}

void Poller::get_events(EventList& list, int size) {
    list.reserve(size + list.size());
    for (int i = 0; i < _fds.size(); ++i) {
        if (_fds[i].revents <= 0)
            continue;
        list.push_back({ _fds[i].fd, _fds[i].revents, _datas[i] });
    }
}
