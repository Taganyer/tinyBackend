//
// Created by taganyer on 3/9/24.
//

#ifndef NET_INETADDRESS_HPP
#define NET_INETADDRESS_HPP

#ifdef NET_INETADDRESS_HPP

#include <cstring>
#include <string>
#include <arpa/inet.h>

namespace Net {

    class InetAddress {
    public:
        InetAddress() = default;

        explicit InetAddress(const sockaddr_in &addr) : _addr(addr) {};

        explicit InetAddress(const sockaddr_in6 &addr6) : _addr6(addr6) {};

        /// 会调用 htons 函数，传入主机序。
        InetAddress(bool IPv4, const char* IP, short port = 0, unsigned short family = -1);

        InetAddress& operator=(const InetAddress &) = default;

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

        sockaddr_in* addr_in_cast() { return &_addr; };

        sockaddr_in6* addr6_in_cast() { return &_addr6; };

        [[nodiscard]] bool valid() const { return _addr.sin_family != PF_UNSPEC; };

        [[nodiscard]] bool is_IPv4() const {
            return _addr.sin_family == AF_INET;
        };

        [[nodiscard]] bool is_IPv6() const {
            return _addr6.sin6_family == AF_INET6;
        };

        [[nodiscard]] in_addr get_net_IP4() const {
            return _addr.sin_addr;
        };

        [[nodiscard]] in6_addr get_net_IP6() const {
            return _addr6.sin6_addr;
        };

        [[nodiscard]] std::string toIp() const;

        [[nodiscard]] std::string toIpPort() const;

        [[nodiscard]] sa_family_t family() const { return _addr.sin_family; };

        [[nodiscard]] unsigned short port() const { return _addr.sin_port; };

        [[nodiscard]] const sockaddr_in* addr_in_cast() const { return &_addr; };

        [[nodiscard]] const sockaddr_in6* addr6_in_cast() const { return &_addr6; };

        static InetAddress get_InetAddress(int fd);

        friend bool operator==(const InetAddress &left, const InetAddress &right) {
            if (left.family() != right.family() || left.port() != right.port())
                return false;
            if (left.is_IPv4())
                return left._addr.sin_addr.s_addr == right._addr.sin_addr.s_addr;
            if (left.is_IPv6())
                return std::memcmp(&left._addr6.sin6_addr, &right._addr6.sin6_addr, 16) == 0;
            return std::memcmp(&left._addr.sin_addr, &right._addr.sin_addr, 24) == 0;
        };

        friend bool operator!=(const InetAddress &left, const InetAddress &right) {
            return !(left == right);
        };

        friend struct std::less<InetAddress>;

        // static InetAddress getLocalHost();

    private:
        union {
            sockaddr_in _addr;

            sockaddr_in6 _addr6;
        };
    };
}

namespace std {

    template <>
    struct hash<Net::InetAddress> {
        size_t operator()(const Net::InetAddress &_val) const noexcept {
            if (_val.is_IPv4()) {
                return _Hash_impl::hash(&_val, 8);
            }
            if (_val.is_IPv6()) {
                char buf[20] {};

                auto family_port = (const int *) &_val;
                auto dest1 = (int *) buf;
                dest1[0] = family_port[0];

                auto ip = (const long long *) (&_val.addr6_in_cast()->sin6_addr);
                auto dest2 = (long long *) (buf + 4);
                dest2[0] = ip[0];
                dest2[1] = ip[1];
                return _Hash_impl::hash(buf, 20);
            }
            return _Hash_impl::hash(&_val, sizeof(Net::InetAddress));
        };
    };

    template <>
    struct less<Net::InetAddress> {
        bool operator()(const Net::InetAddress &lhs, const Net::InetAddress &rhs) const {
            if (lhs.family() != rhs.family())
                return lhs.family() < rhs.family();
            if (lhs.port() != rhs.port())
                return lhs.port() < rhs.port();
            if (lhs.is_IPv4())
                return lhs._addr.sin_addr.s_addr < rhs._addr.sin_addr.s_addr;
            if (lhs.is_IPv6())
                return std::memcmp(&lhs._addr6.sin6_addr, &rhs._addr6.sin6_addr, 16) < 0;
            return std::memcmp(&lhs._addr.sin_addr, &rhs._addr.sin_addr, 24) < 0;
        }
    };

}

#endif

#endif //NET_INETADDRESS_HPP
