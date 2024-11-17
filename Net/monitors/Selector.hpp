//
// Created by taganyer on 3/23/24.
//

#ifndef NET_SELECTOR_HPP
#define NET_SELECTOR_HPP

#include <sys/select.h>
#include "Monitor.hpp"

namespace Net {

    class Selector : public Monitor {
    public:
        ~Selector() override;

        int get_aliveEvent(int timeoutMS, EventList &list) override;

        bool add_fd(Event event) override;

        void remove_fd(int fd, bool fd_closed) override;

        void remove_all() override;

        void update_fd(Event event) override;

        [[nodiscard]] bool exist_fd(int fd) const override;

        [[nodiscard]] uint64 fd_size() const override { return _fds.size(); };

    private:
        EventList _fds;

        fd_set _read {}, _write {}, _error {};

        short read_size = 0;
        short write_size = 0;
        short error_size = 0;
        short ndfs = 0;

        void init_fd_set();

        void fill_events(EventList &list);

        EventList::iterator find_fd(int fd);

    };

}


#endif //NET_SELECTOR_HPP
