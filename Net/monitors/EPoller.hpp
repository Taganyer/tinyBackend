//
// Created by taganyer on 3/23/24.
//

#ifndef NET_EPOLLER_HPP
#define NET_EPOLLER_HPP

#include <set>
#include "Monitor.hpp"

struct epoll_event;

namespace Net {

    class EPoller : public Monitor {
    public:

        using ActiveEvents = std::vector<epoll_event>;

        EPoller();

        ~EPoller() override;

        int get_aliveEvent(int timeoutMS, EventList &list) override;

        bool add_fd(Event event) override;

        void remove_fd(int fd) override;

        void remove_all() override;

        void update_fd(Event event) override;

        [[nodiscard]] uint64 fd_size() const override { return _fds.size(); };

    private:

        ActiveEvents activeEvents;

        std::set<int> _fds;

        int _epfd = -1;

        void get_events(EventList &list, int size);

        bool operate(int mod, int fd, int events);

    };

}


#endif //NET_EPOLLER_HPP
