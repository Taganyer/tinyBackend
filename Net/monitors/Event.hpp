//
// Created by taganyer on 3/24/24.
//

#ifndef NET_EVENT_HPP
#define NET_EVENT_HPP

#ifdef NET_EVENT_HPP

namespace Net {

    struct Event {

        static const int NoEvent;

        static const int Read;

        static const int Urgent;

        static const int Write;

        static const int Error;

        static const int Invalid;

        static const int HangUp;

        void set_read() { event |= Read; };

        void set_write() { event |= Write; };

        void set_error() { event |= Error; };

        void set_HangUp() { event |= HangUp; };

        void set_NoEvent() { event = 0; };

        void unset_read() { event &= ~(Read | Urgent | HangUp); };

        void unset_write() { event &= ~Write; };

        void unset_error() { event &= ~Error; };

        void unset_HangUp() { event &= ~HangUp; };

        [[nodiscard]] bool canRead() const { return event & (Read | Urgent | HangUp); };

        [[nodiscard]] bool canWrite() const { return event & Write; };

        [[nodiscard]] bool hasError() const { return event & (Error | Invalid); };

        [[nodiscard]] bool hasHangUp() const { return event & HangUp; };

        [[nodiscard]] bool is_NoEvent() const { return event == NoEvent; };

        int fd;
        int event;

    };

}

#endif

#endif //NET_EVENT_HPP
