//
// Created by taganyer on 3/14/24.
//

#include "../Acceptor.hpp"
#include "Net/InetAddress.hpp"
#include "Net/error/errors.hpp"

using namespace Net;

int Acceptor::ListenMax = 8192;

const int Acceptor::Accepting = -1;

const int Acceptor::Ready = 0;


Acceptor::Acceptor(Socket &&socket) : _socket(std::move(socket)) {}

bool Acceptor::start_listen() {
    if (_socket.tcpListen(ListenMax))
        return true;
    _state = -errno;
    return false;
}

int64 Acceptor::accept_connections(int64 times) {
    _state = Accepting;
    if (times < 0) {
        while (accepting()) {
            if (!accept())
                return -1;
        }
        _state = Ready;
        return 0;
    } else {
        int64 connections = 0;
        while (accepting() && connections < times) {
            if (!accept()) return connections;
            ++connections;
        }
        _state = Ready;
        return connections;
    }
}

void Acceptor::Async_stop_accept() {
    _state = Ready;
}

error_mark Acceptor::get_error() const {
    if (_state == Accepting || _state == Ready)
        return {error_types::Null, 0};
    if (_state < 0) return {error_types::Listen, -_state};
    else return {error_types::Accept, _state};
}

bool Acceptor::accept() {
    InetAddress inetAddress{};
    int fd = _socket.tcpAccept(inetAddress);
    if (fd < 0) {
        _state = errno;
        return false;
    }
    _accept(fd, inetAddress);
    return true;
}
