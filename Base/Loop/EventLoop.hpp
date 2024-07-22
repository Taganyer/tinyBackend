//
// Created by taganyer on 24-2-29.
//

#ifndef BASE_EVENTLOOP_HPP
#define BASE_EVENTLOOP_HPP

#ifdef BASE_EVENTLOOP_HPP

#include "Base/Condition.hpp"
#include "Base/Log/SystemLog.hpp"

namespace Base {

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

        template <typename Fun>
        void put_events(int32 size, Fun fun);

        void weak_up();

        void assert_in_thread() const;

        [[nodiscard]] bool object_in_thread() const { return CurrentThread::tid() == _tid; };

        [[nodiscard]] bool looping() const { return _run; };

    private:
        volatile bool _run = false, _quit = false;

        bool _weak = false;

        pthread_t _tid = CurrentThread::tid();

        Event _distributor;

        EventsQueue _queue, _waiting;

        Mutex _mutex;

        Condition _condition;

        void loop_begin();

        void loop_end();

    };

    template <typename Fun>
    void EventLoop::put_events(int32 size, Fun fun) {
        assert(size >= 0 && !_quit);
        Lock l(_mutex);
        _waiting.reserve(size + _waiting.size());
        for (int32 i = 0; i < size; ++i) {
            _waiting.push_back(fun());
        }
        G_TRACE << "put " << size << " events in EventLoop "
                << CurrentThread::thread_name();
        _condition.notify_one();
    }

}

#endif

#endif //BASE_EVENTLOOP_HPP
