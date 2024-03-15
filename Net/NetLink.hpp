//
// Created by taganyer on 24-3-5.
//

#ifndef NET_TCPLINK_HPP
#define NET_TCPLINK_HPP

#include "../Base/Detail/config.hpp"
#include "../Base/RingBuffer.hpp"
#include "../Base/Time/Timer.hpp"
#include "FileDescriptor.hpp"
#include <functional>
#include <memory>

namespace Net {

    class EventLoop;

    class Channel;

    class ChannelsManger;

    class NetLink;

    struct error_mark;

    class NetLink : public std::enable_shared_from_this<NetLink>, private Base::NoCopy {
    public:

        using ReadCallback = std::function<void(Base::RingBuffer &)>;

        using WriteCallback = std::function<void(Base::RingBuffer &)>;

        /*
         * 返回值表示是否调用 CloseCallback
         * 可能返回的错误有：
         *                error_type::Read,
         *                error_type::Write,
         *                error_type::Epoll_ctl,
         *                error_type::Link_ErrorEvent,
         *                error_type::Link_TimeoutEvent
        */
        using ErrorCallback = std::function<bool(error_mark)>;

        using CloseCallback = std::function<void()>;

        using FdPtr = std::unique_ptr<FileDescriptor>;

        using LinkPtr = std::shared_ptr<NetLink>;

        /// 隐藏了 NetLink 的构造函数，确保生成的对象都是 heap 对象。
        static LinkPtr create_NetLinkPtr(FdPtr &&Fd);

        ~NetLink();

        void channel_read(bool turn_on);

        void channel_write(bool turn_on);

        void wake_up_event();

        void force_close_link();

        void send_to_loop(std::function<void()> event);

        void set_readCallback(ReadCallback event) { _readFun = std::move(event); };

        void set_writeCallback(WriteCallback event) { _writeFun = std::move(event); };

        void set_errorCallback(ErrorCallback event) { _errorFun = std::move(event); };

        void set_closeCallback(CloseCallback event) { _closeFun = std::move(event); };

        [[nodiscard]] bool valid() const { return _channel && FD; };

        [[nodiscard]] int fd() const { return FD->fd(); };

        [[nodiscard]] bool has_channel() const { return _channel; };

    private:

        std::unique_ptr<FileDescriptor> FD;

        Channel *_channel = nullptr;

        ReadCallback _readFun;

        WriteCallback _writeFun;

        ErrorCallback _errorFun;

        CloseCallback _closeFun;

        Base::RingBuffer _input;

        Base::RingBuffer _output;

        friend class Controller;

        friend class Channel;

        void handle_read();

        void handle_write();

        bool handle_error();

        void handle_close();

        bool handle_timeout();

        bool in_loop_thread() const;


        NetLink(FdPtr &&Fd) : FD(std::move(Fd)) {};

    };


    class Controller {
    public:
        using Weak = std::weak_ptr<NetLink>;

        using Shared = std::shared_ptr<NetLink>;

        using ReadCallback = NetLink::ReadCallback;

        using WriteCallback = NetLink::WriteCallback;

        using ErrorCallback = NetLink::ErrorCallback;

        using CloseCallback = NetLink::CloseCallback;

        Controller(const Shared &ptr);

        /// NOTE 在 channel 的线程中并且发送缓冲区无数据时会直接发送，多线程不安全，失败返回 -1。
        uint32 send(const void *target, uint32 size);

        /// FIXME
        uint64 send_file();

        /// 以下四个函数并不安全，合理使用。
        bool reset_readCallback(ReadCallback event);

        bool reset_writeCallback(WriteCallback event);

        bool reset_errorCallback(ErrorCallback event);

        bool reset_closeCallback(CloseCallback event);

        bool channel_read(bool turn_on);

        bool channel_write(bool turn_on);

        bool read_event(bool turn_on);

        bool write_event(bool turn_on);

        bool error_event(bool turn_on);

        bool close_event(bool turn_on);

        bool wake_up_event();

    private:

        Weak _weak;

    };

}

#endif //NET_TCPLINK_HPP
