//
// Created by taganyer on 3/9/24.
//

#ifndef NET_SOCKET_HPP
#define NET_SOCKET_HPP

#ifdef NET_SOCKET_HPP

#include "file/FileDescriptor.hpp"

struct tcp_info;

namespace Net {

    class InetAddress;

    class Socket : public FileDescriptor {
    public:

        Socket(int domain, int type, int protocol = 0);

        explicit Socket(int fd) : FileDescriptor(fd) {};

        Socket(Socket &&other) noexcept: FileDescriptor(other._fd) {
            other._fd = -1;
        };

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

        void shutdown_TcpRead();

        void shutdown_TcpWrite();

        bool TcpInfo(struct tcp_info *info) const;

        bool TcpInfo(char *buf, int len) const;

    };

}

#endif

#endif //NET_SOCKET_HPP
