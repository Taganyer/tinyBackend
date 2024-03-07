//
// Created by taganyer on 24-3-5.
//

#ifndef NET_TCPLINK_HPP
#define NET_TCPLINK_HPP

#include "../Base/Detail/config.hpp"
#include "../Base/RingBuffer.hpp"
#include "../Base/Time/Timer.hpp"
#include <functional>
#include <memory>

namespace Net {

    class Channel;

    class EventLoop;

    namespace Detail {

        class LinkData {
        public:

            using ReadCallback = std::function<void()>;

            using WriteCallback = std::function<void()>;

            using ErrorCallback = std::function<void()>;

            using CloseCallback = std::function<void()>;

            void set_readCallback(ReadCallback event) { _readFun = std::move(event); };

            void set_writeCallback(WriteCallback event) { _writeFun = std::move(event); };

            void set_errorCallback(ErrorCallback event) { _errorFun = std::move(event); };

            void set_closeCallback(CloseCallback event) { _closeFun = std::move(event); };

            void handle_read(Channel &channel);

            void handle_write(Channel &channel);

            void handle_error(Channel &channel);

            void handle_close(Channel &channel);

            bool handle_timeout();

        private:

            ReadCallback _readFun;

            WriteCallback _writeFun;

            ErrorCallback _errorFun;

            CloseCallback _closeFun;

            Base::RingBuffer _input;

            Base::RingBuffer _output;

        };

    }


    class NetLink {
    public:

        NetLink(int fd, EventLoop *loop);

        void send(const char *target, uint32 size);


    private:

        using LinkData = Detail::LinkData;

        using DataPtr = std::shared_ptr<LinkData>;

        const int _fd = -1;

        int32 state = 0;

        EventLoop *_loop;

        DataPtr _data;

        friend class Channel;

    };

}

#endif //NET_TCPLINK_HPP
