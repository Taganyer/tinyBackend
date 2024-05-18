//
// Created by taganyer on 3/23/24.
//

#include "../Selector.hpp"
#include "Base/Log/Log.hpp"
#include "Net/error/errors.hpp"

using namespace Net;

Selector::~Selector() {
    G_WARN << "Selector force clear " << _fds.size();
    _fds.clear();
    read_size = write_size = error_size = 0;
}

int Selector::get_aliveEvent(int timeoutMS, EventList &list) {
    assert_in_right_thread("Selector::fet_aliveEvent ");
    fd_set read;
    fd_set write;
    fd_set error;

    FD_ZERO(&read);
    FD_ZERO(&write);
    FD_ZERO(&error);
    for (auto event: _fds) {
        if (event.canRead()) FD_SET(event.fd, &read);
        if (event.canWrite()) FD_SET(event.fd, &write);
        if (event.hasError()) FD_SET(event.fd, &error);
    }

    timeval timeout{timeoutMS / 1000, timeoutMS % 1000};

    int ret = ::select((int) _fds.size(),
                       read_size > 0 ? &read : nullptr,
                       write_size > 0 ? &write : nullptr,
                       error_size > 0 ? &error : nullptr,
                       timeoutMS > 0 ? &timeout : nullptr);
    if (likely(ret >= 0)) {
        list.reserve(list.size() + ret);
        G_TRACE << "Selector get " << ret << " events.";
    } else {
        error_ = {error_types::Select, errno};
        G_ERROR << "Selector: " << ops::get_select_error(errno);
    }
    for (auto const &event: _fds) {
        Event val{event.fd, Event::NoEvent};
        if (event.canRead() && FD_ISSET(event.fd, &read))
            val.set_read();
        if (event.canWrite() && FD_ISSET(event.fd, &write))
            val.set_write();
        if (event.hasError() && FD_ISSET(event.fd, &error))
            val.set_error();
        if (!val.is_NoEvent()) list.push_back(val);
    }
    return ret;
}

bool Selector::add_fd(Event event) {
    assert_in_right_thread("Selector::add_fd ");
    if (fd_size() == FD_SETSIZE)
        return false;
    _fds.push_back(event);
    if (event.canRead()) ++read_size;
    if (event.canWrite()) ++write_size;
    if (event.hasError()) ++error_size;
    G_TRACE << "Selector add " << event.fd;
    return true;
}

void Selector::remove_fd(int fd) {
    assert_in_right_thread("Selector::remove_fd ");
    auto iter = _fds.begin();
    auto end = _fds.end();
    while (iter != end && iter->fd != fd)
        ++iter;
    if (iter == end) return;
    if (iter->canRead()) --read_size;
    if (iter->canWrite()) --write_size;
    if (iter->hasError()) --error_size;
    *iter = _fds.back();
    _fds.pop_back();
    G_TRACE << "Selector remove " << fd;
}

void Selector::remove_all() {
    assert_in_right_thread("Selector::remove_all ");
    G_WARN << "Selector force clear " << _fds.size();
    _fds.clear();
    read_size = write_size = error_size = 0;
}

void Selector::update_fd(Event event) {
    assert_in_right_thread("Selector::update_fd ");
    auto iter = _fds.begin();
    auto end = _fds.end();
    while (iter != end && iter->fd != event.fd)
        ++iter;
    if (iter == end) return;
    if (iter->canRead()) --read_size;
    if (iter->canWrite()) --write_size;
    if (iter->hasError()) --error_size;
    *iter = event;
    if (iter->canRead()) ++read_size;
    if (iter->canWrite()) ++write_size;
    if (iter->hasError()) ++error_size;
    G_TRACE << "Selector update " << event.fd << " events to " << event.event;
}
