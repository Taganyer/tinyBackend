//
// Created by taganyer on 3/14/24.
//

#include "Base/Log/SystemLog.hpp"
#include "Net/Acceptor.hpp"
#include "Net/InetAddress.hpp"
#include "Net/error/errors.hpp"

using namespace Net;

using namespace Base;

int Acceptor::ListenMax = 8192;

Acceptor::AcceptorPtr Acceptor::create_AcceptorPtr(Socket &&socket) {
    return AcceptorPtr(new Acceptor(std::move(socket)));
}

Acceptor::~Acceptor() {
    G_TRACE << "Acceptor " << _socket.fd() << " close.";
    if (_links.size() > 0) {
        for (const auto &[_, ptr] : _links) ptr->_acceptor = WeakPtr();
        G_WARN << "Acceptor will force close " << _links.size() << " linkes.";
    }
}

Acceptor::Message Acceptor::accept_connection(bool NoDelay) {
    InetAddress address {};
    int fd = _socket.tcpAccept(address);
    if (fd < 0) return { {}, address };
    Lock l(_mutex);
    auto ptr = std::make_unique<Socket>(fd);
    ptr->setTcpNoDelay(NoDelay);
    auto link = NetLink::create_NetLinkPtr(std::move(ptr), shared_from_this());
    auto [iter, success] = _links.try_emplace(fd, link);
    assert(success && link->socket());
    G_INFO << "Acceptor add NetLink " << fd;
    return { link, address };
}

bool Acceptor::add_LinkPtr(NetLink::LinkPtr link_ptr) {
    if (!link_ptr->valid() || link_ptr->_acceptor.lock()) return false;
    Lock l(_mutex);
    auto [iter, success] = _links.try_emplace(link_ptr->fd(), link_ptr);
    if (success) iter->second->_acceptor = weak_from_this();
    return success;
}

NetLink::LinkPtr Acceptor::get_LinkPtr(int fd) {
    Lock l(_mutex);
    auto iter = _links.find(fd);
    if (iter == _links.end())
        return {};
    return iter->second;
}

void Acceptor::remove_Link(int fd) {
    Lock l(_mutex);
    auto iter = _links.find(fd);
    if (iter == _links.end()) return;
    iter->second->_acceptor = WeakPtr();
    _links.erase(iter);
    G_INFO << "Acceptor remove NetLink " << fd;
}

Acceptor::Acceptor(Socket &&socket) : _socket(std::move(socket)) {
    if (!_socket.tcpListen(ListenMax)) {
        G_FATAL << _socket.fd() << " listen failed: " << ops::get_listen_error(errno);
    } else {
        G_TRACE << "Acceptor " << _socket.fd() << " created.";
    }
}
