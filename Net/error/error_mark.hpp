//
// Created by taganyer on 3/24/24.
//

#ifndef NET_ERROR_MARK_HPP
#define NET_ERROR_MARK_HPP

namespace Net {

    enum class error_types {
        Null,
        Socket,
        Close,
        Read,
        Write,
        Connect,
        Bind,
        Listen,
        Accept,
        Shutdown,
        Sendfile,
        Encoding,
        Select,
        Poll,
        Epoll_create,
        Epoll_wait,
        Epoll_ctl,
        Socket_opt,
        ErrorEvent,
        TimeoutEvent,
        UnexpectedShutdown
    };

    struct error_mark {
        error_types types = error_types::Null;
        int codes = -1;
    };
}

#endif //NET_ERROR_MARK_HPP
