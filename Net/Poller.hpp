//
// Created by taganyer on 24-2-29.
//

#ifndef NET_POLLER_HPP
#define NET_POLLER_HPP

#include <pthread.h>
#include <vector>
#include "../Base/Detail/config.hpp"

namespace Net {

    class Channel;

    class EventLoop;

    class ChannelsManger;

    class Poller {
    public:

        using ChannelList = std::vector<Channel *>;

        static constexpr int UntilEventOccur = -1;

        static constexpr int Immediately = 0;

        virtual ~Poller() = default;

        virtual int poll(int timeoutMS, ChannelList &list) = 0;

        /// 由所属同一个线程的 ChannelsManager 调用。
        virtual void add_channel(Channel *channel) = 0;

        /// 由所属同一个线程的 ChannelsManger 调用,直接调用可能会带来错误。
        virtual void remove_channel(int fd) = 0;

        /// 改变 channel 的读写状态。
        virtual void update_channel(Channel *channel) = 0;

        /// TODO 只在初始化时调用
        void set_tid(pthread_t tid) { _tid = tid; };

        void assert_in_right_thread(const char *message) const;

        [[nodiscard]] pthread_t tid() const { return _tid; };

        [[nodiscard]] virtual uint32 events_size() const = 0;

    protected:

        pthread_t _tid;

    };

}


#endif //NET_POLLER_HPP
