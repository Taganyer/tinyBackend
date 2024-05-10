//
// Created by taganyer on 3/24/24.
//

#ifndef NET_CONTROLLER_HPP
#define NET_CONTROLLER_HPP

#ifdef NET_CONTROLLER_HPP

#include "NetLink.hpp"
#include "monitors/Event.hpp"

namespace Net {

    class Reactor;

    /// 安全的控制 NetLink,可自由复制，不会出现循环引用的问题。
    class Controller {
    public:
        using Weak = std::weak_ptr<NetLink>;

        using Shared = std::shared_ptr<NetLink>;

        using ReadCallback = NetLink::ReadCallback;

        using WriteCallback = NetLink::WriteCallback;

        using ErrorCallback = NetLink::ErrorCallback;

        using CloseCallback = NetLink::CloseCallback;

        using EventFun = std::function<void()>;

        Controller(const Shared &ptr, Reactor *reactor);

        /// NOTE 在 EventLoop 的线程中并且发送缓冲区无数据时会直接发送，多线程不安全，失败返回 -1。
        uint32 send(const void *target, uint32 size);

        /// 以下四个函数不会立即生效并且线程不安全，合理使用。
        bool reset_readCallback(ReadCallback event);

        bool reset_writeCallback(WriteCallback event);

        bool reset_errorCallback(ErrorCallback event);

        bool reset_closeCallback(CloseCallback event);

        bool set_read(bool turn_on);

        bool set_write(bool turn_on);

        bool wake_readCallback(bool after = true);

        bool wake_writeCallback(bool after = true);

        bool wake_error(error_mark mark, bool after = true);

        bool wake_close(bool after = true);

        void close_fd();

        void send_to_loop(EventFun fun);

        [[nodiscard]] bool in_loop_thread() const;

    private:

        Weak _weak;

        Reactor *_reactor;

    };

}

#endif

#endif //NET_CONTROLLER_HPP
