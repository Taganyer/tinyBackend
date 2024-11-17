//
// Created by taganyer on 3/24/24.
//

#ifndef NET_CONTROLLER_HPP
#define NET_CONTROLLER_HPP

#ifdef NET_CONTROLLER_HPP

#include <cassert>
#include <cerrno>
#include "error/error_mark.hpp"
#include "Net/monitors/Event.hpp"
#include "NetLink.hpp"


namespace Net {

    class EventLoop;

    class Reactor;

    /*
     * 对相应的 NetLink 进行控制。
     * 全部的方法将会在 EventLoop 线程中调用。
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

        Controller(const Controller &) = delete;

        /// 发送缓冲区无数据时会直接发送，否则写入缓冲区中，失败返回 -1。
        int64 send(const void* target, uint32 size, bool invoke_write_fun = true) const;

        template <std::size_t N>
        int64 send(const Base::BufferArray<N> &target, bool invoke_write_fun = true) const;

        /// 将数据写入缓冲区，不会直接发送。
        uint32 indirect_send(const void* target, uint32 size,
                             bool fixed, bool invoke_write_fun = false) const;

        template <std::size_t N>
        uint32 indirect_send(const Base::BufferArray<N> &target,
                             bool fixed, bool invoke_write_fun = false) const;

        /// 以下四个函数事件将被发送到 EventLoop 中等待生效。
        void reset_readCallback(ReadCallback event) const;

        void reset_writeCallback(WriteCallback event) const;

        void reset_errorCallback(ErrorCallback event) const;

        void reset_closeCallback(CloseCallback event) const;

        /// 如果在 EventLoop 线程中就立即生效，否则事件将被发送到 EventLoop 中等待生效。
        void update_event(Event event) const;

        void send_to_loop(EventFun fun) const;

        /// 可以改变当前的事件。
        [[nodiscard]] Event& current_event() const;

        [[nodiscard]] Event registered_event() const;

        [[nodiscard]] const Socket& socket() const { return _link.socket(); };

        [[nodiscard]] int fd() const { return _link.fd(); };

        [[nodiscard]] Base::RingBuffer& input_buffer() const { return _link._agent.input; };

        [[nodiscard]] const Base::RingBuffer& output_buffer() const { return _link._agent.output; };

    private:
        NetLink &_link;

        Event &_event;

        Reactor &_reactor;

        int _registered_event;

        friend class Reactor;

        Controller(NetLink &link, Event &event, Reactor &reactor, int registered_event);

        [[nodiscard]] bool in_loop_thread() const;

    };

    template <std::size_t N>
    int64 Controller::send(const Base::BufferArray<N> &target, bool invoke_write_fun) const {
        assert(in_loop_thread());

        int64 written = _link._agent.direct_send(target);
        if (written < 0) {
            _link.handle_error({ error_types::Write, errno }, *this);
        } else if (invoke_write_fun && _link._writeFun) {
            _link._writeFun(*this);
        }
        return written;
    }

    template <std::size_t N>
    uint32 Controller::indirect_send(const Base::BufferArray<N> &target,
                                     bool fixed, bool invoke_write_fun) const {
        assert(in_loop_thread());

        uint32 written = _link._agent.indirect_send(target, fixed);
        if (invoke_write_fun && _link._writeFun)
            _link._writeFun(*this);
        return written;
    }

}

#endif

#endif //NET_CONTROLLER_HPP
