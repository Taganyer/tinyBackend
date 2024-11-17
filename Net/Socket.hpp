//
// Created by taganyer on 3/9/24.
//

#ifndef NET_SOCKET_HPP
#define NET_SOCKET_HPP

#include "Base/Detail/NoCopy.hpp"

#define IGNORE_SIGPIPE

struct tcp_info;

namespace Net {

    class InetAddress;

    struct PipePair;

    class Socket : Base::NoCopy {
    public:
        Socket() = default;

        Socket(int domain, int type, int protocol = 0);

        Socket(Socket &&other) noexcept: _fd(other._fd) {
            other._fd = -1;
        };

        Socket &operator=(Socket &&other) noexcept;

        ~Socket();

        void close();

        [[nodiscard]] bool bind(const InetAddress &address) const;

        [[nodiscard]] bool tcpListen(int max_size) const;

        [[nodiscard]] bool tcpConnect(const InetAddress &address) const;

        Socket tcpAccept(InetAddress &address) const;

        /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        [[nodiscard]] bool setTcpNoDelay(bool on) const;

        /// Enable/disable SO_KEEPALIVE
        [[nodiscard]] bool setTcpKeepAlive(bool on) const;

        /// Enable/disable SO_REUSEADDR (After a socket is closed,
        ///                             other sockets are allowed to immediately bind to
        ///                             the address before the closing.)
        [[nodiscard]] bool setReuseAddr(bool on) const;

        /// Enable/disable SO_REUSEPORT (Multiple processes or threads can be bound to the same port
        ///                              at the same time. Only one of them can receive incoming connections
        ///                              or packets from the port at any one time.)
        [[nodiscard]] bool setReusePort(bool on) const;

        [[nodiscard]] bool setNonBlock(bool on) const;

        [[nodiscard]] bool shutdown_TcpRead() const;

        [[nodiscard]] bool shutdown_TcpWrite() const;

        bool TcpInfo(tcp_info* info) const;

        bool TcpInfo(char* buf, int len) const;

        operator bool() const { return _fd > 0; };

        [[nodiscard]] int fd() const { return _fd; };

        [[nodiscard]] bool valid() const { return _fd > 0; };

    private:
        int _fd = -1;

        explicit Socket(int fd) : _fd(fd) {};

    public:
        static PipePair create_pipe();

    };

    struct PipePair {
        Socket read;
        Socket write;

        PipePair() = default;

        PipePair(Socket read, Socket write);

        PipePair(PipePair &&other) noexcept = default;

    };
}


#endif //NET_SOCKET_HPP
