//
// Created by taganyer on 3/8/24.
//

#ifndef NET_RECTOR_HPP
#define NET_RECTOR_HPP

#ifdef NET_RECTOR_HPP

#include <map>
#include "NetLink.hpp"
#include "Base/Loop/EventLoop.hpp"
#include "Base/Container/List.hpp"
#include "Base/Time/Time_difference.hpp"

namespace Net {

    class NetLink;

    class Event;

    class Monitor;

    class Reactor : Base::NoCopy {
    public:
        enum MOD {
            SELECT,
            POLL,
            EPOLL
        };

        Reactor(MOD mod, Base::Time_difference link_timeout);

        ~Reactor();

        void add_NetLink(NetLink::LinkPtr &netLink, Event event);

        void start(int monitor_timeoutMS);

        void close();

        Base::EventLoop* get_loop() { return _loop; };

        Monitor* get_monitor() { return _monitor; };

        [[nodiscard]] bool running() const { return _running; };

    private:
        using TimerQueue = Base::List<std::pair<Base::Time_difference, Event>>;

        using MapData = std::pair<NetLink::WeakPtr, TimerQueue::Iter>;

        using LinkMap = std::map<int, MapData>;

        Base::Time_difference timeout;

        Monitor* _monitor;

        Base::EventLoop* _loop = nullptr;

        LinkMap _map;

        TimerQueue _queue;

        volatile bool _running = false;

        void remove_timeouts();

        void invoke(int timeoutMS, std::vector<Event> &list);

        friend class Controller;

    };

}

#endif

#endif //NET_RECTOR_HPP
