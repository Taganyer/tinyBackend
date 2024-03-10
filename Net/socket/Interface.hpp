//
// Created by taganyer on 3/9/24.
//

#ifndef NET_INTERFACE_HPP
#define NET_INTERFACE_HPP

#include "../../Base/Detail/config.hpp"

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;
struct iovec;

namespace Net {

    using size_t = unsigned long;

    namespace socket {

        int createNonblockSocket(int domain);

        bool close(int fd);

        int64 read(int fd, void *dest, uint32 size);

        int64 readv(int fd, const iovec *iov, int iov_size);

        int64 write(int fd, const void *target, uint32 size);

        int connect(int fd, sockaddr *addr);

        bool bind(int fd, const sockaddr *addr);

        bool listen(int fd, uint32 size);

        int accept(int fd, sockaddr_in6 *addr);

        void shutdownWrite(int fd);

        void toIpPort(char *buf, uint32 size, const sockaddr *addr);

        void toIp(char *buf, uint32 size, const sockaddr *addr);

        void fromIpPort(const char *ip, uint16 port, sockaddr_in *addr);

        void fromIpPort(const char *ip, uint16 port, sockaddr_in6 *addr);

        int getSocketError(int fd);

        const struct sockaddr *sockaddr_cast(const sockaddr_in *addr);

        const struct sockaddr *sockaddr_cast(const sockaddr_in6 *addr);

        struct sockaddr *sockaddr_cast(sockaddr_in6 *addr);

        const struct sockaddr_in *sockaddr_in_cast(const sockaddr *addr);

        const struct sockaddr_in6 *sockaddr_in6_cast(const sockaddr *addr);

        struct sockaddr_in6 getLocalAddr(int fd);

        struct sockaddr_in6 getPeerAddr(int fd);

        bool isSelfConnect(int fd);

        void Encoding_conversion(const char *from, const char *to,
                                 char *target, size_t input_len,
                                 char *dest, size_t output_len);

    }

}


#endif //NET_INTERFACE_HPP
