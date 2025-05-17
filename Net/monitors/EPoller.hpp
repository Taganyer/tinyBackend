//
// Created by taganyer on 3/23/24.
//

#ifndef NET_EPOLLER_HPP
#define NET_EPOLLER_HPP

#include <map>
#include "Monitor.hpp"

struct epoll_event;

namespace Net {

    class EPoller : public Monitor {
    public:
        using ActiveEvents = std::vector<epoll_event>;

        EPoller();

        ~EPoller() override;

        int get_aliveEvent(int timeoutMS, EventList& list) override;

        bool add_fd(Event event) override;

        void remove_fd(int fd, bool fd_closed) override;

        void remove_all() override;

        /// 设置为 NoEvent 会直接删除 fd;
        void update_fd(Event event) override;

        [[nodiscard]] bool exist_fd(int fd) const override;

        [[nodiscard]] uint64 fd_size() const override { return _fds.size(); };

    private:
        ActiveEvents activeEvents;

        std::map<int, Event> _fds;

        int _epfd = -1;

        void get_events(EventList& list, int size);

        bool operate(int mod, Event *event);

    };

}


#endif //NET_EPOLLER_HPP
