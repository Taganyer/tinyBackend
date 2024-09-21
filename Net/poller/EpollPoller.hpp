//
// Created by taganyer on 24-3-4.
//

#ifndef NET_EPOLLPOLLER_HPP
#define NET_EPOLLPOLLER_HPP

#include <map>
#include "Poller.hpp"

struct epoll_event;

namespace Net {

    class EpollPoller : public Poller {
    public:

        using ChannelsMap = std::map<int, Channel *>;

        using ActiveEvents = std::vector<struct epoll_event>;

        EpollPoller();

        ~EpollPoller() override;

        int poll(int timeoutMS, ChannelList &list) override;

        void add_channel(Channel *channel) override;

        void remove_channel(int fd) override;

        void update_channel(Channel *channel) override;

        [[nodiscard]] uint32 events_size() const override { return _channels.size(); };

    private:

        ChannelsMap _channels;

        ActiveEvents _eventsQueue;

        int _epfd = -1;

        void operate(int operation, Channel *channel);

        void get_events(ChannelList &list, int size);

    };

}


#endif //NET_EPOLLPOLLER_HPP
