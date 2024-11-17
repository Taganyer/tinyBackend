//
// Created by taganyer on 24-3-5.
//

#ifndef NET_NETLINK_HPP
#define NET_NETLINK_HPP

#ifdef NET_NETLINK_HPP

#include <memory>
#include <functional>
#include "Socket.hpp"
#include "Net/TcpMessageAgent.hpp"


namespace Net {

    struct error_mark;

    class Event;

    class Controller;

    class NetLink : public std::enable_shared_from_this<NetLink>, Base::NoCopy {
    public:
        static constexpr uint32 DEFAULT_INPUT_BUFFER_SIZE = 1024;

        static constexpr uint32 DEFAULT_OUTPUT_BUFFER_SIZE = 1024;

        using ReadCallback = std::function<void(const Controller &)>;

        using WriteCallback = std::function<void(const Controller &)>;

        /*
         * 返回值表示是否调用 CloseCallback
         * 可能返回的错误有：
         *                error_types::Read,
         *                error_types::Write,
         *                error_types::Epoll_ctl,
         *                error_types::ErrorEvent,
         *                error_types::TimeoutEvent
        */
        using ErrorCallback = std::function<bool(error_mark, const Controller &)>;

        using CloseCallback = std::function<void(const Controller &)>;

        using LinkPtr = std::shared_ptr<NetLink>;

        using WeakPtr = std::weak_ptr<NetLink>;

        /// 隐藏了 NetLink 的构造函数，确保生成的对象都是 heap 对象。
        static LinkPtr create_NetLinkPtr(Socket &&socket,
                                         uint32 inputBufferSize = DEFAULT_INPUT_BUFFER_SIZE,
                                         uint32 outputBufferSize = DEFAULT_OUTPUT_BUFFER_SIZE);

        ~NetLink();

        void set_readCallback(ReadCallback event) { _readFun = std::move(event); };

        void set_writeCallback(WriteCallback event) { _writeFun = std::move(event); };

        void set_errorCallback(ErrorCallback event) { _errorFun = std::move(event); };

        void set_closeCallback(CloseCallback event) { _closeFun = std::move(event); };

        const Socket& socket() const { return _agent.socket; };

        [[nodiscard]] bool valid() const { return _agent.socket.valid(); };

        [[nodiscard]] int fd() const { return _agent.socket.fd(); };

    private:
        TcpMessageAgent _agent;

        ReadCallback _readFun;

        WriteCallback _writeFun;

        ErrorCallback _errorFun;

        CloseCallback _closeFun;

        friend class Controller;

        friend class Reactor;

        void handle_read(const Controller &controller);

        void handle_write(const Controller &controller);

        void handle_error(error_mark mark, const Controller &controller) const;

        void handle_close(const Controller &controller);

        bool handle_timeout(const Controller &controller) const;

        /// 强行关闭文件描述符，多次调用不会出现问题。
        void close_fd();

        explicit NetLink(Socket &&socket, uint32 inputBufferSize, uint32 outputBufferSize);

    };

}

#endif

#endif //NET_NETLINK_HPP
