//
// Created by taganyer on 3/9/24.
//

#include "../InetAddress.hpp"
#include "Net/functions/Interface.hpp"

using namespace Net;


InetAddress::InetAddress(bool IPv4, short port, const char* IP,
                         unsigned short family) {
    if (IPv4) {
        _addr.sin_port = port;
        _addr.sin_family = family == (unsigned short) -1 ? AF_INET : family;
    } else {
        _addr6.sin6_port = port;
        _addr6.sin6_family = family == (unsigned short) -1 ? AF_INET6 : family;
    }
    set_IP(IPv4, IP);
}

std::string InetAddress::toIp() const {
    char buf[64];
    ops::toIp(buf, sizeof buf, ops::sockaddr_cast(addr_in_cast()));
    return buf;
}

std::string InetAddress::toIpPort() const {
    char buf[64];
    ops::toIpPort(buf, sizeof buf, ops::sockaddr_cast(addr_in_cast()));
    return buf;
}
