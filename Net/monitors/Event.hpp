//
// Created by taganyer on 3/24/24.
//

#ifndef NET_EVENT_HPP
#define NET_EVENT_HPP

#include <type_traits>

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

        int fd = 0;
        int event = 0;
        void *extra_data = nullptr;

        template<typename Type>
        static void write_to_extra_data(Event &event, const Type &value) {
            static_assert(sizeof(Type) <= sizeof(void *));
            static_assert(std::is_trivially_copyable_v<Type>);
            auto ptr = static_cast<const long long *>(static_cast<const void *>(&value));
            auto target = static_cast<long long *>(static_cast<void *>(&event.extra_data));
            *target = *ptr;
        };

        template<typename Type>
        static Type get_extra_data(Event &event) {
            static_assert(sizeof(Type) <= sizeof(void *));
            static_assert(std::is_trivially_copyable_v<Type>);
            return *static_cast<Type *>(static_cast<void *>(&event.extra_data));
        };

    };

}


#endif //NET_EVENT_HPP
