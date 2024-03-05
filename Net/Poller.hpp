//
// Created by taganyer on 24-2-29.
//

#ifndef NET_POLLER_HPP
#define NET_POLLER_HPP

#include <pthread.h>
#include "../Base/Detail/config.hpp"

namespace Net {

    class Channel;

    class EventLoop;

    class ConnectionsManger;

    class Poller {
    public:

        static constexpr int UntilEventOccur = -1;

        static constexpr int Immediately = 0;

        virtual ~Poller() = default;

        virtual void poll(int timeoutMS) = 0;

        /// 逐个返回活跃事件，无活跃事件返回 nullptr。
        virtual Channel *get_events() = 0;

        /// 只能由所属同一个线程的 EventLoop 调用。
        virtual void add_channel(ConnectionsManger *manger, Channel *channel) = 0;

        /// 只能由所属同一个线程的 EventLoop 调用。
        virtual void remove_channel(ConnectionsManger *manger, int fd) = 0;

        /// 改变 channel 的读写状态。
        virtual void update_channel(Channel *channel) = 0;

        void set_tid(pthread_t tid) { _tid = tid; };

        void set_EventLoop(EventLoop *loop) { _loop = loop; };

        void assert_in_right_thread(const char *message) const;

        [[nodiscard]] virtual uint32 events_size() const = 0;

        [[nodiscard]] virtual uint32 active_events_size() const = 0;

        [[nodiscard]] pthread_t tid() const { return _tid; };

        [[nodiscard]] EventLoop *eventLoop() const { return _loop; };

    protected:

        pthread_t _tid;

        EventLoop *_loop;

    };

}


#endif //NET_POLLER_HPP
