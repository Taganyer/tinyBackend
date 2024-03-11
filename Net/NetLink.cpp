//
// Created by taganyer on 24-3-5.
//

#include "NetLink.hpp"
#include "Channel.hpp"
#include "ChannelsManger.hpp"
#include "EventLoop.hpp"
#include "../Base/Log/Log.hpp"
#include "functions/Interface.hpp"
#include "functions/errors.hpp"

using namespace Net;

using namespace Net::Detail;

using namespace Base;


LinkData::~LinkData() {

}

void LinkData::handle_read() {
    auto len = _input.continuously_writable();
    auto size = ops::read(socket.fd(), _input.write_data(), len);
    if (size < 0) {
        G_ERROR << socket.fd() << " handle_read " << ops::get_read_error(errno);
        _errorFun({error_types::Read, errno});
        return;
    }
    _input.write_advance(size);
    if (size == len) {
        len = _input.continuously_writable();
        auto s = ops::read(socket.fd(), _input.write_data(), len);
        if (s < 0) {
            G_ERROR << socket.fd() << " handle_read " << ops::get_read_error(errno);
            _errorFun({error_types::Read, errno});
        } else {
            _input.write_advance(s);
            size += s;
        }
    }
    G_TRACE << socket.fd() << " read " << size << " bytes";
}

void LinkData::handle_write() {
    auto len = _output.continuously_readable();
    auto size = ops::write(socket.fd(), _output.read_data(), len);
    if (size < 0) {
        G_ERROR << socket.fd() << " handle_write " << ops::get_write_error(errno);
        _errorFun({error_types::Write, errno});
        return;
    }
    _output.read_advance(size);
    if (size == len) {
        len = _output.continuously_readable();
        auto s = ops::write(socket.fd(), _output.read_data(), len);
        if (s < 0) {
            G_ERROR << socket.fd() << " handle_write " << ops::get_write_error(errno);
            _errorFun({error_types::Write, errno});
        } else {
            _output.read_advance(s);
            size += s;
        }
    }
    G_TRACE << socket.fd() << " write " << size << " bytes";
}

void LinkData::handle_error() {
    _errorFun({})
}

void LinkData::handle_close() {

}

bool LinkData::handle_timeout() {
    return false;
}

NetLink::NetLink(int fd, EventLoop *loop) {

}

NetLink::~NetLink() {
    force_close();
}

uint32 NetLink::send(const void *target, uint32 size) {
    int64 writen = 0;
    if (in_loop_thread() && _data->_output.empty()) {
        writen = ops::write(fd(), target, size);
        if (writen < 0) {
            G_ERROR << fd() << " NetLink " << ops::get_write_error(errno);
            _data->_errorFun({error_types::Write, errno});
            writen = 0;
        } else {
            G_TRACE << "write " << writen << " byte in " << fd();
        }
    }
    if (writen < size) {
        writen += _data->_output.write((const char *) target + writen, size - writen);
    }
    return writen;
}

void NetLink::shutdown_write() {
    send_to_loop([ptr = std::weak_ptr(_data)] {
        auto data = ptr.lock();
        data->socket.shutdown_write();
    });
}

void NetLink::force_close() {
    _data = nullptr;
}

void NetLink::set_channel_read(bool turn_on) {
    send_to_loop([turn_on, ptr = std::weak_ptr(_data)] {
        auto data = ptr.lock();
        if (data && data->_channel) {
            data->_channel->set_read(turn_on);
        }
    });
}

void NetLink::set_channel_write(bool turn_on) {
    send_to_loop([turn_on, ptr = std::weak_ptr(_data)] {
        auto data = ptr.lock();
        if (data && data->_channel) {
            data->_channel->set_write(turn_on);
        }
    });
}

void NetLink::send_to_loop(std::function<void()> event) {
    if (in_loop_thread()) event();
    else _data->_channel->manger()->loop()->put_event(std::move(event));
}

bool NetLink::in_loop_thread() const {
    return _data->_channel->manger()->tid() == Base::tid();
}


