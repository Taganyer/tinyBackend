//
// Created by taganyer on 24-10-16.
//

#ifndef NET_REACTOR_HPP
#define NET_REACTOR_HPP

#ifdef NET_REACTOR_HPP

#include <atomic>
#include <map>
#include <memory>
#include "tinyBackend/Base/Thread.hpp"
#include "tinyBackend/Base/Container/List.hpp"
#include "tinyBackend/Base/Time/TimeInterval.hpp"
#include "tinyBackend/Net/Channel.hpp"
#include "tinyBackend/Net/MessageAgent.hpp"

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

        using FixedFun = std::function<void()>;

        using WeakUpFun = std::function<void(MessageAgent &, Channel &)>;

        using MessageAgentPtr = std::unique_ptr<MessageAgent>;

        explicit Reactor(Base::TimeInterval link_timeout) : timeout(link_timeout) {};

        ~Reactor() { stop(); };

        void start(MOD mod, int monitor_timeoutMS, FixedFun fun = FixedFun());

        void stop();

        void add_channel(MessageAgentPtr &&ptr, Channel channel, Event monitor_event);

        void send_to_loop(std::function<void()> fun) const;

        void update_channel(Event monitor_event);

        void weak_up_channel(int fd, WeakUpFun fun);

        [[nodiscard]] bool in_reactor_thread() const;

        [[nodiscard]] bool running() const { return _running.load(std::memory_order_acquire); };

    private:
        struct TimerData {
            Base::TimeInterval flush_time;
            int fd;

            explicit TimerData(int fd) : flush_time(Base::Unix_to_now()), fd(fd) {};
        };

        using TimerQueue = Base::List<TimerData>;

        struct ChannelData {
            MessageAgentPtr agent;
            Channel channel;
            TimerQueue::Iter timer_iter;
            Event monitor_event;

            ChannelData(MessageAgentPtr &&ag, Channel &&ch, Event event) :
                agent(std::move(ag)), channel(std::move(ch)), monitor_event(event) {};

            ChannelData(ChannelData &&) = default;
        };

        using ChannelMap = std::map<int, ChannelData>;

        Monitor* _monitor = nullptr;

        EventLoop* _loop = nullptr;

        ChannelMap _map;

        TimerQueue _queue;

        Base::TimeInterval timeout;

        Base::Thread _thread;

        std::atomic_bool _running = false;

        void create_source(MOD mod);

        void destroy_source();

        void remove_timeouts();

        void invoke(int timeoutMS, std::vector<Event> &list);

        void create_task(Event* event);

        void erase_channel(ChannelMap::iterator iter);

        void close_alive();

        friend class Controller;

    };

}

#endif

#endif //NET_REACTOR_HPP
