//
// Created by taganyer on 24-10-16.
//

#ifndef NET_REACTOR_HPP
#define NET_REACTOR_HPP

#ifdef NET_REACTOR_HPP

#include <map>
#include <atomic>
#include "Base/Thread.hpp"
#include "Net/NetLink.hpp"
#include "Net/monitors/Event.hpp"
#include "Base/Container/List.hpp"
#include "Base/Time/Time_difference.hpp"

namespace Net {

    class Monitor;

    class EventLoop;

    class Reactor : Base::NoCopy {
    public:
        enum MOD {
            SELECT,
            POLL,
            EPOLL
        };

        explicit Reactor(Base::Time_difference link_timeout) : timeout(link_timeout) {};

        ~Reactor() { stop(); };

        using FixedFun = std::function<void()>;

        void start(MOD mod, int monitor_timeoutMS, FixedFun fun = FixedFun());

        void stop();

        void add_NetLink(NetLink::LinkPtr &netLink, Event event);

        void remove_NetLink(int fd);

        void send_to_loop(std::function<void()> fun) const;

        void update_link(Event event);

        using WeakUpFun = std::function<void(const Controller &)>;

        void weak_up_link(int fd, WeakUpFun fun);

        void weak_up_all_link(WeakUpFun fun);

        [[nodiscard]] bool in_reactor_thread() const;

        [[nodiscard]] bool running() const { return _running.load(std::memory_order_acquire); };

    private:
        struct TimerData {
            Base::Time_difference flush_time;
            int fd;

            explicit TimerData(int fd) : flush_time(Base::Unix_to_now()), fd(fd) {};
        };

        using TimerQueue = Base::List<TimerData>;

        struct LinkData {
            NetLink::LinkPtr link;
            TimerQueue::Iter timer_iter;
            Event event;

            LinkData(NetLink::LinkPtr link, Event event) :
                link(std::move(link)), event(event) {};
        };

        using LinkMap = std::map<int, LinkData>;

        Monitor* _monitor = nullptr;

        EventLoop* _loop = nullptr;

        LinkMap _map;

        TimerQueue _queue;

        Base::Time_difference timeout;

        Base::Thread _thread;

        std::atomic_bool _running = false;

        void create_source(MOD mod);

        void destroy_source();

        void remove_timeouts();

        void invoke(int timeoutMS, std::vector<Event> &list);

        void create_task(Event* event);

        void erase_link(LinkMap::iterator iter);

        void close_alive();

        friend class Controller;

    };

}

#endif

#endif //NET_REACTOR_HPP
