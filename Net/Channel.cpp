//
// Created by taganyer on 24-2-29.
//

#include <poll.h>
#include "Poller.hpp"
#include "Channel.hpp"
#include "../Base/Log/Log.hpp"

using namespace Net;

using namespace Base;

const short Channel::NoEvent = 0;

const short Channel::Readable = POLLIN | POLLPRI;

const short Channel::Writeable = POLLOUT;

Channel::ChannelPtr Channel::create_Channel(int fd) {
    return ChannelPtr(new Channel(fd));
}

Channel::~Channel() {
    assert(!has_event());
    G_TRACE << "Channel " << _fd << " has been destroyed";
}

void Channel::invoke() {
    if ((_revents & POLLHUP) && !(_revents & POLLIN)) {
        G_TRACE << "fd " << _fd << "is hang up and no data to read. close() called";
        if (closeCallback) closeCallback();
    }

    if (_revents & (POLLERR | POLLNVAL)) {
        if (_revents & POLLERR)
            G_ERROR << "fd " << _fd << "error. error() called";
            else G_ERROR << "fd " << _fd << " is invalid. error() called";
        if (errorCallback) errorCallback();
    }

    if (_revents & (POLLIN | POLLPRI | POLLRDHUP) && readCallback)
        readCallback();

    if (_revents & POLLOUT && writeCallback)
        writeCallback();

    _revents = NoEvent;
}

void Channel::enable_read() {
    _events |= Readable;
    _poller->update_channel(this);
}

void Channel::enable_write() {
    _events |= Writeable;
    _poller->update_channel(this);
}

void Channel::disable_read() {
    _events &= ~Readable;
    _poller->update_channel(this);
}

void Channel::disable_write() {
    _events &= ~Writeable;
    _poller->update_channel(this);
}

void Channel::set_nonevent() {
    _events = NoEvent;
    _poller->update_channel(this);
}

Channel::Channel(int fd) : _fd(fd) {
    G_TRACE << "Channel " << _fd << "create";
}
