//
// Created by taganyer on 3/14/24.
//

#include "../../LogSystem/SystemLog.hpp"
#include "Net/Acceptor.hpp"
#include "Net/InetAddress.hpp"
#include "Net/error/errors.hpp"

using namespace Net;

using namespace Base;

int Acceptor::ListenMax = 8192;

Acceptor::Acceptor(Socket &&socket) : _socket(std::move(socket)) {
    if (!_socket.tcpListen(ListenMax)) {
        G_FATAL << _socket.fd() << " listen failed: " << ops::get_listen_error(errno);
    } else {
        G_TRACE << "Acceptor " << _socket.fd() << " created.";
    }
}

Acceptor::~Acceptor() {
    G_TRACE << "Acceptor " << _socket.fd() << " close.";
}

Acceptor::Message Acceptor::accept_connection() const {
    InetAddress address {};
    Socket socket = _socket.tcpAccept(address);
    if (!socket) return { {}, address };
    int fd = socket.fd();
    auto link = NetLink::create_NetLinkPtr(std::move(socket));
    G_INFO << "Acceptor create NetLink " << fd;
    return { link, address };
}
