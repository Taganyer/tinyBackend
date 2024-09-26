//
// Created by taganyer on 24-3-5.
//

#ifndef NET_NETLINK_HPP
#define NET_NETLINK_HPP

#ifdef NET_NETLINK_HPP

#include <memory>
#include <functional>
#include "Socket.hpp"
#include "Base/Buffer/RingBuffer.hpp"


namespace Net {

    struct error_mark;

    class Event;

    class Acceptor;

    class NetLink : public std::enable_shared_from_this<NetLink>, Base::NoCopy {
    public:
        using ReadCallback = std::function<void(Base::RingBuffer &, Socket &)>;

        using WriteCallback = std::function<void(Socket &)>;

        /*
         * 返回值表示是否调用 CloseCallback
         * 可能返回的错误有：
         *                error_types::Read,
         *                error_types::Write,
         *                error_types::Epoll_ctl,
         *                error_types::ErrorEvent,
         *                error_types::TimeoutEvent
        */
        using ErrorCallback = std::function<bool(error_mark, Socket &)>;

        using CloseCallback = std::function<void(Socket &)>;

        using SocketPtr = std::unique_ptr<Socket>;

        using AcceptorPtr = std::shared_ptr<Acceptor>;

        using LinkPtr = std::shared_ptr<NetLink>;

        using WeakPtr = std::weak_ptr<NetLink>;

        /// 隐藏了 NetLink 的构造函数，确保生成的对象都是 heap 对象。
        static LinkPtr create_NetLinkPtr(SocketPtr &&socket_ptr,
                                         const AcceptorPtr &acceptor_ptr);

        ~NetLink();

        /// 强行关闭文件描述符，多次调用不会出现问题。
        void close_fd();

        Socket& socket() { return *_socket; };

        void set_readCallback(ReadCallback event) { _readFun = std::move(event); };

        void set_writeCallback(WriteCallback event) { _writeFun = std::move(event); };

        void set_errorCallback(ErrorCallback event) { _errorFun = std::move(event); };

        void set_closeCallback(CloseCallback event) { _closeFun = std::move(event); };

        [[nodiscard]] bool valid() const { return _socket->valid(); };

        [[nodiscard]] int fd() const { return _socket->fd(); };

    private:
        std::unique_ptr<Socket> _socket;

        ReadCallback _readFun;

        WriteCallback _writeFun;

        ErrorCallback _errorFun;

        CloseCallback _closeFun;

        Base::RingBuffer _input;

        Base::RingBuffer _output;

        std::weak_ptr<Acceptor> _acceptor;

        friend class Controller;

        friend class Reactor;

        friend class BalancedReactor;

        friend class Acceptor;

        void handle_read(Event* event);

        void handle_write(Event* event);

        void handle_error(error_mark mark, Event* event);

        void handle_close();

        bool handle_timeout();

        NetLink(SocketPtr &&socket_ptr, const AcceptorPtr &acceptor_ptr);

    };

}

#endif

#endif //NET_NETLINK_HPP
