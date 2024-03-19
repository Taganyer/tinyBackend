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

    if (!FD) {
        handle_error({error_types::Link_UnexpectedShutdown, -1});
        return;
    }
    struct iovec vec[2];
    vec[0].iov_base = _input.write_data();
    vec[0].iov_len = _input.continuously_writable();
    vec[1].iov_base = _input.data_begin();
    vec[1].iov_len = _input.writable_len() - _input.continuously_writable();
    auto size = ops::readv(FD->fd(), vec, 2);
    if (size == 0) {
        G_TRACE << "fd " << FD->fd() << " read to end.";
        handle_close();
    } else if (size < 0) {
        G_ERROR << FD->fd() << " handle_read " << ops::get_read_error(errno);
        handle_error({error_types::Read, errno});
    } else {
        _input.write_advance(size);
        G_TRACE << FD->fd() << " read " << size << " bytes in readBuf";
        if (_readFun) _readFun(_input, *FD);
    }
}

void NetLink::handle_write() {
    int revents = _channel->revents();
    _channel->set_revents(revents & ~Channel::Write);

    if (!FD) {
        handle_error({error_types::Link_UnexpectedShutdown, -1});
        return;
    }
    struct iovec vec[2];
    vec[0].iov_base = _output.read_data();
    vec[0].iov_len = _input.continuously_readable();
    vec[1].iov_base = _input.data_begin();
    vec[1].iov_len = _input.readable_len() - _input.continuously_readable();
    auto size = ops::writev(FD->fd(), vec, 2);
    if (size < 0) {
        G_ERROR << FD->fd() << " handle_write " << ops::get_write_error(errno);
        handle_error({error_types::Write, errno});
        return;
    } else {
        _output.read_advance(size);
        G_TRACE << FD->fd() << " write " << size << " bytes in writeBuf";
        if (_writeFun) _writeFun(_output, *FD);
    }
}

void NetLink::handle_error(error_mark mark) {
    int revents = _channel->revents();
    _channel->set_revents(revents & ~(Channel::Error | Channel::Invalid));

    G_ERROR << "error occur in Link " << FD->fd();
    if (_errorFun && _errorFun(mark, *FD))
        handle_close();
}

void NetLink::handle_close() {
    if (!_channel->is_nonevent())
        _channel->set_nonevent();
    _channel->set_revents(Channel::NoEvent);
    G_TRACE << "Link " << FD->fd() << " close";
    if (_closeFun) _closeFun(*FD);
    _channel->remove_this();
    _channel = nullptr;
}

bool NetLink::handle_timeout() {
    G_WARN << "Link " << FD->fd() << " timeout";
    if (_errorFun({error_types::Link_TimeoutEvent, _channel->revents()}, *FD)) {
        if (!_channel->is_nonevent())
            _channel->set_nonevent();
        _channel->set_revents(Channel::NoEvent);
        G_TRACE << "Link " << FD->fd() << " close";
        if (_closeFun) _closeFun(*FD);
        _channel = nullptr;
        return true;
    }
    return false;
}


NetLink::LinkPtr NetLink::create_NetLinkPtr(NetLink::FdPtr &&Fd) {
    return std::shared_ptr<NetLink>(new NetLink(std::move(Fd)));
}

NetLink::NetLink(FdPtr &&Fd) : FD(std::move(Fd)) {}

NetLink::~NetLink() {
    close_fd();
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
    if (!valid())
        return;
    if (in_loop_thread()) {
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

void NetLink::close_fd() {
    FD.reset();
}

void NetLink::send_to_loop(std::function<void()> event) {
    _channel->manger()->loop()->put_event(std::move(event));
}

Controller NetLink::get_controller() {
    return shared_from_this();
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
            ptr->handle_error({error_types::Write, errno});
            return -1;
        } else {
            G_TRACE << "write " << writen << " byte in " << ptr->fd();
            if (ptr->_writeFun) ptr->_writeFun(ptr->_output, *ptr->FD);
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

bool Controller::wake_readCallback() {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (ptr->in_loop_thread()) {
        ptr->_readFun(ptr->_input, *ptr->FD);
    } else {
        ptr->send_to_loop([weak = _weak] {
            auto data = weak.lock();
            if (data && data->_channel) {
                data->_readFun(data->_input, *data->FD);
            }
        });
    }
    return true;
}

bool Controller::wake_writeCallback() {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (ptr->in_loop_thread()) {
        ptr->_writeFun(ptr->_output, *ptr->FD);
    } else {
        ptr->send_to_loop([weak = _weak] {
            auto data = weak.lock();
            if (data && data->_channel) {
                data->_writeFun(data->_output, *data->FD);
            }
        });
    }
    return true;
}

bool Controller::wake_error(error_mark mark) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (ptr->in_loop_thread()) {
        ptr->handle_error(mark);
    } else {
        ptr->send_to_loop([weak = _weak, mark] {
            auto data = weak.lock();
            if (data && data->_channel)
                data->handle_error(mark);
        });
    }
    return true;
}

bool Controller::wake_close() {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (ptr->in_loop_thread()) {
        ptr->handle_close();
    } else {
        ptr->send_to_loop([weak = _weak] {
            auto data = weak.lock();
            if (data && data->_channel)
                data->handle_close();
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

bool Controller::set_channelRevents(int revents) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (ptr->in_loop_thread()) {
        ptr->_channel->set_revents(revents);
    } else {
        ptr->send_to_loop([weak = _weak, revents] {
            auto data = weak.lock();
            if (data && data->_channel)
                data->_channel->set_revents(revents);
        });
    }
    return true;
}

void Controller::close_fd() {
    Shared ptr = _weak.lock();
    if (!ptr) return;
    ptr->close_fd();
}

int Controller::get_channelRevents() const {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return 0;
    return ptr->_channel->revents();
}
