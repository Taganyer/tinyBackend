//
// Created by taganyer on 24-3-5.
//

#include "NetLink.hpp"
#include "Channel.hpp"
#include "ChannelsManger.hpp"
#include "EventLoop.hpp"
#include "functions/Interface.hpp"
#include "functions/errors.hpp"

using namespace Net;

using namespace Net::Detail;

using namespace Base;


void LinkData::handle_read() {
    int revents = _channel->revents();
    _channel->set_revents(revents & ~(Channel::Read | Channel::Urgent));

    auto len = _input.continuously_writable();
    auto size = ops::read(FD->fd(), _input.write_data(), len);
    if (size < 0) {
        G_ERROR << FD->fd() << " handle_read " << ops::get_read_error(errno);
        if (_errorFun({error_types::Read, errno}))
            handle_close();
        return;
    }
    _input.write_advance(size);
    if (size == len) {
        len = _input.continuously_writable();
        auto s = ops::read(FD->fd(), _input.write_data(), len);
        if (s < 0) {
            G_ERROR << FD->fd() << " handle_read " << ops::get_read_error(errno);
            if (_errorFun({error_types::Read, errno}))
                handle_close();
        } else {
            _input.write_advance(s);
            size += s;
        }
    }
    G_TRACE << FD->fd() << " read " << size << " bytes";

    if (_readFun) _readFun(_input);
}

void LinkData::handle_write() {
    int revents = _channel->revents();
    _channel->set_revents(revents & ~Channel::Write);

    auto len = _output.continuously_readable();
    auto size = ops::write(FD->fd(), _output.read_data(), len);
    if (size < 0) {
        G_ERROR << FD->fd() << " handle_write " << ops::get_write_error(errno);
        if (_errorFun({error_types::Write, errno}))
            handle_close();
        return;
    }
    _output.read_advance(size);
    if (size == len) {
        len = _output.continuously_readable();
        auto s = ops::write(FD->fd(), _output.read_data(), len);
        if (s < 0) {
            G_ERROR << FD->fd() << " handle_write " << ops::get_write_error(errno);
            if (_errorFun({error_types::Read, errno}))
                handle_close();
            return;
        } else {
            _output.read_advance(s);
            size += s;
        }
    }
    G_TRACE << FD->fd() << " write " << size << " bytes";
    if (_writeFun) _writeFun(_output);
}

bool LinkData::handle_error() {
    int revents = _channel->revents();
    _channel->set_revents(revents & ~(Channel::Error | Channel::Invalid));

    G_ERROR << "error occur in Link " << FD->fd();
    if (_errorFun) _errorFun(_channel->errorMark);
}

void LinkData::handle_close() {
    /// FIXME
    if (_channel->is_nonevent())
        _channel->set_nonevent();
    _channel->set_revents(Channel::NoEvent);
    G_TRACE << "Link " << FD->fd() << " close";
    if (_closeFun) _closeFun();
    _channel->remove_this();
    _channel = nullptr;
}

bool LinkData::handle_timeout() {
    G_WARN << "Link " << FD->fd() << " timeout";
    if (_errorFun({error_types::Link_TimeoutEvent, _channel->revents()})) {
        if (_channel->is_nonevent())
            _channel->set_nonevent();
        _channel->set_revents(Channel::NoEvent);
        G_TRACE << "Link " << FD->fd() << " timeout close";
        if (_closeFun) _closeFun();
        _channel = nullptr;
        return true;
    }
    return false;
}


NetLink::NetLink(FdPtr &&Fd) :
        _data(std::make_shared<LinkData>(std::move(Fd))) {}

NetLink::~NetLink() {
    force_close_link();
}

uint32 NetLink::send(const void *target, uint32 size) {
    int64 writen = 0;
    if (in_loop_thread() && _data->_output.empty()) {
        writen = ops::write(fd(), target, size);
        if (writen < 0) {
            G_ERROR << fd() << " NetLink " << ops::get_write_error(errno);
            if (_data->_errorFun({error_types::Write, errno})) {
                _data->handle_close();
                return 0;
            }
            writen = 0;
        } else {
            G_TRACE << "write " << writen << " byte in " << fd();
            if (_data->_writeFun) _data->_writeFun(_data->_output);
        }
    }
    if (writen < size) {
        writen += _data->_output.write((const char *) target + writen, size - writen);
    }
    return writen;
}

uint64 NetLink::send_file() {
    return 0;
}

void NetLink::channel_read(bool turn_on) {
    if (in_loop_thread() && _data->_channel) {
        _data->_channel->set_readable(turn_on);
        return;
    }
    send_to_loop([turn_on, ptr = std::weak_ptr(_data)] {
        auto data = ptr.lock();
        if (data && data->_channel) {
            data->_channel->set_readable(turn_on);
        }
    });
}

void NetLink::channel_write(bool turn_on) {
    if (in_loop_thread() && _data->_channel) {
        _data->_channel->set_writable(turn_on);
        return;
    }
    send_to_loop([turn_on, ptr = std::weak_ptr(_data)] {
        auto data = ptr.lock();
        if (data && data->_channel) {
            data->_channel->set_writable(turn_on);
        }
    });
}

void NetLink::read_event(bool turn_on) {
    if (in_loop_thread() && _data->_channel) {
        int revents = _data->_channel->revents();
        if (turn_on) _data->_channel->set_revents(revents | Channel::Read);
        else _data->_channel->set_revents(revents & ~Channel::Read);
        return;
    }
    send_to_loop([turn_on, ptr = std::weak_ptr(_data)] {
        auto data = ptr.lock();
        if (data && data->_channel) {
            int revents = data->_channel->revents();
            if (turn_on) data->_channel->set_revents(revents | Channel::Read);
            else data->_channel->set_revents(revents & ~Channel::Read);
        }
    });
}

void NetLink::write_event(bool turn_on) {
    if (in_loop_thread() && _data->_channel) {
        int revents = _data->_channel->revents();
        if (turn_on) _data->_channel->set_revents(revents | Channel::Write);
        else _data->_channel->set_revents(revents & ~Channel::Write);
        return;
    }
    send_to_loop([turn_on, ptr = std::weak_ptr(_data)] {
        auto data = ptr.lock();
        if (data && data->_channel) {
            int revents = data->_channel->revents();
            if (turn_on) data->_channel->set_revents(revents | Channel::Write);
            else data->_channel->set_revents(revents & ~Channel::Write);
        }
    });
}

void NetLink::error_event(bool turn_on) {
    if (in_loop_thread() && _data->_channel) {
        int revents = _data->_channel->revents();
        if (turn_on) _data->_channel->set_revents(revents | Channel::Error);
        else _data->_channel->set_revents(revents & ~Channel::Error);
        return;
    }
    send_to_loop([turn_on, ptr = std::weak_ptr(_data)] {
        auto data = ptr.lock();
        if (data && data->_channel) {
            int revents = data->_channel->revents();
            if (turn_on) data->_channel->set_revents(revents | Channel::Error);
            else data->_channel->set_revents(revents & ~Channel::Error);
        }
    });
}

void NetLink::close_event(bool turn_on) {
    if (in_loop_thread() && _data->_channel) {
        int revents = _data->_channel->revents();
        if (turn_on) _data->_channel->set_revents(revents | Channel::Close);
        else _data->_channel->set_revents(revents & ~Channel::Close);
        return;
    }
    send_to_loop([turn_on, ptr = std::weak_ptr(_data)] {
        auto data = ptr.lock();
        if (data && data->_channel) {
            int revents = data->_channel->revents();
            if (turn_on) data->_channel->set_revents(revents | Channel::Close);
            else data->_channel->set_revents(revents & ~Channel::Close);
        }
    });
}

void NetLink::wake_up_event() {
    if (!_data->_channel->has_aliveEvent())
        return;
    if (in_loop_thread() && _data->_channel) {
        _data->_channel->send_to_next_loop();
        return;
    }
    send_to_loop([ptr = std::weak_ptr(_data)] {
        auto data = ptr.lock();
        if (data && data->_channel)
            data->_channel->send_to_next_loop();
    });
}

void NetLink::force_close_link() {
    _data = nullptr;
}

void NetLink::send_to_loop(std::function<void()> event) {
    _data->_channel->manger()->loop()->put_event(std::move(event));
}

bool NetLink::in_loop_thread() const {
    return _data->_channel->manger()->tid() == Base::tid();
}
