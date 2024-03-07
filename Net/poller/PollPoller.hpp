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

        int poll(int timeoutMS, ChannelList &list) override;

        void add_channel(Channel *channel) override;

        void remove_channel(int fd) override;

        void update_channel(Channel *channel) override;

        [[nodiscard]] uint32 events_size() const override { return _channels.size(); };

    private:

        ChannelsMap _channels;

        ActiveEvents _eventsQueue;

        void get_events(ChannelList &list, int size);

    };

}


#endif //NET_POLLPOLLER_HPP
