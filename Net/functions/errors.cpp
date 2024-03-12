//
// Created by taganyer on 3/11/24.
//

#include <cerrno>
#include "errors.hpp"

namespace Net::ops {

    const char *get_socket_error(int error) {
        const char *ret;
        switch (error) {
            case EACCES:
                ret = "ops: creation permission is insufficient.";
                break;
            case EAFNOSUPPORT:
                ret = "ops: not support the specified address family.";
                break;
            case EINVAL:
                ret = "ops: unknown protocol, or protocol family not available, "
                      "or invalid flags.";
                break;
            case EMFILE:
                ret = "ops: the maximum number of sockets can be created.";
                break;
            case ENOBUFS:
            case ENOMEM:
                ret = "ops: insufficient memory is available.";
                break;
            case EPROTONOSUPPORT:
                ret = "ops: protocol is not supported within this domain.";
                break;
            default:
                ret = "ops: unknown error.";
                break;
        }
        return ret;
    }

    const char *get_close_error(int error) {
        const char *ret;
        switch (error) {
            case EBADF:
                ret = "close: fd isn't a valid open file descriptor.";
                break;
            case EINTR:
                ret = "close: close() call was interrupted by a signal.";
                break;
            case EIO:
                ret = "close: I/O error occurred.";
                break;
            default:
                ret = "close: unknown error.";
                break;
        }
        return ret;
    }

    const char *get_read_error(int error) {
        const char *ret;
        switch (error) {
            case EWOULDBLOCK:
                ret = "read: fd has been marked nonblocking,but read() would block.";
                break;
            case EBADF:
                ret = "read: fd is invalid or is not open for reading.";
                break;
            case EFAULT:
                ret = "read: buf is outside your accessible address space.";
                break;
            case EINTR:
                ret = "read: The call was interrupted by a signal before any data was read.";
                break;
            case EINVAL:
                ret = "read: fd is attached to an object which is unsuitable for reading,"
                      "or fd was created via a call to timerfd_create(2) and the wrong "
                      "size buffer was given to read()";
                break;
            case EIO:
                ret = "read: I/O error.";
                break;
            default:
                ret = "read: unknown error.";
                break;
        }
        return ret;
    }

    const char *get_write_error(int error) {
        const char *ret;
        switch (error) {
            case EWOULDBLOCK:
                ret = "write: fd marked nonblocking,but write() would block.";
                break;
            case EBADF:
                ret = "write: fd is invalid or is not open for writing.";
                break;
            case EDESTADDRREQ:
                ret = "write: fd has not using connect(2).";
                break;
            case EDQUOT:
                ret = "write: user's quota of disk blocks has been exhausted.";
                break;
            case EFAULT:
                ret = "write: buf is outside your accessible address space.";
                break;
            case EFBIG:
                ret = "write: The number of writes exceeds the limit.";
                break;
            case EINTR:
                ret = "write: call was interrupted by a signal.";
                break;
            case EINVAL:
                ret = "write: fd is unsuitable for writing, or the file was "
                      "opened with the O_DIRECT flag, and either the address"
                      " specified in buf, the value specified in count,or the"
                      " file offset is not suitably aligned.";
                break;
            case EIO:
                ret = "write: a low-level I/O error occurred while modifying the inode.";
                break;
            case ENOSPC:
                ret = "write: The device no room for the data.";
                break;
            case EPERM:
                ret = "write: operation was prevented by a file seal.";
                break;
            case EPIPE:
                ret = "write: fd is connected to a pipe or ops whose reading end is closed.";
                break;
            default:
                ret = "write: unknown error.";
                break;
        }
        return ret;
    }

    const char *get_connect_error(int error) {
        const char *ret;
        switch (error) {
            case ETIMEDOUT:
                ret = "connect: Timeout while attempting connection.";
                break;
            case EPERM:
            case EACCES:
                ret = "connect: permission is denied or tried to connect to a broadcast "
                      "address without having the ops broadcast flag enabled or the "
                      "connection request failed because of a local firewall rule.";
                break;
            case EADDRINUSE:
                ret = "connect: local address is already in use.";
                break;
            case EADDRNOTAVAIL:
                ret = "connect: fd had not previously been bound to an address.";
                break;
            case EAFNOSUPPORT:
                ret = "connect: passed address didn't have the correct address family.";
                break;
            case EAGAIN:
                ret = "connect: "
                      "for nonblocking sockets: ops is nonblocking,but connect would blocked;"
                      "for other insufficient entries in the routing cache.";
                break;
            case EALREADY:
                ret = "connect: ops is nonblocking and a previous connection attempt "
                      "has not yet been completed.";
                break;
            case EFAULT:
                ret = "connect: ops structure address is outside the user's address space.";
                break;
            case EBADF:
                ret = "connect: fd is not a valid open file descriptor.";
                break;
            case ECONNREFUSED:
                ret = "connect: no one listening on the remote address.";
                break;
            case EINTR:
                ret = "connect: system call was interrupted by a signal.";
                break;
            case EINPROGRESS:
                ret = "connect: ops is nonblocking but connect would blocked.";
                break;
            case EISCONN:
                ret = "connect: ops is already connected.";
                break;
            case ENETUNREACH:
                ret = "connect: Network is unreachable.";
                break;
            case ENOTSOCK:
                ret = "connect: fd does not refer to a ops.";
                break;
            case EPROTOTYPE:
                ret = "connect: ops type does not support the requested protocol.";
                break;
            default:
                ret = "connect: unknown error.";
        }
        return ret;
    }

    const char *get_bind_error(int error) {
        const char *ret;
        switch (error) {
            case EADDRINUSE:
                ret = "bind: address is already in use or port number "
                      "was specified as zero in the ops address structure";
                break;
            case EBADF:
                ret = "bind: fd is invalid.";
                break;
            case EINVAL:
                ret = "bind: ops is already bound to an address or addrlen is wrong, "
                      "or addr is not a valid address";
                break;
            case ENOTSOCK:
                ret = "bind: fd does not refer to a ops.";
                break;
            case EACCES:
                ret = "bind: search permission is denied on a component of the path prefix.";
                break;
            case EADDRNOTAVAIL:
                ret = "bind: nonexistent interface was requested or the "
                      "requested address was not local.";
                break;
            case EFAULT:
                ret = "bind: addr points outside the user's accessible address space.";
                break;
            case ELOOP:
                ret = "bind: Too many symbolic links.";
                break;
            case ENAMETOOLONG:
                ret = "bind: addr is too long.";
                break;
            case ENOENT:
                ret = "bind: prefix of the ops pathname does not exist.";
                break;
            case ENOMEM:
                ret = "bind: Insufficient kernel memory was available.";
                break;
            case ENOTDIR:
                ret = "bind: A component of the path prefix is not a directory.";
                break;
            case EROFS:
                ret = "bind: ops inode would reside on a read-only filesystem.";
                break;
            default:
                ret = "bind: unknown error.";
                break;
        }
        return ret;
    }

    const char *get_listen_error(int error) {
        const char *ret;
        switch (error) {
            case EACCES:
                ret = "listen: the address is protected or search permission "
                      "is denied on a component of the path prefix.";
                break;
            case EADDRINUSE:
                ret = "listen: given address is already in use or port number "
                      "was specified as zero in the ops address structure.";
                break;
            case EBADF:
                ret = "listen: fd is not a valid file descriptor.";
                break;
            case EINVAL:
                ret = "listen: ops is already bound to an address, or addrlen is "
                      "wrong,or addr is not a valid address for this ops's domain.";
                break;
            case ENOTSOCK:
                ret = "listen: fd does not refer to a ops.";
                break;
            case EADDRNOTAVAIL:
                ret = "listen: nonexistent interface was requested or the requested "
                      "address was not local.";
                break;
            case EFAULT:
                ret = "listen: addr points outside the user's accessible address space.";
                break;
            case ELOOP:
                ret = "listen: too many symbolic links were encountered in resolving addr.";
                break;
            case ENAMETOOLONG:
                ret = "listen: addr is too long.";
                break;
            case ENOENT:
                ret = "listen: component in the directory prefix of the ops pathname"
                      " does not exist.";
                break;
            case ENOMEM:
                ret = "listen: insufficient kernel memory was available.";
                break;
            case ENOTDIR:
                ret = "listen: component of the path prefix is not a directory.";
                break;
            case EROFS:
                ret = "listen: ops inode would reside on a read-only filesystem.";
                break;
            default:
                ret = "listen: unknown error.";
                break;
        }
        return ret;
    }

    const char *get_accept_error(int error) {
        const char *ret;
        switch (error) {
            case EAGAIN:
                ret = "accept: nonblocking ops no connections are present to be accepted.";
                break;
            case EBADF:
                ret = "accept: ops fd is not open.";
                break;
            case ECONNABORTED:
                ret = "accept: connection has been aborted.";
                break;
            case EFAULT:
                ret = "accept: addr argument is not in a writable part of the user address space.";
                break;
            case EINTR:
                ret = "accept: The system call was interrupted by a signal.";
                break;
            case EINVAL:
                ret = "accept: ops is not listening for connections, or addrlen is invalid";
                break;
            case EMFILE:
                ret = "accept: the maximum number of sockets can be created.";
                break;
            case ENOTSOCK:
                ret = "accept: fd does not refer to a ops.";
                break;
            case ENOBUFS:
            case ENOMEM:
                ret = "accept: insufficient memory is available.";
                break;
            case EOPNOTSUPP:
                ret = "accept: fd is not of type SOCK_STREAM.";
                break;
            case EPROTO:
                ret = "accept: Protocol error.";
                break;
            default:
                ret = "accept: unknown error.";
                break;
        }
        return ret;
    }

    const char *get_shutdown_error(int error) {
        const char *ret;
        switch (error) {
            case EBADF:
                ret = "shutdownWrite: fd is invalid.";
                break;
            case EINVAL:
                ret = "shutdownWrite: An invalid value was specified in how.";
                break;
            case ENOTCONN:
                ret = "shutdownWrite: ops is not connected.";
                break;
            case ENOTSOCK:
                ret = "shutdownWrite: fd does not refer to a ops.";
                break;
            default:
                ret = "shutdownWrite: unknown error.";
                break;
        }
        return ret;
    }

    const char *get_encoding_error(int error) {
        const char *ret;
        switch (error) {
            case E2BIG:
                ret = "Encoding_conversion: output buffer is insufficient";
                break;
            case EILSEQ:
                ret = "Encoding_conversion: invalid multibyte sequence";
                break;
            case EINVAL:
                ret = "Encoding_conversion: incomplete multibyte sequence or "
                      "The conversion from fromcode to tocode is not supported ";
                break;
            default:
                ret = "Encoding_conversion: unknown error.";
        }
        return ret;
    }

    const char *get_poll_error(int error) {
        const char *ret;
        switch (error) {
            case EFAULT:
                ret = "poll: fds points outside the process's accessible address space.";
                break;
            case EINTR:
                ret = "poll: a signal occurred before any requested event.";
                break;
            case EINVAL:
                ret = "poll: nfds value exceeds the RLIMIT_NOFILE value or "
                      "(ppoll())  The  timeout value expressed in *ip is invalid";
                break;
            case ENOMEM:
                ret = "poll: unable to allocate memory for kernel data structures.";
                break;
            default:
                ret = "poll: unknown error.";
        }
        return ret;
    }

    const char *get_epoll_create_error(int error) {
        const char *ret;
        switch (error) {
            case EINVAL:
                ret = "epoll_create: size is not positive or (epoll_create1()) "
                      "invalid value specified in flags.";
                break;
            case EMFILE:
                ret = "epoll_create: The number of epoll instances reaches the user limit"
                      "(control by /proc/sys/fs/epoll/max_user_instances) or "
                      "the number of fd reaches process/system limit.";
                break;
            case ENOMEM:
                ret = "epoll_create: insufficient memory to create the kernel object.";
                break;
            default:
                ret = "epoll_create: unknown error.";
        }
        return ret;
    }

    const char *get_epoll_wait_error(int error) {
        const char *ret;
        switch (error) {
            case EBADF:
                ret = "epoll_wait: epfd is not a valid file descriptor.";
                break;
            case EFAULT:
                ret = "epoll_wait: The memory area pointed to by events is not "
                      "accessible with write permissions.";
                break;
            case EINTR:
                ret = "epoll_wait: The call was interrupted by a signal.";
                break;
            case EINVAL:
                ret = "epoll_wait: epfd is not an epoll fd, or maxevents is less than one.";
                break;
            default:
                ret = "epoll_wait: unknown error.";
                break;
        }
        return ret;
    }

    const char *get_epoll_ctl_error(int error) {
        const char *ret;
        switch (error) {
            case EBADF:
                ret = "epoll_ctl: epfd or fd is invalid.";
                break;
            case EEXIST:
                ret = "epoll_ctl: EPOLL_CTL_ADD: supplied fd is already registered.";
                break;
            case EINVAL:
                ret = "epoll_ctl: epfd is not an epoll fd; or fd is the same as epfd; "
                      "or the requested operation op not support; "
                      "or invalid event type is specified in the event; "
                      "or EPOLL_CTL_MOD: events included EPOLLEXCLUSIVE or "
                      "the EPOLLEXCLUSIVE flag has previously been applied to epfd; "
                      "or EPOLLEXCLUSIVE was specified in event and fd refers to an epoll instance.";
                break;
            case ELOOP:
                ret = "epoll_ctl: fd refers to an epoll instance.";
                break;
            case ENOENT:
                ret = "epoll_ctl: EPOLL_CTL_MOD or EPOLL_CTL_DEL: fd is not registered.";
                break;
            case ENOMEM:
                ret = "epoll_ctl: insufficient memory to handle the requested op control operation.";
                break;
            case ENOSPC:
                ret = "epoll_wait: register fd reaches limit(by /proc/sys/fs/epoll/max_user_watches).";
                break;
            case EPERM:
                ret = "epoll_wait: target file fd does not support epoll.";
                break;
            default:
                ret = "epoll_wait: unknown error.";
                break;
        }
        return ret;
    }

    const char *get_socket_setOpt_error(int error) {
        const char *ret;

        return ret;
    }

    const char *get_socket_opt_error(int error) {
        const char *ret;
        switch (error) {
            case EBADF:
                ret = "getsockopt: fd is invalid.";
                break;
            case EFAULT:
                ret = "getsockopt: address pointed to by optval is not in a valid part of "
                      "the process address space.";
                break;
            case EINVAL:
                ret = "getsockopt: optlen invalid in setsockopt().";
                break;
            case ENOPROTOOPT:
                ret = "getsockopt: option is unknown at the level indicated.";
                break;
            case ENOTSOCK:
                ret = "getsockopt: fd does not refer to a socket.";
                break;
            default:
                ret = "getsockopt: unknown error.";
                break;
        }
        return ret;
    }

}
