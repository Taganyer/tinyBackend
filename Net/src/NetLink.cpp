//
// Created by taganyer on 24-3-5.
//

#include "../NetLink.hpp"
#include "Net/Acceptor.hpp"
#include "Net/error/errors.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Net/monitors/Monitor.hpp"
#include "Net/functions/Interface.hpp"

using namespace Net;

using namespace Base;


void NetLink::handle_read(Event* event) {
    if (!_socket->valid()) {
        handle_error({ error_types::UnexpectedShutdown, 0 }, event);
        return;
    }
    iovec vec[2];
    vec[0].iov_base = _input.write_data();
    vec[0].iov_len = _input.continuously_writable();
    vec[1].iov_base = _input.data_begin();
    vec[1].iov_len = _input.writable_len() - _input.continuously_writable();
    auto size = ops::readv(_socket->fd(), vec, 2);
    if (size == 0) {
        G_TRACE << "NetLink " << _socket->fd() << " read to end.";
        event->set_HangUp();
    } else if (size < 0) {
        G_ERROR << _socket->fd() << " handle_read " << ops::get_read_error(errno);
        handle_error({ error_types::Read, errno }, event);
    } else {
        _input.write_advance(size);
        G_TRACE << _socket->fd() << " read " << size << " bytes to readBuf.";
        if (_readFun) _readFun(_input, *_socket);
    }
}

void NetLink::handle_write(Event* event) {
    if (!_socket->valid()) {
        handle_error({ error_types::UnexpectedShutdown, 0 }, event);
        return;
    }
    iovec vec[2];
    vec[0].iov_base = _output.read_data();
    vec[0].iov_len = _input.continuously_readable();
    vec[1].iov_base = _input.data_begin();
    vec[1].iov_len = _input.readable_len() - _input.continuously_readable();
    auto size = ops::writev(_socket->fd(), vec, 2);
    if (size < 0) {
        G_ERROR << _socket->fd() << " handle_write " << ops::get_write_error(errno);
        handle_error({ error_types::Write, errno }, event);
    } else {
        _output.read_advance(size);
        G_TRACE << _socket->fd() << " write " << size << " bytes to " << _socket->fd();
        if (_writeFun) _writeFun(_output, *_socket);
    }
}

void NetLink::handle_error(error_mark mark, Event* event) {
    event->unset_error();
    if (_socket->valid())
        G_ERROR << "error occur in NetLink " << _socket->fd();
    if (!_errorFun || _errorFun(mark, *_socket))
        event->set_HangUp();
}

void NetLink::handle_close() {
    G_TRACE << "NetLink " << _socket->fd() << " close";
    if (_closeFun) _closeFun(*_socket);
    if (_socket->valid()) {
        auto ptr = _acceptor.lock();
        if (ptr) ptr->remove_Link(_socket->fd());
    }
}

bool NetLink::handle_timeout() {
    G_WARN << "NetLink " << _socket->fd() << " timeout.";
    if (!_errorFun || _errorFun({ error_types::TimeoutEvent, 0 }, *_socket)) {
        G_TRACE << "NetLink " << _socket->fd() << " close";
        if (_closeFun) _closeFun(*_socket);
        return true;
    }
    return false;
}

NetLink::LinkPtr NetLink::create_NetLinkPtr(
    SocketPtr &&socket_ptr, const AcceptorPtr &acceptor_ptr) {
    return LinkPtr(new NetLink(std::move(socket_ptr), acceptor_ptr));
}

NetLink::NetLink(SocketPtr &&socket_ptr, const AcceptorPtr &acceptor_ptr) :
    _socket(std::move(socket_ptr)), _acceptor(acceptor_ptr) {}

NetLink::~NetLink() {
    close_fd();
}

void NetLink::close_fd() {
    if (_socket->valid()) {
        G_INFO << "NetLink " << _socket->fd() << " closed.";
        _socket = std::make_unique<Socket>(-1);
    }
}
