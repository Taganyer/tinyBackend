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
#include "functions/errors.hpp"

namespace Net {

    class Poller;

    class ChannelsManger;

    class NetLink;

    class Channel : private Base::NoCopy {
    public:

        static const short NoEvent;

        static const short Read;

        static const short Urgent;

        static const short Write;

        static const short Error;

        static const short Invalid;

        static const short Close;

        using Data = NetLink;

        using ChannelPtr = Channel *;

        using DataPtr = std::weak_ptr<Data>;

        using SharedData = std::shared_ptr<Data>;

        /// 隐藏了构造函数，保证生成的对象都是 heap 对象。
        static ChannelPtr create_Channel(const SharedData &data, ChannelsManger &manger);

        static void destroy_Channel(Channel *channel);

        ~Channel();

        void invoke();

        void set_readable(bool turn_on);

        void set_writable(bool turn_on);

        void set_nonevent();

        /// 提供给 Poller 和用户使用。
        void set_revents(int revents) { _revents = (short) revents; };

        /// 提供给 ChannelsManager 使用。
        bool timeout(const Base::Time_difference &time);

        void remove_this();

        /// channel 会在下一次 loop 循环中按照 revents 的值继续调用。
        void send_to_next_loop();

        [[nodiscard]] int fd() const { return _fd; };

        [[nodiscard]] short events() const { return _events; };

        [[nodiscard]] short revents() const { return _revents; };

        [[nodiscard]] bool is_nonevent() const { return _events == NoEvent; };

        [[nodiscard]] bool has_aliveEvent() const { return _revents != NoEvent; };

        [[nodiscard]] ChannelsManger *manger() const { return _manger; };

        [[nodiscard]] Base::Time_difference last_active_time() const { return _last_active_time; };

    private:

        Channel(const SharedData &data, ChannelsManger &manger);

        const int _fd;

        short _events = 0;

        short _revents = NoEvent;

        ChannelsManger *_manger = nullptr;

        DataPtr _data;

        Base::Time_difference _last_active_time = Base::Unix_to_now();

    public:

        Channel *_prev = nullptr;
        Channel *_next = nullptr;

        error_mark errorMark{error_types::Link_ErrorEvent, -1};

    };

}


#endif //NET_CHANNEL_HPP
