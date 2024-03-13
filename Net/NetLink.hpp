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

    namespace Detail {

        class LinkData {
        public:

            std::unique_ptr<FileDescriptor> FD;

            Channel *_channel = nullptr;

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

            explicit LinkData(FdPtr &&Fd) : FD(std::move(Fd)) {};

            void handle_read();

            void handle_write();

            bool handle_error();

            void handle_close();

            bool handle_timeout();

        private:

            ReadCallback _readFun;

            WriteCallback _writeFun;

            ErrorCallback _errorFun;

            CloseCallback _closeFun;

            Base::RingBuffer _input;

            Base::RingBuffer _output;

            friend class Net::NetLink;

        };

    }


    class NetLink {
    public:

        using ReadCallback = Detail::LinkData::ReadCallback;

        using WriteCallback = Detail::LinkData::WriteCallback;

        using ErrorCallback = Detail::LinkData::ErrorCallback;

        using CloseCallback = Detail::LinkData::CloseCallback;

        using FdPtr = Detail::LinkData::FdPtr;

        NetLink(FdPtr &&Fd);

        ~NetLink();

        /// NOTE 在 channel 的线程中并且发送缓冲区无数据时会直接发送，多线程不安全。
        uint32 send(const void *target, uint32 size);

        /// FIXME
        uint64 send_file();

        void channel_read(bool turn_on);

        void channel_write(bool turn_on);

        void read_event(bool turn_on);

        void write_event(bool turn_on);

        void error_event(bool turn_on);

        void close_event(bool turn_on);

        void wake_up_event();

        void force_close_link();

        void send_to_loop(std::function<void()> event);

        void set_readCallback(ReadCallback event) { _data->_readFun = std::move(event); };

        void set_writeCallback(WriteCallback event) { _data->_writeFun = std::move(event); };

        void set_errorCallback(ErrorCallback event) { _data->_errorFun = std::move(event); };

        void set_closeCallback(CloseCallback event) { _data->_closeFun = std::move(event); };

        [[nodiscard]] bool valid() const { return _data.operator bool(); };

        [[nodiscard]] int fd() const { return _data->FD->fd(); };

        [[nodiscard]] bool has_channel() const { return _data.operator bool() && _data->_channel; };

        [[nodiscard]] std::weak_ptr<Detail::LinkData> get_data() const { return _data; };

    private:

        using LinkData = Detail::LinkData;

        using DataPtr = std::shared_ptr<LinkData>;

        DataPtr _data;

        [[nodiscard]] bool in_loop_thread() const;

    };

}

#endif //NET_TCPLINK_HPP
