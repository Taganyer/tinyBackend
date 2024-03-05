//
// Created by taganyer on 24-3-4.
//

#ifndef NET_POLLPOLLER_HPP
#define NET_POLLPOLLER_HPP

#include <map>
#include <vector>
#include "../Poller.hpp"

struct pollfd;

namespace Net {

    class PollPoller : public Poller {
    public:

        using ChannelsMap = std::map<int, std::pair<Channel *, int>>;

        using ActiveEvents = std::vector<struct pollfd>;

        void poll(int timeoutMS) override;

        Channel *get_events() override;

        void add_channel(ConnectionsManger *manger, Channel *channel) override;

        void remove_channel(ConnectionsManger *manger, int fd) override;

        void update_channel(Channel *channel) override;

        [[nodiscard]] uint32 events_size() const override { return _channels.size(); };

        [[nodiscard]] uint32 active_events_size() const override { return _active_size; };

    private:

        ChannelsMap _channels;

        ActiveEvents _eventsQueue;

        /// 用来记录下一个活跃事件在 _eventsQueue 中的位置。
        uint32 _index = 0;

        uint32 _active_size = 0;

    };

}


#endif //NET_POLLPOLLER_HPP
