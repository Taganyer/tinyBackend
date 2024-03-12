//
// Created by taganyer on 24-3-5.
//

#ifndef NET_TCPLINK_HPP
#define NET_TCPLINK_HPP

#include "../Base/Detail/config.hpp"
#include "../Base/RingBuffer.hpp"
#include "../Base/Time/Timer.hpp"
#include "Socket.hpp"
#include <functional>
#include <memory>

namespace Net {

    class Channel;

    class ChannelsManger;

    class NetLink;

    struct error_mark;

    namespace Detail {

        class LinkData {
        public:

            Socket socket;

            Channel *_channel = nullptr;

            using ReadCallback = std::function<void(Base::RingBuffer &)>;

            using WriteCallback = std::function<void(Base::RingBuffer &)>;

            using ErrorCallback = std::function<void(error_mark)>;

            using CloseCallback = std::function<void(error_mark)>;

            explicit LinkData(Socket &&s) : socket(std::move(s)) {};

            void handle_read();

            void handle_write();

            void handle_error(int error_code);

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

        NetLink(Socket &&socket, ChannelsManger &manger);

        ~NetLink();

        /// NOTE 在 channel 的线程中并且发送缓冲区无数据时会直接发送，多线程不安全。
        uint32 send(const void *target, uint32 size);

        uint64 send_file();

        void set_channel_read(bool turn_on);

        void set_channel_write(bool turn_on);

        void shutdown_write();

        void force_close();

        void set_readCallback(ReadCallback event) { _data->_readFun = std::move(event); };

        void set_writeCallback(WriteCallback event) { _data->_writeFun = std::move(event); };

        void set_errorCallback(ErrorCallback event) { _data->_errorFun = std::move(event); };

        void set_closeCallback(CloseCallback event) { _data->_closeFun = std::move(event); };

        [[nodiscard]] bool valid() const { return _data.operator bool() && _data->_channel; };

        [[nodiscard]] int fd() const { return _data->socket.fd(); };

        /// TODO 提供给用户使用，所有关于 channel 的调用必须在对应的 EventLoop 线程中。
        [[nodiscard]] Channel *channel() const { return _data->_channel; };

    private:

        using LinkData = Detail::LinkData;

        using DataPtr = std::shared_ptr<LinkData>;

        DataPtr _data;

        void send_to_loop(std::function<void()> event);

        [[nodiscard]] bool in_loop_thread() const;

    };

}

#endif //NET_TCPLINK_HPP
