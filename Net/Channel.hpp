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

        void set_readable(bool turn_on);

        void set_writable(bool turn_on);

        void set_nonevent();

        /// 提供给 Poller 使用。
        void set_revents(short revents) { _revents = revents; };

        /// 提供给 ChannelsManager 使用。
        bool timeout(const Base::Time_difference &time);

        void remove_this();

        /// 解除 Poller 的监控，同时 channel 会在下一次 loop 循环中按照 revents 的值继续调用。
        void send_to_next_loop();

        /// TODO 以下五个函数提供给上层对象完成 invoke 中应有的事件后调用，invoke 不会主动调用。
        void set_readRevents(bool turn_on) {
            if (turn_on)_revents |= POLLIN;
            else _revents &= ~POLLIN;
        };

        void set_writeRevents(bool turn_on) {
            if (turn_on) _revents |= POLLOUT;
            else _revents &= ~POLLOUT;
        };

        void set_errorRevents(bool turn_on) {
            if (turn_on) _revents |= POLLERR;
            else _revents &= ~(POLLERR | POLLNVAL);
        };

        void set_closeRevents(bool turn_on) {
            if (turn_on) _revents |= POLLHUP;
            else _revents &= ~POLLHUP;
        };

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

        Channel *_prev = nullptr;
        Channel *_next = nullptr;

    };

}


#endif //NET_CHANNEL_HPP
