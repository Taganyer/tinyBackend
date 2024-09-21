//
// Created by taganyer on 24-2-29.
//

#include "Poller.hpp"
#include "Channel.hpp"

#include "ChannelsManger.hpp"
#include "../NetLink.hpp"
#include "../../Base/Log/Log.hpp"

using namespace Net;

using namespace Base;

const short Channel::NoEvent = 0;

const short Channel::Read = POLLIN;

const short Channel::Urgent = POLLPRI;

const short Channel::Write = POLLOUT;

const short Channel::Error = POLLERR;

const short Channel::Invalid = POLLNVAL;

const short Channel::Close = POLLHUP;

Channel::ChannelPtr Channel::create_Channel(const SharedData &data,
                                            ChannelsManger &manger) {
    if (data->FD && *data->FD) {
        auto channel = new Channel(data, manger);
        data->_channel = channel;
        return channel;
    }
    return nullptr;
}

void Channel::destroy_Channel(Channel *channel) {
    delete channel;
}

Channel::~Channel() {
    G_TRACE << "Channel " << _fd << " has been destroyed";
}

void Channel::invoke() {
    std::shared_ptr data = _data.lock();
    if (!data) {
        _revents = NoEvent;
        remove_this();
        return;
    }

    if (_revents & (Error | Invalid)) {
        G_ERROR << "fd " << _fd << " error. error() called";
        data->handle_error(errorMark);
    }

    if ((_revents & Close) && !(_revents & Read)) {
        G_TRACE << "fd " << _fd << " is hang up and no data to read. close() called";
        data->handle_close();
    }

    if (_revents & (Read | Urgent | Close))
        data->handle_read();

    if (_revents & Write)
        data->handle_write();

    if (data->_channel) {
        _last_active_time = Unix_to_now();
        _manger->put_to_top(this);
    }

}

void Channel::set_readable(bool turn_on) {
    if (turn_on) _events |= Read;
    else _events &= ~Read;
    _manger->poller()->update_channel(this);
}

void Channel::set_writable(bool turn_on) {
    if (turn_on)_events |= Write;
    else _events &= ~Write;
    _manger->poller()->update_channel(this);
}

void Channel::set_nonevent() {
    _events = NoEvent;
    _manger->poller()->update_channel(this);
}

Channel::Channel(const SharedData &data, ChannelsManger &manger) :
        _fd(data->fd()), _manger(&manger), _data(data) {
    data->_channel = this;
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

void Channel::send_to_next_loop() {
    _manger->activeList.push_back(this);
}
