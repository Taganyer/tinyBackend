//
// Created by taganyer on 3/11/24.
//

#ifndef NET_ERRORS_HPP
#define NET_ERRORS_HPP

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
        Poll,
        Epoll_create,
        Epoll_wait,
        Epoll_ctl,
        Socket_opt,
        Encoding,
        Link_ErrorEvent,
        Link_CloseEvent,
        Link_TimeoutEvent
    };

    struct error_mark {
        error_types types = error_types::Null;
        int codes = -1;
    };


    namespace ops {

        const char *get_socket_error(int error);

        const char *get_close_error(int error);

        const char *get_read_error(int error);

        const char *get_write_error(int error);

        const char *get_connect_error(int error);

        const char *get_bind_error(int error);

        const char *get_listen_error(int error);

        const char *get_accept_error(int error);

        const char *get_shutdown_error(int error);

        const char *get_encoding_error(int error);

        const char *get_poll_error(int error);

        const char *get_epoll_create_error(int error);

        const char *get_epoll_wait_error(int error);

        const char *get_epoll_ctl_error(int error);

        const char *get_socket_opt_error(int error);

    }

}


#endif //NET_ERRORS_HPP
