//
// Created by taganyer on 24-9-25.
//

#ifndef BALANCEDREACTOR_HPP
#define BALANCEDREACTOR_HPP

#ifdef BALANCEDREACTOR_HPP

#include <map>
#include <atomic>
#include "NetLink.hpp"
#include "Base/Condition.hpp"
#include "Base/Container/List.hpp"
#include "Base/Time/Time_difference.hpp"

namespace Base {
    class ThreadPool;
}

namespace Net {

    class NetLink;

    class Event;

    class Monitor;

    class EventLoop;

    class BalancedReactor : Base::NoCopy {
    public:
        enum MOD {
            SELECT,
            POLL,
            EPOLL
        };

        static constexpr uint32 DEFAULT_TASK_SIZE = 1024;

        BalancedReactor(MOD mod, Base::Time_difference link_timeout);

        ~BalancedReactor();

        void add_NetLink(NetLink::LinkPtr &netLink, Event event);

        void start(int monitor_timeoutMS, uint32 thread_size);

        EventLoop* get_loop() { return _loop; };

        Monitor* get_monitor() { return _monitor; };

        [[nodiscard]] bool running() const { return _running.load(std::memory_order_relaxed); };

        [[nodiscard]] bool in_reactor_thread() const;

    private:
        using TimerQueue = Base::List<std::pair<Base::Time_difference, Event>>;

        using MapData = std::pair<NetLink::WeakPtr, TimerQueue::Iter>;

        using LinkMap = std::map<int, MapData>;

        Base::Mutex _mutex;

        Base::Condition _condition;

        Base::Time_difference timeout;

        Monitor* _monitor;

        EventLoop* _loop = nullptr;

        Base::ThreadPool* _thread_pool = nullptr;

        LinkMap _map;

        TimerQueue _queue;

        std::atomic_bool _running = false;

        std::atomic_int32_t _running_tasks = 0;

        void remove_timeouts();

        void invoke(int timeoutMS, std::vector<Event> &list, Base::ThreadPool &pool);

        void create_task(Event* event, Base::ThreadPool &pool);

        void erase_link(LinkMap::iterator iter);

        void close_alive();

        friend class Controller;

        void update_link(Event event);

    };

}

#endif

#endif //BALANCEDREACTOR_HPP
