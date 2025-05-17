//
// Created by taganyer on 24-5-11.
//

#ifndef BASE_TIME_DIFFERENCE_HPP
#define BASE_TIME_DIFFERENCE_HPP

#ifdef BASE_TIME_DIFFERENCE_HPP

#include <utility>
#include "tinyBackend/Base/Time/Time.hpp"

namespace Base {

    constexpr int64 NS_ = 1;
    constexpr int64 US_ = 1000;
    constexpr int64 MS_ = 1000000;
    constexpr int64 SEC_ = 1000000000;
    constexpr int64 MIN_ = 60 * SEC_;
    constexpr int64 HOUR_ = 60 * MIN_;

    struct TimeInterval {
        static constexpr char Time_difference_format[] = "%4d%02d%02d-%02d:%02d:%02d";

        static constexpr int32 Time_difference_format_len = 17;

        static constexpr char Time_difference_us_format[] = "%4d%02d%02d-%02d:%02d:%02d.%06ld";

        static constexpr int32 Time_difference_us_format_len = 24;

        int64 nanoseconds = 0;

        TimeInterval() = default;

        constexpr explicit TimeInterval(int64 ns) : nanoseconds(ns) {};

        constexpr explicit TimeInterval(timeval tv) : nanoseconds(tv.tv_sec * SEC_ + tv.tv_usec * US_) {};

        constexpr explicit TimeInterval(timespec ts) : nanoseconds(ts.tv_sec * SEC_ + ts.tv_nsec) {};

        explicit TimeInterval(const Time& time);

        constexpr operator int64() const { return nanoseconds; };

        [[nodiscard]] constexpr double to_hour() const { return (double) nanoseconds / HOUR_; };

        [[nodiscard]] constexpr double to_min() const { return (double) nanoseconds / MIN_; };

        [[nodiscard]] constexpr double to_sec() const { return (double) nanoseconds / SEC_; };

        [[nodiscard]] constexpr double to_ms() const { return (double) nanoseconds / MS_; };

        [[nodiscard]] constexpr double to_us() const { return (double) nanoseconds / US_; };

        [[nodiscard]] constexpr timeval to_timeval() const {
            return { nanoseconds / SEC_, nanoseconds % SEC_ / 1000 };
        };

        [[nodiscard]] constexpr timespec to_timespec() const {
            return { nanoseconds / SEC_, nanoseconds % SEC_ };
        };

        [[nodiscard]] Time to_Time(bool UTC = false) const;

        static TimeInterval now();

    };

    constexpr TimeInterval operator+(const TimeInterval& left, const TimeInterval& right) {
        return TimeInterval { left.nanoseconds + right.nanoseconds };
    }

    constexpr TimeInterval operator-(const TimeInterval& left, const TimeInterval& right) {
        return TimeInterval { left.nanoseconds - right.nanoseconds };
    }

    constexpr TimeInterval operator ""_ns(uint64 ns) {
        return TimeInterval { (int64) ns };
    }

    constexpr TimeInterval operator ""_us(uint64 us) {
        return TimeInterval { (int64) us * US_ };
    }

    constexpr TimeInterval operator ""_ms(uint64 ms) {
        return TimeInterval { (int64) ms * MS_ };
    }

    constexpr TimeInterval operator ""_s(uint64 sec) {
        return TimeInterval { (int64) sec * SEC_ };
    }

    constexpr TimeInterval operator ""_min(uint64 min) {
        return TimeInterval { (int64) min * MIN_ };
    }

    constexpr TimeInterval operator ""_h(uint64 hour) {
        return TimeInterval { (int64) hour * HOUR_ };
    }

    TimeInterval Unix_to_now();

    void sleep(TimeInterval time);

    template <typename Fun, typename... Args>
    TimeInterval chronograph(Fun&& fun, Args&&... args) {
        timespec startTime {}, endTime {};
        clock_gettime(CLOCK_REALTIME, &startTime);
        fun(std::forward<Args>(args)...);
        clock_gettime(CLOCK_REALTIME, &endTime);
        int64 ns = SEC_;
        ns *= endTime.tv_sec - startTime.tv_sec;
        ns += endTime.tv_nsec - startTime.tv_nsec;
        return TimeInterval { ns };
    }

    std::string to_string(TimeInterval time, bool show_us = true, bool UTC = false);

    void format(char *dest, TimeInterval time, bool show_us = true, bool UTC = false);

}

#endif

#endif //BASE_TIME_DIFFERENCE_HPP
