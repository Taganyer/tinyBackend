//
// Created by taganyer on 24-2-29.
//

#ifndef NET_CHANNEL_HPP
#define NET_CHANNEL_HPP

#include <memory>
#include "../Base/Detail/config.hpp"
#include "../Base/Detail/NoCopy.hpp"
#include "../Base/Time/Timer.hpp"

namespace Net {

    class Poller;

    class Channel : private Base::NoCopy {
    public:

        using EventCallback = std::function<void()>;

        using ChannelPtr = std::unique_ptr<Channel>;

        /// 隐藏了构造函数，保证生成的对象都是唯一对象。
        static ChannelPtr create_Channel(int fd);

        ~Channel();

        void invoke();

        void enable_read();

        void enable_write();

        void disable_read();

        void disable_write();

        void set_nonevent();

        void set_readCallback(EventCallback event) { readCallback = std::move(event); };

        void set_writeCallback(EventCallback event) { writeCallback = std::move(event); };

        void set_errorCallback(EventCallback event) { errorCallback = std::move(event); };

        void set_closeCallback(EventCallback event) { closeCallback = std::move(event); };

        /// 提供给 Poller 使用。
        void active() { _last_active_time = Base::Unix_to_now(); };

        void set_poller(Poller *poller) { _poller = poller; }

        /// 提供给 Poller 使用。
        void set_revents(short revents) { _revents = revents; };

        [[nodiscard]] int fd() const { return _fd; };

        [[nodiscard]] short events() const { return _events; };

        [[nodiscard]] bool is_nonevent() const { return _events == NoEvent; };

        [[nodiscard]] bool has_event() const { return _revents != NoEvent; };

        [[nodiscard]] bool can_read() const { return _events & Readable; };

        [[nodiscard]] bool can_write() const { return _events & Writeable; };

        [[nodiscard]] Base::Time_difference last_active_time() const { return _last_active_time; };

    private:

        Channel(int fd);

        static const short NoEvent;

        static const short Readable;

        static const short Writeable;

        const int _fd;

        Poller *_poller = nullptr;

        short _events = 0;

        short _revents = NoEvent;

        EventCallback readCallback;

        EventCallback writeCallback;

        EventCallback errorCallback;

        EventCallback closeCallback;

        Base::Time_difference _last_active_time = Base::Unix_to_now();

    };

}


#endif //NET_CHANNEL_HPP
