//
// Created by taganyer on 24-2-29.
//

#ifndef NET_EVENTLOOP_HPP
#define NET_EVENTLOOP_HPP

#ifdef NET_EVENTLOOP_HPP

#include "Base/Condition.hpp"

namespace Net {

    class EventLoop {
    public:

        using Event = std::function<void()>;

        using EventsQueue = std::vector<Event>;

        EventLoop();

        ~EventLoop();

        void loop();

        void shutdown();

        /// 无锁保护，注意。
        void set_distributor(Event event);

        void put_event(Event event);

        void weak_up();

        void assert_in_thread() const;

        [[nodiscard]] bool object_in_thread() const {
            return Base::CurrentThread::tid() == _tid;
        };

        [[nodiscard]] bool looping() const { return _run; };

    private:

        volatile bool _run = false,  _quit = false;

        bool _weak = false;

        pthread_t _tid = Base::CurrentThread::tid();

        Event _distributor;

        EventsQueue _queue, _waiting;

        Base::Mutex _mutex;

        Base::Condition _condition;

        void loop_begin();

        void loop_end();

    };

}

#endif

#endif //NET_EVENTLOOP_HPP
