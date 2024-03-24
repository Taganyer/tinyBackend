//
// Created by taganyer on 3/9/24.
//

#include <netinet/tcp.h>
#include "Socket.hpp"
#include "error/errors.hpp"
#include "InetAddress.hpp"
#include "../Base/Log/Log.hpp"
#include "functions/Interface.hpp"

using namespace Net;


Socket::Socket(int domain, int type, int protocol) :
        FileDescriptor(ops::socket(domain, type, protocol)) {
    if (!valid())
        G_ERROR << "Socket create " << ops::get_socket_error(errno);
}

bool Socket::Bind(InetAddress &address) {
    if (!ops::bind(_fd, ops::sockaddr_cast(address.addr_in_cast())))
        G_ERROR << _fd << ' ' << ops::get_bind_error(errno);
    return true;
}

bool Socket::tcpListen(int max_size) {
    if (!ops::listen(_fd, max_size)) {
        G_ERROR << "Socket " << _fd << ' ' << ops::get_listen_error(errno);
        return false;
    }
    return true;
}

bool Socket::tcpConnect(InetAddress &address) {
    if (!ops::connect(_fd, ops::sockaddr_cast(address.addr_in_cast()))) {
        G_ERROR << "Socket " << _fd << ' ' << ops::get_connect_error(errno);
        return false;
    }
    return true;
}

int Socket::tcpAccept(InetAddress &address) {
    int fd = ops::accept(_fd, address.addr6_in_cast());
    if (fd < 0)
        G_ERROR << "Socket " << _fd << ' ' << ops::get_accept_error(errno);
    return fd;
}

bool Socket::setTcpNoDelay(bool on) {
    int opt_val = on ? 1 : 0;
    if (!ops::set_socket_opt(_fd, IPPROTO_TCP, TCP_NODELAY,
                             &opt_val, static_cast<socklen_t>(sizeof(int)))) {
        G_ERROR << "Socket " << _fd << " set TCP_NODELAY " << ops::get_socket_opt_error(errno);
        return false;
    }
    return true;
}

bool Socket::setTcpKeepAlive(bool on) {
    int opt_val = on ? 1 : 0;
    if (!ops::set_socket_opt(_fd, SOL_SOCKET, SO_KEEPALIVE,
                             &opt_val, static_cast<socklen_t>(sizeof(int)))) {
        G_ERROR << "Socket " << _fd << " set TCP KEEPALIVE " << ops::get_socket_opt_error(errno);
        return false;
    }
    return true;
}

bool Socket::setReuseAddr(bool on) {
    int opt_val = on ? 1 : 0;
    if (!ops::set_socket_opt(_fd, SOL_SOCKET, SO_REUSEADDR,
                             &opt_val, static_cast<socklen_t>(sizeof(int)))) {
        G_ERROR << "Socket " << _fd << " set REUSEADDR " << ops::get_socket_opt_error(errno);
        return false;
    }
    return true;
}

bool Socket::setReusePort(bool on) {
    int opt_val = on ? 1 : 0;
    if (!ops::set_socket_opt(_fd, SOL_SOCKET, SO_REUSEPORT,
                             &opt_val, static_cast<socklen_t>(sizeof(int)))) {
        G_ERROR << "Socket " << _fd << " set REUSEPORT " << ops::get_socket_opt_error(errno);
        return false;
    }
    return true;
}

void Socket::shutdown_TcpWrite() {
    if (!ops::shutdownWrite(_fd))
        G_ERROR << "Socket " << _fd << ops::get_shutdown_error(errno);
}

bool Socket::TcpInfo(tcp_info *info) const {
    socklen_t len = sizeof(tcp_info);
    memset(info, 0, len);
    return ::getsockopt(_fd, SOL_TCP, TCP_INFO, info, &len) == 0;
}

bool Socket::TcpInfo(char *buf, int len) const {
    tcp_info info{};
    if (!TcpInfo(&info)) {
        G_ERROR << "Socket " << _fd << ' ' << ops::get_socket_opt_error(errno);
        return false;
    }
    /// FIXME 进一步扩充
    sprintf(buf, buf, len, "unrecovered = %u "
                           "rto = %u ato = %u snd_mss = %u rcv_mss = %u "
                           "lost = %u retrans = %u rtt = %u rttvar = %u "
                           "sshthresh = %u cwnd = %u total_retrans = %u",
            info.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
            info.tcpi_rto,          // Retransmit timeout in usec
            info.tcpi_ato,          // Predicted tick of soft clock in usec
            info.tcpi_snd_mss,
            info.tcpi_rcv_mss,
            info.tcpi_lost,         // Lost packets
            info.tcpi_retrans,      // Retransmitted packets out
            info.tcpi_rtt,          // Smoothed round trip time in usec
            info.tcpi_rttvar,       // Medium deviation
            info.tcpi_snd_ssthresh,
            info.tcpi_snd_cwnd,
            info.tcpi_total_retrans);  // Total retransmits for entire connection
    return true;
}
