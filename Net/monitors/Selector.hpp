//
// Created by taganyer on 3/23/24.
//

#ifndef NET_SELECTOR_HPP
#define NET_SELECTOR_HPP

#include "Monitor.hpp"

namespace Net {

    class Selector : public Monitor {
    public:

        ~Selector() override;

        int get_aliveEvent(int timeoutMS, EventList &list) override;

        bool add_fd(Event event) override;

        void remove_fd(int fd) override;

        void remove_all() override;

        void update_fd(Event event) override;

        [[nodiscard]] uint64 fd_size() const override { return _fds.size(); };

    private:

        std::vector<Event> _fds;

        short read_size = 0;
        short write_size = 0;
        short error_size = 0;

    };

}


#endif //NET_SELECTOR_HPP
