//
// Created by taganyer on 24-2-29.
//

#ifndef NET_EVENTLOOP_HPP
#define NET_EVENTLOOP_HPP

#include "../Base/Condition.hpp"
#include "../Base/Log/Log.hpp"


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

        template<typename Fun>
        void put_events(int32 size, Fun fun);

        void assert_in_thread() const;

        [[nodiscard]] bool object_in_thread() const { return Base::tid() == _tid; };

        [[nodiscard]] bool running() const { return run; };

    private:

        bool run = false;

        bool quit = false;

        pthread_t _tid = Base::tid();

        Event _distributor;

        EventsQueue _queue, _waiting;

        Base::Mutex _mutex;

        Base::Condition _condition;

        void loop_begin();

        void loop_end();

    };

    template<typename Fun>
    void EventLoop::put_events(int32 size, Fun fun) {
        assert(size >= 0 && !quit);
        Base::Lock l(_mutex);
        _waiting.reserve(size + _waiting.size());
        for (int32 i = 0; i < size; ++i) {
            _waiting.push_back(fun());
        }
        G_TRACE << "put" << size << "event in EventLoop "
                << _tid << " at " << Base::thread_name();
        _condition.notify_one();
    }

}


#endif //NET_EVENTLOOP_HPP
