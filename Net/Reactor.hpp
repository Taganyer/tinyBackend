//
// Created by taganyer on 3/8/24.
//

#ifndef NET_RECTOR_HPP
#define NET_RECTOR_HPP

#include <map>
#include <list>
#include "NetLink.hpp"
#include "../Base/Detail/config.hpp"
#include "../Base/Detail/NoCopy.hpp"
#include "../Base/Time/Timer.hpp"

namespace Net {

    class NetLink;

    class Event;

    class Monitor;

    class Poller;

    class EventLoop;

    class Reactor : Base::NoCopy {
    public:

        Reactor(bool epoll, Base::Time_difference link_timeout);

        ~Reactor();

        void add_NetLink(NetLink &netLink, Event event);

        void start(int monitor_timeoutMS);

        void close();

        EventLoop *get_loop() { return _loop; };

        Monitor *get_monitor() { return _monitor; };

        [[nodiscard]] bool running() const { return _loop; };

    private:

        using TimerQueue = std::list<std::pair<Base::Time_difference, Event>>;

        using MapData = std::pair<NetLink::WeakPtr, TimerQueue::iterator>;

        using LinkMap = std::map<int, MapData>;

        Base::Time_difference timeout;

        Monitor *_monitor;

        EventLoop *_loop = nullptr;

        LinkMap _map;

        TimerQueue _queue;

        void remove_timeouts();

        void invoke(int timeoutMS, std::vector<Event> &list);

        friend class Controller;

    };

}

#endif //NET_RECTOR_HPP
