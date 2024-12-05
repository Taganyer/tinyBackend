//
// Created by taganyer on 3/14/24.
//

#include "../../LogSystem/SystemLog.hpp"
#include "Net/Acceptor.hpp"
#include "Net/InetAddress.hpp"

using namespace Net;

using namespace Base;

int Acceptor::ListenMax = 512;

Acceptor::Acceptor(bool Ipv4, unsigned short target_port, const char* target_ip):
    Acceptor(Ipv4, InetAddress(Ipv4, target_ip ? target_ip : Ipv4 ? "0.0.0.0" : "::", target_port)) {}

Acceptor::Acceptor(bool Ipv4, const InetAddress &target) :
    _socket(Ipv4 ? AF_INET : AF_INET6, SOCK_STREAM) {
    if (!_socket.setReuseAddr(true)) {
        G_ERROR << "Acceptor error setting reuse address on socket";
    }
    if (_socket.bind(target) && _socket.tcpListen(ListenMax)) {
        G_TRACE << "Acceptor " << _socket.fd() << " created.";
    } else {
        G_FATAL << "Acceptor " << _socket.fd() << " create failed.";
    }
}

Acceptor::Acceptor(Socket &&socket) : _socket(std::move(socket)) {
    if (!_socket.setReuseAddr(true)) {
        G_ERROR << "Acceptor error setting reuse address on socket";
    }
    if (_socket.tcpListen(ListenMax)) {
        G_TRACE << "Acceptor " << _socket.fd() << " created.";
    } else {
        G_FATAL << "Acceptor " << _socket.fd() << " create failed.";
    }
}

Acceptor::~Acceptor() {
    G_TRACE << "Acceptor " << _socket.fd() << " close.";
}

Acceptor::Message Acceptor::accept_connection() const {
    InetAddress address {};
    Socket socket = _socket.tcpAccept(address);
    if (socket) {
        G_INFO << "Acceptor create connection " << socket.fd();
    }
    return { std::move(socket), address };
}
