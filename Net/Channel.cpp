//
// Created by taganyer on 24-2-29.
//

#include "Poller.hpp"
#include "Channel.hpp"
#include "ChannelsManger.hpp"
#include "../Base/Log/Log.hpp"
#include "NetLink.hpp"

using namespace Net;

using namespace Base;

const short Channel::NoEvent = 0;

const short Channel::Readable = POLLIN | POLLPRI;

const short Channel::Writeable = POLLOUT;

Channel::ChannelPtr Channel::create_Channel(int fd, const std::shared_ptr<Data> &data,
                                            ChannelsManger &manger) {
    return new Channel(fd, data, manger);
}

void Channel::destroy_Channel(Channel *channel) {
    delete channel;
}

Channel::~Channel() {
    assert(!has_aliveEvent());
    G_TRACE << "Channel " << _fd << " has been destroyed";
}

void Channel::invoke() {
    std::shared_ptr data = _data.lock();
    if (!data) {
        finish_revents();
        remove_this();
        return;
    }
    if ((_revents & POLLHUP) && !(_revents & POLLIN)) {
        G_TRACE << "fd " << _fd << "is hang up and no data to read. close() called";
        data->handle_close(*this);
    }

    if (_revents & (POLLERR | POLLNVAL)) {
        if (_revents & POLLERR) {
            G_ERROR << "fd " << _fd << "error. error() called";
        } else G_ERROR << "fd " << _fd << " is invalid. error() called";
        data->handle_error(*this);
    }

    if (_revents & (POLLIN | POLLPRI | POLLRDHUP))
        data->handle_read(*this);

    if (_revents & POLLOUT)
        data->handle_write(*this);

    _last_active_time = Unix_to_now();
    _manger->put_to_top(this);

}

void Channel::enable_read() {
    _events |= Readable;
    _manger->poller()->update_channel(this);
}

void Channel::enable_write() {
    _events |= Writeable;
    _manger->poller()->update_channel(this);
}

void Channel::disable_read() {
    _events &= ~Readable;
    _manger->poller()->update_channel(this);
}

void Channel::disable_write() {
    _events &= ~Writeable;
    _manger->poller()->update_channel(this);
}

void Channel::set_nonevent() {
    _events = NoEvent;
    _manger->poller()->update_channel(this);
}

void Channel::set_events(short events) {
    _events = events;
    _manger->poller()->update_channel(this);
}

Channel::Channel(int fd, const std::shared_ptr<Data> &data,
                 ChannelsManger &manger) : _fd(fd), _manger(&manger), _data(data) {
    G_TRACE << "Channel " << _fd << "create";
}

bool Channel::timeout(const Time_difference &time) {
    if (time >= _last_active_time) {
        std::shared_ptr data = _data.lock();
        if (!data) return true;
        return data->handle_timeout();
    }
    return false;
}

void Channel::remove_this() {
    _manger->remove_channel(this);
}

void Channel::continue_events() {
    if (!is_nonevent())
        set_nonevent();
    _manger->activeList.push_back(this);
}
