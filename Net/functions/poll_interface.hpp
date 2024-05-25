//
// Created by taganyer on 3/11/24.
//

#ifndef NET_POLL_INTERFACE_HPP
#define NET_POLL_INTERFACE_HPP

#ifdef NET_POLL_INTERFACE_HPP

#include <poll.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "Base/Detail/config.hpp"

struct pollfd;

struct epoll_event;


namespace Net::ops {

    inline int poll(pollfd *pollfd, uint64 nfds, int timeoutMS) {
        int ret = ::poll(pollfd, nfds, timeoutMS);
        return ret;
    }

    inline int epoll_create() {
        return ::epoll_create1(EPOLL_CLOEXEC);
    }

    inline int epoll_close(int epfd) {
        return ::close(epfd);
    }

    inline int epoll_wait(int epfd, epoll_event *event, int max_size, int timeoutMS) {
        return ::epoll_wait(epfd, event, max_size, timeoutMS);
    }

    inline int epoll_ctl(int epfd, int operation, int fd, epoll_event *event) {
        return ::epoll_ctl(epfd, operation, fd, event);
    }

}

#endif

#endif //NET_POLL_INTERFACE_HPP
