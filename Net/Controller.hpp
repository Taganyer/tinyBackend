//
// Created by taganyer on 3/24/24.
//

#ifndef NET_CONTROLLER_HPP
#define NET_CONTROLLER_HPP

#ifdef NET_CONTROLLER_HPP

#include "NetLink.hpp"

namespace Net {

    class Reactor;

    /*
     * 对相应的 NetLink 进行控制，所有操作均线程安全（不安全的操作进行了标记）。
     * 可以在任何时候调用，不会影响到对应 Reactor 的正常运行。
     * 可自由复制，并且不会出现循环引用的问题。
    */
    class Controller {
    public:
        using Weak = std::weak_ptr<NetLink>;

        using Shared = std::shared_ptr<NetLink>;

        using ReadCallback = NetLink::ReadCallback;

        using WriteCallback = NetLink::WriteCallback;

        using ErrorCallback = NetLink::ErrorCallback;

        using CloseCallback = NetLink::CloseCallback;

        using EventFun = std::function<void()>;

        Controller(const Shared &ptr, Reactor* reactor);

        /// 在 EventLoop 的线程中并且发送缓冲区无数据时会直接发送，否则写入缓冲区中，失败返回 -1。
        uint32 send(const void* target, uint32 size);

        /// 以下四个函数事件将被发送到 EventLoop 中等待生效。
        /// 后两个函数即使 NetLink 中的 FD 已经释放也会生效。
        /// 当 NetLink 被注册到多个 Reactor 时线程不安全。
        void reset_readCallback(ReadCallback event);

        void reset_writeCallback(WriteCallback event);

        void reset_errorCallback(ErrorCallback event);

        void reset_closeCallback(CloseCallback event);

        /// 如果在 EventLoop 线程中就立即生效，否则事件将被发送到 EventLoop 中等待生效。
        void update_event(Event event);

        void close_fd();

        void send_to_loop(EventFun fun);

        [[nodiscard]] bool in_loop_thread() const;

    private:
        Weak _weak;

        Reactor* _reactor;

    };

}

#endif

#endif //NET_CONTROLLER_HPP
