//
// Created by taganyer on 24-3-4.
//

#ifndef NET_EPOLLPOLLER_HPP
#define NET_EPOLLPOLLER_HPP

#include <map>
#include <vector>
#include "../Poller.hpp"

struct epoll_event;

namespace Net {

    class EpollPoller : public Poller {
    public:

        using ChannelsMap = std::map<int, Channel *>;

        using ActiveEvents = std::vector<struct epoll_event>;

        EpollPoller();

        ~EpollPoller() override;

        void poll(int timeoutMS) override;

        Channel *get_events() override;

        void add_channel(ConnectionsManger *manger, Channel *channel) override;

        void remove_channel(ConnectionsManger *manger, int fd) override;

        void update_channel(Channel *channel) override;

        [[nodiscard]] uint32 events_size() const override { return _channels.size(); };

        [[nodiscard]] uint32 active_events_size() const override { return _end - _begin; };

    private:

        ChannelsMap _channels;

        ActiveEvents _eventsQueue;

        int _epfd = -1;

        int _begin = 0, _end = 0;

        void operate(int operation, Channel *channel);

    };

}


#endif //NET_EPOLLPOLLER_HPP
