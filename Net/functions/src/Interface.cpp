//
// Created by taganyer on 3/9/24.
//

#include "../Interface.hpp"
#include "Base/SystemLog.hpp"

#include <iconv.h>
#include <sys/sendfile.h>
#include <fcntl.h>

using namespace Net;

namespace Net::ops {

    int64 sendfile(int target_socket, int file_fd, off_t *offset, size_t count) {
        int64 ret = ::sendfile(target_socket, file_fd, offset, count);
        return ret;
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

    sockaddr_in6 getLocalAddr(int fd) {
        sockaddr_in6 addr{};
        memset(&addr, 0, sizeof addr);
        if (auto len = static_cast<socklen_t>(sizeof addr);
                ::getsockname(fd, sockaddr_cast(&addr), &len) < 0) {
            G_ERROR << "sockets::getLocalAddr";
        }
        return addr;
    }

    sockaddr_in6 getPeerAddr(int fd) {
        sockaddr_in6 addr{};
        memset(&addr, 0, sizeof addr);
        if (auto len = static_cast<socklen_t>(sizeof addr);
                ::getpeername(fd, sockaddr_cast(&addr), &len) < 0) {
            G_ERROR << "sockets::getPeerAddr";
        }
        return addr;
    }

    bool isSelfConnect(int fd) {
        sockaddr_in6 localAddr = getLocalAddr(fd);
        sockaddr_in6 peerAddr = getPeerAddr(fd);
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

    bool fd_isValid(int fd) {
        int record = errno;
        if (::fcntl(fd, F_GETFL) == -1) {
            errno = record;
            return false;
        }
        return true;
    }

    bool Encoding_conversion(const char *from, const char *to,
                             char *target, size_t input_len,
                             char *dest, size_t output_len) {
        if (!input_len) return true;
        iconv_t cd = iconv_open(to, from);
        if (cd == (iconv_t) -1) return false;
        auto ret = iconv(cd, &target, &input_len, &dest, &output_len);
        iconv_close(cd);
        return ret == -1;
    }

}
