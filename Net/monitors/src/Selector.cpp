//
// Created by taganyer on 3/23/24.
//

#include "../Selector.hpp"
#include "Net/error/errors.hpp"
#include "Base/Log/Log.hpp"

using namespace Net;

Selector::~Selector() {
    if (_fds.size() > 0)
        G_WARN << "Selector force close " << _fds.size();
}

int Selector::get_aliveEvent(int timeoutMS, EventList &list) {
    assert_in_right_thread("Selector::get_aliveEvent ");

    init_fd_set();
    timeval timeout { timeoutMS / 1000, timeoutMS % 1000 };
    int ret = ::select(ndfs + 1,
                       read_size > 0 ? &_read : nullptr,
                       write_size > 0 ? &_write : nullptr,
                       error_size > 0 ? &_error : nullptr,
                       timeoutMS > 0 ? &timeout : nullptr);
    if (ret > 0) {
        G_TRACE << "Selector::select " << _tid << " get " << ret << " events";
        list.reserve(list.size() + ret);
        fill_events(list);
    } else if (ret == 0) {
        G_INFO << "Selector::select " << _tid << " timeout " << timeoutMS << " ms";
    } else {
        error_ = { error_types::Select, errno };
        G_ERROR << "Selector::select " << ops::get_select_error(errno);
    }

    return ret;
}

bool Selector::add_fd(Event event) {
    assert_in_right_thread("Selector::add_fd ");
    if (event.fd >= FD_SETSIZE || find_fd(event.fd) != _fds.end()) {
        G_INFO << "Selector add " << event.fd << " failed.";
        return false;
    }
    ndfs = ndfs < event.fd ? event.fd : ndfs;
    _fds.push_back(event);
    if (event.canRead()) ++read_size;
    if (event.canWrite()) ++write_size;
    if (event.hasError()) ++error_size;
    G_INFO << "Selector add " << event.fd;
    return true;
}

void Selector::remove_fd(int fd) {
    assert_in_right_thread("Selector::remove_fd ");
    auto iter = find_fd(fd);
    if (iter == _fds.end()) return;
    if (iter->canRead()) --read_size;
    if (iter->canWrite()) --write_size;
    if (iter->hasError()) --error_size;
    *iter = _fds.back();
    _fds.pop_back();
    if (fd == ndfs) {
        ndfs = 0;
        for (auto [f, _] : _fds)
            if (f > ndfs) ndfs = f;
    }
    G_INFO << "Selector remove fd " << fd;
}

void Selector::remove_all() {
    assert_in_right_thread("Selector::remove_all ");
    if (_fds.size() > 0)
        G_WARN << "Selector force remove " << _fds.size() << " fds.";
    _fds.clear();
    read_size = write_size = error_size = ndfs = 0;
}

void Selector::update_fd(Event event) {
    assert_in_right_thread("Selector::update_fd ");
    auto iter = find_fd(event.fd);
    if (iter == _fds.end()) return;
    if (iter->canRead()) --read_size;
    if (iter->canWrite()) --write_size;
    if (iter->hasError()) --error_size;
    *iter = event;
    if (iter->canRead()) ++read_size;
    if (iter->canWrite()) ++write_size;
    if (iter->hasError()) ++error_size;
    G_INFO << "Selector update " << event.fd << " events to " << event.event;
}

void Selector::init_fd_set() {
    FD_ZERO(&_read);
    FD_ZERO(&_write);
    FD_ZERO(&_error);
    for (const auto &event : _fds) {
        if (event.canRead())
            FD_SET(event.fd, &_read);
        if (event.canWrite())
            FD_SET(event.fd, &_write);
        if (event.hasError())
            FD_SET(event.fd, &_error);
    }
}

void Selector::fill_events(EventList &list) {
    for (const auto &event : _fds) {
        Event val { event.fd, Event::NoEvent };
        if (event.canRead() && FD_ISSET(event.fd, &_read))
            val.set_read();
        if (event.canWrite() && FD_ISSET(event.fd, &_write))
            val.set_write();
        if (event.hasError() && FD_ISSET(event.fd, &_error))
            val.set_error();
        if (!val.is_NoEvent()) list.push_back(val);
    }
}

std::vector<Event>::iterator Selector::find_fd(int fd) {
    auto iter = _fds.begin(), end = _fds.end();
    while (iter != end && iter->fd != fd)
        ++iter;
    return iter;
}
