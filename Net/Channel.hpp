//
// Created by taganyer on 24-2-29.
//

#ifndef NET_CHANNEL_HPP
#define NET_CHANNEL_HPP

#include <memory>
#include <sys/poll.h>
#include "../Base/Detail/config.hpp"
#include "../Base/Detail/NoCopy.hpp"
#include "../Base/Time/Timer.hpp"

namespace Net {

    class Poller;

    class ChannelsManger;

    namespace Detail {

        class LinkData;

    }

    class Channel : private Base::NoCopy {
    public:

        using Data = Detail::LinkData;

        using ChannelPtr = Channel *;

        /// 隐藏了构造函数，保证生成的对象都是 heap 对象。
        static ChannelPtr create_Channel(int fd, const std::shared_ptr<Data> &data, ChannelsManger &manger);

        static void destroy_Channel(Channel *channel);

        ~Channel();

        void invoke();

        void enable_read();

        void enable_write();

        void disable_read();

        void disable_write();

        void set_nonevent();

        void set_events(short events);

        /// 提供给 Poller 使用。
        void set_revents(short revents) { _revents = revents; };

        /// 提供给 ChannelsManager 使用。
        bool timeout(const Base::Time_difference &time);

        void remove_this();

        /// 会解除监控，但在下一次 loop 循环中，会继续调用。
        void continue_events();

        /// TODO 以下九个函数提供给上层对象完成 invoke 中应有的事件后调用，invoke 不会主动调用。
        void set_readRevents() { _revents |= POLLIN; };

        void set_writeRevents() { _revents |= POLLOUT; };

        void set_errorRevents() { _revents |= POLLERR; };

        void set_closeRevents() { _revents |= POLLHUP; };

        void finish_readRevents() { _revents &= ~POLLIN; };

        void finish_writeRevents() { _revents &= ~POLLOUT; };

        void finish_errorRevents() { _revents &= ~(POLLERR | POLLNVAL); };

        void finish_closeRevents() { _revents &= ~POLLHUP; };

        void finish_revents() { _revents = NoEvent; };

        [[nodiscard]] int fd() const { return _fd; };

        [[nodiscard]] short events() const { return _events; };

        [[nodiscard]] bool is_nonevent() const { return _events == NoEvent; };

        [[nodiscard]] bool has_aliveEvent() const { return _revents != NoEvent; };

        [[nodiscard]] ChannelsManger *manger() const { return _manger; };

        [[nodiscard]] bool can_read() const { return _events & Readable; };

        [[nodiscard]] bool can_write() const { return _events & Writeable; };

        [[nodiscard]] Base::Time_difference last_active_time() const { return _last_active_time; };

    private:

        using DataPtr = std::weak_ptr<Data>;

        Channel(int fd, const std::shared_ptr<Data> &data, ChannelsManger &manger);

        static const short NoEvent;

        static const short Readable;

        static const short Writeable;

        const int _fd;

        short _events = 0;

        short _revents = NoEvent;

        ChannelsManger *_manger = nullptr;

        DataPtr _data;

        Base::Time_difference _last_active_time = Base::Unix_to_now();

    public:

        Channel *_prev = nullptr, *_next = nullptr;

    };

}


#endif //NET_CHANNEL_HPP