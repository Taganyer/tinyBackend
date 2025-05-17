//
// Created by taganyer on 3/14/24.
//

#include "Base/SystemLog.hpp"
#include "Net/Acceptor.hpp"
#include "Net/InetAddress.hpp"

using namespace Net;

using namespace Base;

#define CHECK(expr, error_handle) \
if (unlikely(!(expr))) { G_ERROR << "Acceptor: " #expr " failed in " << __FUNCTION__; error_handle; }

int Acceptor::ListenMax = 512;

Acceptor::Acceptor(bool Ipv4, unsigned short target_port, const char* target_ip):
    Acceptor(InetAddress(Ipv4, target_ip ? target_ip : Ipv4 ? "0.0.0.0" : "::", target_port)) {}

Acceptor::Acceptor(const InetAddress &target) :
    _socket(target.is_IPv4() ? AF_INET : AF_INET6, SOCK_STREAM) {
    assert(target.is_IPv4() || target.is_IPv6());
    CHECK(_socket.setReuseAddr(true), _socket.close(); return)
    CHECK(_socket.bind(target), _socket.close(); return)
    CHECK(_socket.tcpListen(ListenMax), _socket.close(); return)
    G_TRACE << "Acceptor " << _socket.fd() << " created.";
}

Acceptor::Acceptor(Socket &&socket) : _socket(std::move(socket)) {
    CHECK(_socket.setReuseAddr(true), _socket.close(); return)
    CHECK(_socket.tcpListen(ListenMax), _socket.close(); return)
    G_TRACE << "Acceptor " << _socket.fd() << " created.";
}

Acceptor::~Acceptor() {
    close();
}

void Acceptor::close() {
    if (!_socket) return;
    G_TRACE << "Acceptor " << _socket.fd() << " close.";
    _socket.close();
}

Acceptor::Message Acceptor::accept_connection() const {
    InetAddress address {};
    Socket socket = _socket.tcpAccept(address);
    CHECK(socket,)
    return { std::move(socket), address };
}
