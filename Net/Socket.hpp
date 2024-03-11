//
// Created by taganyer on 3/9/24.
//

#ifndef NET_SOCKET_HPP
#define NET_SOCKET_HPP

#include "../Base/Detail/NoCopy.hpp"

namespace Net {

    class InetAddress;

    class Socket : Base::NoCopy {
    public:

        Socket(int domain, int type, int protocol = 0);

        explicit Socket(int fd);

        ~Socket();

        bool Bind(const InetAddress &address);

        bool tcpListen(int max_size);

        bool tcpConnect();

        bool tcpAccept();

        /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
        void setTcpNoDelay(bool on);

        /// Enable/disable SO_REUSEADDR
        void setReuseAddr(bool on);

        /// Enable/disable SO_REUSEPORT
        void setReusePort(bool on);

        /// Enable/disable SO_KEEPALIVE
        void setKeepAlive(bool on);

        void shutdown_write();

        bool TcpInfo(struct tcp_info *) const;

        bool TcpInfo(char *buf, int len) const;

        [[nodiscard]] int fd() const { return _fd; };

        [[nodiscard]] bool valid() const { return _fd > 0; };

    private:

        const int _fd;
    };

}


#endif //NET_SOCKET_HPP
