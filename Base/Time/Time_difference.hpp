//
// Created by taganyer on 24-5-11.
//

#ifndef BASE_TIME_DIFFERENCE_HPP
#define BASE_TIME_DIFFERENCE_HPP

#ifdef BASE_TIME_DIFFERENCE_HPP

#include <ctime>
#include <utility>
#include "../Detail/config.hpp"

namespace Base {

    constexpr int64 US_ = 1000;
    constexpr int64 MS_ = 1000000;
    constexpr int64 SEC_ = 1000000000;
    constexpr int64 MIN_ = 60 * SEC_;
    constexpr int64 HOUR_ = 60 * MIN_;

    struct Time_difference {

        int64 nanoseconds = 0;

        Time_difference() = default;

        constexpr Time_difference(int64 ns) : nanoseconds(ns) {};

        operator int64() const { return nanoseconds; };

        [[nodiscard]] double to_hour() const { return (double) nanoseconds / HOUR_; };

        [[nodiscard]] double to_min() const { return (double) nanoseconds / MIN_; };

        [[nodiscard]] double to_sec() const { return (double) nanoseconds / SEC_; };

        [[nodiscard]] double to_ms() const { return (double) nanoseconds / MS_; };

        [[nodiscard]] double to_us() const { return (double) nanoseconds / US_; };

        [[nodiscard]] timespec to_timespec() const {
            timespec time {};
            time.tv_sec = nanoseconds / SEC_;
            time.tv_nsec = nanoseconds % SEC_;
            return time;
        };

    };

    inline Time_difference operator+(const Time_difference &left, const Time_difference &right) {
        return { left.nanoseconds + right.nanoseconds };
    }

    inline Time_difference operator-(const Time_difference &left, const Time_difference &right) {
        return { left.nanoseconds - right.nanoseconds };
    }

    inline int64 operator ""_ns(uint64 ns) {
        return (int64) ns;
    }

    inline int64 operator ""_us(uint64 us) {
        return (int64) us * US_;
    }

    inline int64 operator ""_ms(uint64 ms) {
        return (int64) ms * MS_;
    }

    inline int64 operator ""_s(uint64 sec) {
        return (int64) sec * SEC_;
    }

    inline int64 operator ""_min(uint64 min) {
        return (int64) min * MIN_;
    }

    inline int64 operator ""_h(uint64 hour) {
        return (int64) hour * HOUR_;
    }

    Time_difference Unix_to_now();

    void sleep(Time_difference time);

    template <typename Fun, typename...Args>
    Time_difference chronograph(Fun &&fun, Args &&...args) {
        struct timespec startTime {}, endTime {};
        clock_gettime(CLOCK_REALTIME, &startTime);
        fun(std::forward<Args>(args)...);
        clock_gettime(CLOCK_REALTIME, &endTime);
        int64 ns = SEC_;
        ns *= (endTime.tv_sec - startTime.tv_sec);
        ns += (endTime.tv_nsec - startTime.tv_nsec);
        return { ns };
    }

}

#endif

#endif //BASE_TIME_DIFFERENCE_HPP
