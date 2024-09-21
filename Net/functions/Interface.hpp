//
// Created by taganyer on 3/9/24.
//

#ifndef NET_INTERFACE_HPP
#define NET_INTERFACE_HPP

#include "Base/Detail/config.hpp"
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;
struct iovec;

namespace Net {

    using size_t = unsigned long;

    namespace ops {

        inline const sockaddr *sockaddr_cast(const sockaddr_in6 *addr) {
            return static_cast<const sockaddr *>((const void *) addr);
        }

        inline sockaddr *sockaddr_cast(sockaddr_in6 *addr) {
            return static_cast<sockaddr *>((void *) (addr));
        }

        inline const sockaddr *sockaddr_cast(const sockaddr_in *addr) {
            return static_cast<const sockaddr *>((const void *) (addr));
        }

        inline sockaddr *sockaddr_cast(sockaddr_in *addr) {
            return static_cast<sockaddr *>((void *) (addr));
        }

        inline const sockaddr_in *sockaddr_in_cast(const sockaddr *addr) {
            return static_cast<const sockaddr_in *>((const void *) (addr));
        }

        inline const sockaddr_in6 *sockaddr_in6_cast(const sockaddr *addr) {
            return static_cast<const sockaddr_in6 *>((const void *) (addr));
        }

        inline int socket(int domain, int type, int protocol) {
            int sock = ::socket(domain, type, protocol);
            return sock;
        }

        inline int createNonblockSocket(int domain) {
            int sock = ::socket(domain, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
            return sock;
        }

        inline bool close(int fd) {
            int ret = ::close(fd);
            return ret == 0;
        }

        inline int64 read(int fd, void *dest, uint32 size) {
            int64 ret = ::read(fd, dest, size);
            return ret;
        }

        inline int64 readv(int fd, const iovec *iov, int iov_size) {
            int64 ret = ::readv(fd, iov, iov_size);
            return ret;
        }

        inline int64 write(int fd, const void *target, uint32 size) {
            int64 ret = ::write(fd, target, size);
            return ret;
        }

        inline int64 writev(int fd, const iovec *iov, int iov_size) {
            int64 ret = ::writev(fd, iov, iov_size);
            return ret;
        }

        inline bool connect(int fd, const sockaddr *addr) {
            int ret = ::connect(fd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
            return ret == 0;
        }

        inline bool bind(int fd, const sockaddr *addr) {
            int ret = ::bind(fd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
            return ret == 0;
        }

        inline bool listen(int fd, uint32 size) {
            int ret = ::listen(fd, size);
            return ret == 0;
        }

        inline int accept(int fd, sockaddr_in *addr) {
            auto len = static_cast<socklen_t>(sizeof(struct sockaddr_in6));
#if VALGRIND || defined (NO_ACCEPT4)
            int ret = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
#else
            int ret = ::accept4(fd, sockaddr_cast(addr),
                                &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
            return ret;
        }

        inline int accept(int fd, sockaddr_in6 *addr) {
            auto len = static_cast<socklen_t>(sizeof *addr);
#if VALGRIND || defined (NO_ACCEPT4)
            int ret = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
#else
            int ret = ::accept4(fd, sockaddr_cast(addr),
                                &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
            return ret;
        }

        inline bool shutdown(int fd, bool read, bool write) {
            int ret = 0;
            if (read && write) {
                ret = ::shutdown(fd, SHUT_RDWR);
            } else if (read) {
                ret = ::shutdown(fd, SHUT_RD);
            } else if (write) {
                ret = ::shutdown(fd, SHUT_WR);
            }
            return ret == 0;
        }

        inline bool set_socket_opt(int fd, int level, int opt_name,
                                   const void *opt_val, socklen_t opt_len) {
            return ::setsockopt(fd, level, opt_name, opt_val, opt_len) == 0;
        }

        inline bool get_socket_opt(int fd, int level, int opt_name,
                                   void *opt_val, socklen_t *opt_len) {
            return ::getsockopt(fd, level, opt_name, opt_val, opt_len) == 0;
        }

        int64 sendfile(int target_socket, int file_fd, off_t *offset, size_t count);

        void toIpPort(char *buf, uint32 size, const sockaddr *addr);

        void toIp(char *buf, uint32 size, const sockaddr *addr);

        void fromIpPort(const char *ip, uint16 port, sockaddr_in *addr);

        void fromIpPort(const char *ip, uint16 port, sockaddr_in6 *addr);

        int getSocketError(int fd);

        sockaddr_in6 getLocalAddr(int fd);

        sockaddr_in6 getPeerAddr(int fd);

        bool isSelfConnect(int fd);

        bool fd_isValid(int fd);

        bool Encoding_conversion(const char *from, const char *to,
                                 char *target, size_t input_len,
                                 char *dest, size_t output_len);

    }

}


#endif //NET_INTERFACE_HPP
