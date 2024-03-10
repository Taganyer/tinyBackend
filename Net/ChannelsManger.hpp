//
// Created by taganyer on 24-3-2.
//

#ifndef NET_CONNECTIONSMANGER_HPP
#define NET_CONNECTIONSMANGER_HPP

#include "../Base/Time/Timer.hpp"
#include "../Base/Detail/NoCopy.hpp"
#include "../Base/Mutex.hpp"

namespace Net {

    class Poller;

    class Channel;

    class EventLoop;

    class ChannelsManger : Base::NoCopy {
    public:

        friend class Node;

        ChannelsManger(EventLoop *loop, Poller *poller, pthread_t tid);

        ~ChannelsManger();

        /// 只能由 ChannelsManger 运行线程调用。
        void add_channel(Channel *channel);

        /// 只能由 ChannelsManger 运行线程调用，会先添加到 remove_head 中。
        void remove_channel(Channel *channel);

        /// 只能由 ChannelsManger 运行线程调用。
        void set_timeout(const Base::Time_difference &timeout);

        void assert_in_right_thread() const;

        /// loop() 必调函数，作用是将 prepare_head 中的 Channels 添加到queue中。
        void update();

        /// 只能由 Manager 中的 Channel 调用，作用是将传入的活跃节点放到头部。
        void put_to_top(Channel *channel);

        [[nodiscard]] pthread_t tid() const { return _tid; };

        [[nodiscard]] EventLoop *loop() const { return _loop; };

        [[nodiscard]] Poller *poller() const { return _poller; };

        [[nodiscard]] uint32 channels_size() const { return _size; };

    private:

        /// timeout <= 0 移除所有活动的 channels，以懒方式调用。
        void remove_timeout_channels();

        void remove_channels(Channel *channel);

        void remove();

        Channel *queue_head = nullptr, *queue_tail = nullptr;

        /// 主要是防止在本线程中，上层强行移除拥有活跃任务的 channel。
        Channel *remove_head = nullptr;

        EventLoop *_loop;

        Poller *_poller;

        uint32 _size = 0;

        pthread_t _tid;

        Base::Time_difference _timeout{-1};

    public:

        using ActiveChannelList = std::vector<Channel *>;

        ActiveChannelList activeList;

    };

}


#endif //NET_CONNECTIONSMANGER_HPP
