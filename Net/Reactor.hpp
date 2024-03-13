//
// Created by taganyer on 3/8/24.
//

#ifndef NET_RECTOR_HPP
#define NET_RECTOR_HPP

#include "../Base/Detail/config.hpp"
#include "../Base/Detail/NoCopy.hpp"
#include "EventLoop.hpp"
#include "ChannelsManger.hpp"

namespace Net {

    class NetLink;

    class Channel;

    class Poller;

    class FileDescriptor;

    class Reactor : Base::NoCopy {
    public:

        Reactor(bool epoll);

        ~Reactor();

        void add_NetLink(NetLink &netLink);

        void start(int poller_timeoutMS, int channel_timeoutS);

        void close();

        EventLoop *get_loop() { return _loop; };

        [[nodiscard]] bool running() const { return _loop; };

    private:

        Poller *_poller;

        EventLoop *_loop = nullptr;

        ChannelsManger *_manger = nullptr;

    };

}


#endif //NET_RECTOR_HPP
