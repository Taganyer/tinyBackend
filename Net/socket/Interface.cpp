//
// Created by taganyer on 3/9/24.
//

#include "Interface.hpp"
#include "../../Base/Log/Log.hpp"

#include <sys/socket.h>
#include <cerrno>
#include <fcntl.h>
#include <cstdio>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iconv.h>

using namespace Net;

namespace Net::socket {
    int createNonblockSocket(int domain) {
        int sock = ::socket(domain, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
        if (unlikely(sock == -1)) {
            int error = errno;
            switch (error) {
                case EACCES:
                    G_ERROR << "socket: creation permission is insufficient.";
                    break;
                case EAFNOSUPPORT:
                    G_ERROR << "socket: not support the specified address family.";
                    break;
                case EINVAL:
                    G_ERROR << "socket: unknown protocol, or protocol family not available, "
                               "or invalid flags.";
                    break;
                case EMFILE:
                    G_ERROR << "socket: the maximum number of sockets can be created.";
                    break;
                case ENOBUFS:
                case ENOMEM:
                    G_ERROR << "socket: insufficient memory is available.";
                    break;
                case EPROTONOSUPPORT:
                    G_ERROR << "socket: protocol is not supported within this domain.";
                    break;
                default:
                    G_ERROR << "socket: unknown error: " << error;
                    break;
            }
            errno = error;
        }
        return sock;
    }

    bool close(int fd) {
        return ::close(fd) == 0;
    }

    int64 read(int fd, void *dest, uint32 size) {
        return ::read(fd, dest, size);
    }

    int64 readv(int fd, const iovec *iov, int iov_size) {
        return ::readv(fd, iov, iov_size);
    }

    int64 write(int fd, const void *target, uint32 size) {
        return ::write(fd, target, size);
    }

    int connect(int fd, sockaddr *addr) {
        return ::connect(fd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    }

    bool bind(int fd, const sockaddr *addr) {
        int ret = ::bind(fd, addr, sizeof(*addr));
        return ret == 0;
    }

    bool listen(int fd, uint32 size) {
        int ret = ::listen(fd, size);
        if (unlikely(ret < 0)) {
            int error = errno;
            switch (error) {
                case EACCES:
                    G_ERROR << "listen: the address is protected or search permission "
                               "is denied on a component of the path prefix.";
                    break;
                case EADDRINUSE:
                    G_ERROR << "listen: given address is already in use or port number "
                               "was specified as zero in the socket address structure.";
                    break;
                case EBADF:
                    G_ERROR << "listen: fd is not a valid file descriptor.";
                    break;
                case EINVAL:
                    G_ERROR << "listen: socket is already bound to an address, or addrlen is "
                               "wrong,or addr is not a valid address for this socket's domain.";
                    break;
                case ENOTSOCK:
                    G_ERROR << "listen: fd does not refer to a socket.";
                    break;
                case EADDRNOTAVAIL:
                    G_ERROR << "listen: nonexistent interface was requested or the requested "
                               "address was not local.";
                    break;
                case EFAULT:
                    G_ERROR << "listen: addr points outside the user's accessible address space.";
                    break;
                case ELOOP:
                    G_ERROR << "listen: too many symbolic links were encountered in resolving addr.";
                    break;
                case ENAMETOOLONG:
                    G_ERROR << "listen: addr is too long.";
                    break;
                case ENOENT:
                    G_ERROR << "listen: component in the directory prefix of the socket pathname"
                               " does not exist.";
                    break;
                case ENOMEM:
                    G_FATAL << "listen: insufficient kernel memory was available.";
                    break;
                case ENOTDIR:
                    G_ERROR << "listen: component of the path prefix is not a directory.";
                    break;
                case EROFS:
                    G_ERROR << "listen: socket inode would reside on a read-only filesystem.";
                    break;
                default:
                    G_ERROR << "listen: unknown error: " << error;
                    break;
            }
            errno = error;
        }
        return ret == 0;
    }

    int accept(int fd, sockaddr_in6 *addr) {
        auto len = static_cast<socklen_t>(sizeof *addr);
#if VALGRIND || defined (NO_ACCEPT4)
        int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
#else
        int conn = ::accept4(fd, sockaddr_cast(addr),
                             &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
        if (unlikely(conn < 0)) {
            int error = errno;
            switch (error) {
                case EAGAIN:
                    G_ERROR << "accept: nonblocking socket no connections are present to be accepted.";
                    break;
                case EBADF:
                    G_ERROR << "accept: socket fd is not open.";
                    break;
                case ECONNABORTED:
                    G_ERROR << "accept: connection has been aborted.";
                    break;
                case EFAULT:
                    G_ERROR << "accept: addr argument is not in a writable part of the user address space.";
                    break;
                case EINTR:
                    G_ERROR << "accept: The system call was interrupted by a signal.";
                    break;
                case EINVAL:
                    G_ERROR << "accept: socket is not listening for connections, or addrlen is invalid";
                    break;
                case EMFILE:
                    G_ERROR << "accept: the maximum number of sockets can be created.";
                    break;
                case ENOTSOCK:
                    G_ERROR << "accept: The file descriptor sockfd does not refer to a socket.";
                    break;
                case ENOBUFS:
                case ENOMEM:
                    G_ERROR << "accept: insufficient memory is available.";
                    break;
                case EOPNOTSUPP:
                    G_ERROR << "accept: fd is not of type SOCK_STREAM.";
                    break;
                case EPROTO:
                    G_ERROR << "accept: Protocol error.";
                    break;
                default:
                    G_FATAL << "accept: unknown error: " << error;
                    break;
            }
            errno = error;
        }
        return conn;
    }

    void shutdownWrite(int fd) {
        if (::shutdown(fd, SHUT_WR) < 0) {
            G_ERROR << "sockets::shutdownWrite";
        }
    }

    void toIpPort(char *buf, uint32 size, const sockaddr *addr) {
        if (addr->sa_family == AF_INET6) {
            buf[0] = '[';
            toIp(buf + 1, size - 1, addr);
            size_t end = ::strlen(buf);
            const sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
            uint16_t port = ntohs(addr6->sin6_port);
            assert(size > end);
            snprintf(buf + end, size - end, "]:%u", port);
        } else {
            toIp(buf, size, addr);
            size_t end = ::strlen(buf);
            const sockaddr_in *addr4 = sockaddr_in_cast(addr);
            uint16_t port = ntohs(addr4->sin_port);
            assert(size > end);
            snprintf(buf + end, size - end, ":%u", port);
        }
    }

    void toIp(char *buf, uint32 size, const sockaddr *addr) {
        if (addr->sa_family == AF_INET) {
            assert(size >= INET_ADDRSTRLEN);
            const sockaddr_in *addr4 = sockaddr_in_cast(addr);
            ::inet_ntop(AF_INET, &addr4->sin_addr, buf, size);
        } else if (addr->sa_family == AF_INET6) {
            assert(size >= INET6_ADDRSTRLEN);
            const sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
            ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, size);
        }
    }

    void fromIpPort(const char *ip, uint16_t port, sockaddr_in *addr) {
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
        if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
            G_ERROR << "sockets::fromIpPort";
        }
    }

    void fromIpPort(const char *ip, uint16_t port, sockaddr_in6 *addr) {
        addr->sin6_family = AF_INET6;
        addr->sin6_port = htons(port);
        if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
            G_ERROR << "sockets::fromIpPort";
        }
    }

    int getSocketError(int fd) {
        int val;
        auto len = static_cast<socklen_t>(sizeof val);

        if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &val, &len) < 0) {
            return errno;
        } else {
            return val;
        }
    }

    struct sockaddr_in6 getLocalAddr(int fd) {
        sockaddr_in6 addr{};
        memset(&addr, 0, sizeof addr);
        if (auto len = static_cast<socklen_t>(sizeof addr);
                ::getsockname(fd, sockaddr_cast(&addr), &len) < 0) {
            G_ERROR << "sockets::getLocalAddr";
        }
        return addr;
    }

    struct sockaddr_in6 getPeerAddr(int fd) {
        sockaddr_in6 addr{};
        memset(&addr, 0, sizeof addr);
        if (auto len = static_cast<socklen_t>(sizeof addr);
                ::getpeername(fd, sockaddr_cast(&addr), &len) < 0) {
            G_ERROR << "sockets::getPeerAddr";
        }
        return addr;
    }

    bool isSelfConnect(int fd) {
        struct sockaddr_in6 localAddr = getLocalAddr(fd);
        struct sockaddr_in6 peerAddr = getPeerAddr(fd);
        if (localAddr.sin6_family == AF_INET) {
            const sockaddr_in *laddr4 = (sockaddr_in *) &localAddr;
            const sockaddr_in *raddr4 = (sockaddr_in *) &peerAddr;
            return laddr4->sin_port == raddr4->sin_port
                   && laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
        } else if (localAddr.sin6_family == AF_INET6) {
            return localAddr.sin6_port == peerAddr.sin6_port
                   && memcmp(&localAddr.sin6_addr, &peerAddr.sin6_addr, sizeof localAddr.sin6_addr) == 0;
        } else {
            return false;
        }
    }

    const struct sockaddr *sockaddr_cast(const sockaddr_in6 *addr) {
        return static_cast<const sockaddr *>((const void *) addr);
    }

    struct sockaddr *sockaddr_cast(sockaddr_in6 *addr) {
        return static_cast<sockaddr *>((void *) (addr));
    }

    const struct sockaddr *sockaddr_cast(const sockaddr_in *addr) {
        return static_cast<const sockaddr *>((const void *) (addr));
    }

    const struct sockaddr_in *sockaddr_in_cast(const sockaddr *addr) {
        return static_cast<const struct sockaddr_in *>((const void *) (addr));
    }

    const struct sockaddr_in6 *sockaddr_in6_cast(const sockaddr *addr) {
        return static_cast<const struct sockaddr_in6 *>((const void *) (addr));
    }

    void Encoding_conversion(const char *from, const char *to,
                             char *target, size_t input_len,
                             char *dest, size_t output_len) {
        if (!input_len) return;
        iconv_t cd = iconv_open(to, from);
        if (cd == (iconv_t) -1) return;
        if (unlikely(iconv(cd, &target, &input_len, &dest, &output_len) == -1)) {
            switch (errno) {
                case E2BIG:
                    G_INFO << "Encoding_conversion: output buffer is insufficient";
                    break;
                case EILSEQ:
                    G_ERROR << "Encoding_conversion: invalid multibyte sequence";
                    break;
                case EINVAL:
                    G_ERROR << "Encoding_conversion: incomplete multibyte sequence";
                    break;
                default:
                    G_ERROR << "Encoding_conversion: unknown error";
            }
        }
        iconv_close(cd);
    }

}
