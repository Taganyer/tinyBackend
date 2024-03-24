//
// Created by taganyer on 3/23/24.
//

#ifndef NET_POLLER_HPP
#define NET_POLLER_HPP

#include <map>
#include <sys/poll.h>
#include "Monitor.hpp"

struct pollfd;

namespace Net {

    class Poller : public Monitor {
    public:

        using ActiveEvents = std::vector<struct pollfd>;

        ~Poller() override;

        int get_aliveEvent(int timeoutMS, EventList &list) override;

        bool add_fd(Event event) override;

        void remove_fd(int fd) override;

        void remove_all() override;

        void update_fd(Event event) override;

        [[nodiscard]] uint64 fd_size() const override;

    private:

        ActiveEvents _fds;

        std::map<int, int> _mapping;

        void get_events(EventList &list, int size);

    };

}


#endif //NET_POLLER_HPP
