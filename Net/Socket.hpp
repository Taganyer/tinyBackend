//
// Created by taganyer on 3/9/24.
//

#ifndef NET_SOCKET_HPP
#define NET_SOCKET_HPP

#include "../Base/Detail/NoCopy.hpp"

struct tcp_info;

namespace Net {

    class InetAddress;

    class Socket : Base::NoCopy {
    public:

        Socket(int domain, int type, int protocol = 0);

        explicit Socket(int fd) : _fd(fd) {};

        Socket(Socket &&other) noexcept: _fd(other._fd) {
            other._fd = -1;
        };

        ~Socket();

        bool Bind(InetAddress &address);

        bool tcpListen(int max_size);

        bool tcpConnect(InetAddress &address);

        int tcpAccept(InetAddress &address);

        /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        bool setTcpNoDelay(bool on);

        /// Enable/disable SO_KEEPALIVE
        bool setTcpKeepAlive(bool on);

        /// Enable/disable SO_REUSEADDR
        bool setReuseAddr(bool on);

        /// Enable/disable SO_REUSEPORT
        bool setReusePort(bool on);

        void shutdown_TcpWrite();

        bool TcpInfo(struct tcp_info *info) const;

        bool TcpInfo(char *buf, int len) const;

        [[nodiscard]] int fd() const { return _fd; };

        [[nodiscard]] bool valid() const { return _fd > 0; };

    private:

        int _fd;
    };

}


#endif //NET_SOCKET_HPP
