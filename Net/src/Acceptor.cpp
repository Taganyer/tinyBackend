//
// Created by taganyer on 3/14/24.
//

#include "Base/Log/Log.hpp"
#include "../Acceptor.hpp"
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
    if (_links.size() > 0)
        G_WARN << "Acceptor will force close " << _links.size() << " linkes.";
}

Acceptor::Message Acceptor::accept_connection(bool NoDelay) {
    InetAddress address {};
    int fd = _socket.tcpAccept(address);
    if (fd < 0) return { {}, address };
    Lock l(_mutex);
    auto ptr = std::make_unique<Socket>(fd);
    ptr->setTcpNoDelay(NoDelay);
    auto link = NetLink::create_NetLinkPtr(std::move(ptr));
    assert(_links.emplace(fd, link).second && link->fileDescriptor());
    G_INFO << "Acceptor add NetLink " << fd;
    return { link, address };
}

void Acceptor::remove_Link(int fd) {
    Lock l(_mutex);
    if (_links.erase(fd))
        G_INFO << "Acceptor remove NetLink " << fd;
}
