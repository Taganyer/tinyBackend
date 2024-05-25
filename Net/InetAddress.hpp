//
// Created by taganyer on 3/9/24.
//

#ifndef NET_INETADDRESS_HPP
#define NET_INETADDRESS_HPP

#ifdef NET_INETADDRESS_HPP

#include <string>
#include <arpa/inet.h>

namespace Net {

    class InetAddress {
    public:
        InetAddress() = default;

        InetAddress(const sockaddr_in &addr) : _addr(addr) {};

        InetAddress(const sockaddr_in6 &addr6) : _addr6(addr6) {};

        InetAddress(bool IPv4, short port, const char* IP, unsigned short family = -1);

        void set_port(bool IPv4, short port) {
            if (IPv4) {
                _addr.sin_port = htons(port);
            } else {
                _addr6.sin6_port = htons(port);
            }
        };

        void set_family(bool IPv4, unsigned short family) {
            if (IPv4) {
                _addr.sin_family = family;
            } else {
                _addr6.sin6_family = family;
            }
        };

        bool set_IP(bool Ipv4, const char* IP) {
            if (Ipv4) {
                return inet_pton(AF_INET, IP, &_addr.sin_addr) > 0;
            } else {
                return inet_pton(AF_INET6, IP, &_addr6.sin6_addr) > 0;
            }
        };

        void setScopeId(unsigned id) {
            _addr6.sin6_scope_id = id;
        };

        [[nodiscard]] std::string toIp() const;

        [[nodiscard]] std::string toIpPort() const;

        sockaddr_in* addr_in_cast() { return &_addr; };

        sockaddr_in6* addr6_in_cast() { return &_addr6; };

        [[nodiscard]] sa_family_t family() const { return _addr.sin_family; };

        [[nodiscard]] unsigned short port() const { return _addr.sin_port; };

        [[nodiscard]] const sockaddr_in* addr_in_cast() const { return &_addr; };

        [[nodiscard]] const sockaddr_in6* addr6_in_cast() const { return &_addr6; };

    private:
        union {
            sockaddr_in _addr;

            sockaddr_in6 _addr6;
        };
    };
}

#endif

#endif //NET_INETADDRESS_HPP
