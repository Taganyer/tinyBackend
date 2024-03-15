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

using namespace Base;


void NetLink::handle_read() {
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

void NetLink::handle_write() {
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
            if (_errorFun({error_types::Write, errno}))
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

bool NetLink::handle_error() {
    int revents = _channel->revents();
    _channel->set_revents(revents & ~(Channel::Error | Channel::Invalid));

    G_ERROR << "error occur in Link " << FD->fd();
    if (_errorFun) return _errorFun(_channel->errorMark);
    return false;
}

void NetLink::handle_close() {
    if (_channel->is_nonevent())
        _channel->set_nonevent();
    _channel->set_revents(Channel::NoEvent);
    G_TRACE << "Link " << FD->fd() << " close";
    if (_closeFun) _closeFun();
    _channel->remove_this();
    _channel = nullptr;
}

bool NetLink::handle_timeout() {
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

NetLink::LinkPtr NetLink::create_NetLinkPtr(NetLink::FdPtr &&Fd) {
    return std::shared_ptr<NetLink>(new NetLink(std::move(Fd)));
}

NetLink::~NetLink() {
    force_close_link();
}

void NetLink::channel_read(bool turn_on) {
    if (in_loop_thread() && valid()) {
        _channel->set_readable(turn_on);
    } else {
        send_to_loop([turn_on, ptr = weak_from_this()] {
            auto data = ptr.lock();
            if (data && data->_channel) {
                data->_channel->set_readable(turn_on);
            }
        });
    }
}

void NetLink::channel_write(bool turn_on) {
    if (in_loop_thread() && valid()) {
        _channel->set_writable(turn_on);
    } else {
        send_to_loop([turn_on, ptr = weak_from_this()] {
            auto data = ptr.lock();
            if (data && data->_channel) {
                data->_channel->set_writable(turn_on);
            }
        });
    }
}

void NetLink::wake_up_event() {
    if (!valid())
        return;
    if (in_loop_thread()) {
        _channel->send_to_next_loop();
    } else {
        send_to_loop([ptr = weak_from_this()] {
            auto data = ptr.lock();
            if (data && data->_channel)
                data->_channel->send_to_next_loop();
        });
    }
}

void NetLink::force_close_link() {
    FD.reset();
}

void NetLink::send_to_loop(std::function<void()> event) {
    _channel->manger()->loop()->put_event(std::move(event));
}

bool NetLink::in_loop_thread() const {
    return _channel->manger()->tid() == Base::tid();
}


Controller::Controller(const Controller::Shared &ptr) : _weak(ptr) {}

uint32 Controller::send(const void *target, uint32 size) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return -1;
    int64 writen = 0;
    if (ptr->in_loop_thread() && ptr->_output.empty()) {
        writen = ops::write(ptr->fd(), target, size);
        if (writen < 0) {
            G_ERROR << ptr->fd() << " NetLink " << ops::get_write_error(errno);
            if (ptr->_errorFun({error_types::Write, errno})) {
                ptr->handle_close();
                return -1;
            }
            writen = 0;
        } else {
            G_TRACE << "write " << writen << " byte in " << ptr->fd();
            if (ptr->_writeFun) ptr->_writeFun(ptr->_output);
        }
    }
    if (writen < size) {
        writen += ptr->_output.write((const char *) target + writen, size - writen);
    }
    return writen;
}

uint64 Controller::send_file() {
    return 0;
}

bool Controller::reset_readCallback(Controller::ReadCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    ptr->set_readCallback(std::move(event));
    return true;
}

bool Controller::reset_writeCallback(Controller::WriteCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    ptr->set_writeCallback(std::move(event));
    return true;
}

bool Controller::reset_errorCallback(Controller::ErrorCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    ptr->set_errorCallback(std::move(event));
    return true;
}

bool Controller::reset_closeCallback(Controller::CloseCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    ptr->set_closeCallback(std::move(event));
    return true;
}

bool Controller::channel_read(bool turn_on) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    ptr->channel_read(turn_on);
    return true;
}

bool Controller::channel_write(bool turn_on) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    ptr->channel_write(turn_on);
    return true;
}

bool Controller::read_event(bool turn_on) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (ptr->in_loop_thread() && ptr->_channel) {
        int revents = ptr->_channel->revents();
        if (turn_on) ptr->_channel->set_revents(revents | Channel::Read);
        else ptr->_channel->set_revents(revents & ~Channel::Read);
    } else {
        ptr->send_to_loop([turn_on, weak = _weak] {
            auto data = weak.lock();
            if (data && data->_channel) {
                int revents = data->_channel->revents();
                if (turn_on) data->_channel->set_revents(revents | Channel::Read);
                else data->_channel->set_revents(revents & ~Channel::Read);
            }
        });
    }
    return true;
}

bool Controller::write_event(bool turn_on) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (ptr->in_loop_thread() && ptr->_channel) {
        int revents = ptr->_channel->revents();
        if (turn_on) ptr->_channel->set_revents(revents | Channel::Write);
        else ptr->_channel->set_revents(revents & ~Channel::Write);
    } else {
        ptr->send_to_loop([turn_on, weak = _weak] {
            auto data = weak.lock();
            if (data && data->_channel) {
                int revents = data->_channel->revents();
                if (turn_on) data->_channel->set_revents(revents | Channel::Write);
                else data->_channel->set_revents(revents & ~Channel::Write);
            }
        });
    }
    return true;
}

bool Controller::error_event(bool turn_on) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (ptr->in_loop_thread() && ptr->_channel) {
        int revents = ptr->_channel->revents();
        if (turn_on) ptr->_channel->set_revents(revents | Channel::Error);
        else ptr->_channel->set_revents(revents & ~Channel::Error);
    } else {
        ptr->send_to_loop([turn_on, weak = _weak] {
            auto data = weak.lock();
            if (data && data->_channel) {
                int revents = data->_channel->revents();
                if (turn_on) data->_channel->set_revents(revents | Channel::Error);
                else data->_channel->set_revents(revents & ~Channel::Error);
            }
        });
    }
    return true;
}

bool Controller::close_event(bool turn_on) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (ptr->in_loop_thread() && ptr->_channel) {
        int revents = ptr->_channel->revents();
        if (turn_on) ptr->_channel->set_revents(revents | Channel::Close);
        else ptr->_channel->set_revents(revents & ~Channel::Close);
    } else {
        ptr->send_to_loop([turn_on, weak = _weak] {
            auto data = weak.lock();
            if (data && data->_channel) {
                int revents = data->_channel->revents();
                if (turn_on) data->_channel->set_revents(revents | Channel::Close);
                else data->_channel->set_revents(revents & ~Channel::Close);
            }
        });
    }
    return true;
}

bool Controller::wake_up_event() {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    ptr->wake_up_event();
    return true;
}
