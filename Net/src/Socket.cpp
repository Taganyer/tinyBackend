//
// Created by taganyer on 3/9/24.
//

#include "../Socket.hpp"
#include <fcntl.h>
#include <netinet/tcp.h>
#include "tinyBackend/Base/SystemLog.hpp"
#include "tinyBackend/Net/InetAddress.hpp"
#include "tinyBackend/Net/error/errors.hpp"
#include "tinyBackend/Net/functions/Interface.hpp"

#ifdef IGNORE_SIGPIPE
#include <csignal>
static auto _ = [] { return signal(SIGPIPE, SIG_IGN); }();
#endif

using namespace Net;

Socket::Socket(int domain, int type, int protocol) :
    _fd(ops::socket(domain, type, protocol)) {
    if (!valid()) {
        G_ERROR << "Socket create " << ops::get_socket_error(errno);
    } else {
        G_INFO << "Socket " << _fd << " created.";
    }
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (_fd > 0) {
        if (ops::close(_fd)) {
            G_INFO << "socket close " << _fd;
        } else {
            G_FATAL << "socket " << _fd << ' ' << ops::get_close_error(errno);
        }
    }
    _fd = other._fd;
    other._fd = -1;
    return *this;
}

Socket::~Socket() {
    close();
}

void Socket::close() {
    if (_fd > 0) {
        if (ops::close(_fd)) {
            G_INFO << "socket close " << _fd;
        } else {
            G_FATAL << "socket " << _fd << ' ' << ops::get_close_error(errno);
        }
        _fd = -1;
    }
}

bool Socket::bind(const InetAddress& address) const {
    if (!ops::bind(_fd, ops::sockaddr_cast(address.addr_in_cast()))) {
        G_ERROR << _fd << ' ' << ops::get_bind_error(errno);
        return false;
    }
    return true;
}

bool Socket::connect(const InetAddress& address) const {
    if (!ops::connect(_fd, ops::sockaddr_cast(address.addr_in_cast()))) {
        G_ERROR << "Socket " << _fd << ' ' << ops::get_connect_error(errno);
        return false;
    }
    return true;
}

bool Socket::tcpListen(int max_size) const {
    if (!ops::listen(_fd, max_size)) {
        G_ERROR << "Socket " << _fd << ' ' << ops::get_listen_error(errno);
        return false;
    }
    return true;
}

Socket Socket::tcpAccept(InetAddress& address) const {
    int fd = ops::accept(_fd, address.addr6_in_cast());
    if (fd < 0)
        G_ERROR << "Socket " << _fd << ' ' << ops::get_accept_error(errno);
    return Socket(fd);
}

bool Socket::setTcpNoDelay(bool on) const {
    int opt_val = on ? 1 : 0;
    if (!ops::set_socket_opt(_fd, IPPROTO_TCP, TCP_NODELAY,
                             &opt_val, sizeof(int))) {
        G_ERROR << "Socket " << _fd << " set TCP_NODELAY " << ops::get_socket_opt_error(errno);
        return false;
    }
    return true;
}

bool Socket::setTcpKeepAlive(bool on) const {
    int opt_val = on ? 1 : 0;
    if (!ops::set_socket_opt(_fd, SOL_SOCKET, SO_KEEPALIVE,
                             &opt_val, sizeof(int))) {
        G_ERROR << "Socket " << _fd << " set TCP KEEPALIVE " << ops::get_socket_opt_error(errno);
        return false;
    }
    return true;
}

bool Socket::setReuseAddr(bool on) const {
    int opt_val = on ? 1 : 0;
    if (!ops::set_socket_opt(_fd, SOL_SOCKET, SO_REUSEADDR,
                             &opt_val, sizeof(int))) {
        G_ERROR << "Socket " << _fd << " set REUSEADDR " << ops::get_socket_opt_error(errno);
        return false;
    }
    return true;
}

bool Socket::setReusePort(bool on) const {
    int opt_val = on ? 1 : 0;
    if (!ops::set_socket_opt(_fd, SOL_SOCKET, SO_REUSEPORT,
                             &opt_val, sizeof(int))) {
        G_ERROR << "Socket " << _fd << " set REUSEPORT " << ops::get_socket_opt_error(errno);
        return false;
    }
    return true;
}

bool Socket::setNonBlock(bool on) const {
    int flags = fcntl(_fd, F_GETFL, 0);
    if (flags < 0) {
        G_ERROR << "Socket " << _fd << " fcntl(3) F_GETFL failed.";
        return false;
    }
    flags = on ? flags | O_NONBLOCK : flags & ~O_NONBLOCK;
    if (fcntl(_fd, F_SETFL, flags) != 0) {
        G_ERROR << "Socket " << _fd << " fcntl(3) F_SETFL failed.";
        return false;
    }
    return true;
}

bool Socket::shutdown_TcpRead() const {
    if (!ops::shutdown(_fd, true, false)) {
        G_ERROR << "Socket " << _fd << ops::get_shutdown_error(errno);
        return false;
    }
    return true;
}

bool Socket::shutdown_TcpWrite() const {
    if (!ops::shutdown(_fd, false, true)) {
        G_ERROR << "Socket " << _fd << ops::get_shutdown_error(errno);
        return false;
    }
    return true;
}

bool Socket::TcpInfo(tcp_info *info) const {
    socklen_t len = sizeof(tcp_info);
    memset(info, 0, len);
    return ::getsockopt(_fd, SOL_TCP, TCP_INFO, info, &len) == 0;
}

bool Socket::TcpInfo(char *buf, int len) const {
    tcp_info info {};
    if (!TcpInfo(&info)) {
        G_ERROR << "Socket " << _fd << ' ' << ops::get_socket_opt_error(errno);
        return false;
    }
    /// FIXME 进一步扩充
    snprintf(buf, len, "unrecovered = %u "
             "rto = %u ato = %u snd_mss = %u rcv_mss = %u "
             "lost = %u retrans = %u rtt = %u rttvar = %u "
             "sshthresh = %u cwnd = %u total_retrans = %u",
             info.tcpi_retransmits, // Number of unrecovered [RTO] timeouts
             info.tcpi_rto,         // Retransmit timeout in usec
             info.tcpi_ato,         // Predicted tick of soft clock in usec
             info.tcpi_snd_mss,
             info.tcpi_rcv_mss,
             info.tcpi_lost,    // Lost packets
             info.tcpi_retrans, // Retransmitted packets out
             info.tcpi_rtt,     // Smoothed round trip time in usec
             info.tcpi_rttvar,  // Medium deviation
             info.tcpi_snd_ssthresh,
             info.tcpi_snd_cwnd,
             info.tcpi_total_retrans); // Total retransmits for entire connection
    return true;
}

PipePair Socket::create_pipe() {
    int fds[2];
    if (pipe(fds) < 0)
        return PipePair {};
    return { Socket(fds[0]), Socket(fds[1]) };
}

PipePair::PipePair(Socket read, Socket write) :
    read(std::move(read)), write(std::move(write)) {}
