//
// Created by taganyer on 3/9/24.
//

#include "../InetAddress.hpp"
#include "Net/functions/Interface.hpp"

using namespace Net;


InetAddress::InetAddress(bool IPv4, const char* IP, short port,
                         unsigned short family) {
    if (IPv4) {
        _addr.sin_port = htons(port);
        _addr.sin_family = family == (unsigned short) -1 ? AF_INET : family;
    } else {
        _addr6.sin6_port = htons(port);
        _addr6.sin6_family = family == (unsigned short) -1 ? AF_INET6 : family;
    }
    set_IP(IPv4, IP);
}

std::string InetAddress::toIp() const {
    if (!valid()) return "[Invalid]";
    char buf[INET6_ADDRSTRLEN] {};
    ops::toIp(buf, sizeof buf, ops::sockaddr_cast(addr_in_cast()));
    return { buf };
}

std::string InetAddress::toIpPort() const {
    if (!valid()) return "[Invalid]";
    char buf[64];
    ops::toIpPort(buf, sizeof buf, ops::sockaddr_cast(addr_in_cast()));
    return buf;
}

InetAddress InetAddress::get_InetAddress(int fd) {
    InetAddress addr{};
    socklen_t addr_size = sizeof(addr);
    if (getsockname(fd, (sockaddr*)&addr, &addr_size))
        return {};
    return addr;
}

// InetAddress InetAddress::getLocalHost() {
// #include <cstring>
// #include <ifaddrs.h>
//     sockaddr_in* addr = nullptr;
//     ifaddrs* interfaces;
//     getifaddrs(&interfaces);
//     for (ifaddrs* ifa = interfaces; ifa; ifa = ifa->ifa_next) {
//         if (strcmp(ifa->ifa_name, "lo") != 0 && ifa->ifa_addr
//             && ifa->ifa_addr->sa_family == AF_INET) {
//             addr = (sockaddr_in *) ifa->ifa_addr;
//             break;
//         }
//     }
//     freeifaddrs(interfaces);
//     return addr ? InetAddress { *addr } : InetAddress {};
// }
