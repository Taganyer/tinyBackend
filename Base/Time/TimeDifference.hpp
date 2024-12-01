//
// Created by taganyer on 24-5-11.
//

#ifndef BASE_TIME_DIFFERENCE_HPP
#define BASE_TIME_DIFFERENCE_HPP

#ifdef BASE_TIME_DIFFERENCE_HPP

#include <utility>
#include "Base/Time/Time.hpp"

namespace Base {

    constexpr int64 US_ = 1000;
    constexpr int64 MS_ = 1000000;
    constexpr int64 SEC_ = 1000000000;
    constexpr int64 MIN_ = 60 * SEC_;
    constexpr int64 HOUR_ = 60 * MIN_;

    struct TimeDifference {
        static constexpr char Time_difference_format[] = "%4d%02d%02d-%02d:%02d:%02d";

        static constexpr int32 Time_difference_format_len = 17;

        static constexpr char Time_difference_us_format[] = "%4d%02d%02d-%02d:%02d:%02d.%06ld";

        static constexpr int32 Time_difference_us_format_len = 24;

        int64 nanoseconds = 0;

        TimeDifference() = default;

        constexpr explicit TimeDifference(int64 ns) : nanoseconds(ns) {};

        explicit TimeDifference(const Time &time);

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

        static TimeDifference now();

    };

    constexpr TimeDifference operator+(const TimeDifference &left, const TimeDifference &right) {
        return TimeDifference { left.nanoseconds + right.nanoseconds };
    }

    constexpr TimeDifference operator-(const TimeDifference &left, const TimeDifference &right) {
        return TimeDifference { left.nanoseconds - right.nanoseconds };
    }

    constexpr TimeDifference operator ""_ns(uint64 ns) {
        return TimeDifference { (int64) ns };
    }

    constexpr TimeDifference operator ""_us(uint64 us) {
        return TimeDifference { (int64) us * US_ };
    }

    constexpr TimeDifference operator ""_ms(uint64 ms) {
        return TimeDifference { (int64) ms * MS_ };
    }

    constexpr TimeDifference operator ""_s(uint64 sec) {
        return TimeDifference { (int64) sec * SEC_ };
    }

    constexpr TimeDifference operator ""_min(uint64 min) {
        return TimeDifference { (int64) min * MIN_ };
    }

    constexpr TimeDifference operator ""_h(uint64 hour) {
        return TimeDifference { (int64) hour * HOUR_ };
    }

    TimeDifference Unix_to_now();

    void sleep(TimeDifference time);

    template <typename Fun, typename...Args>
    TimeDifference chronograph(Fun &&fun, Args &&...args) {
        timespec startTime {}, endTime {};
        clock_gettime(CLOCK_REALTIME, &startTime);
        fun(std::forward<Args>(args)...);
        clock_gettime(CLOCK_REALTIME, &endTime);
        int64 ns = SEC_;
        ns *= endTime.tv_sec - startTime.tv_sec;
        ns += endTime.tv_nsec - startTime.tv_nsec;
        return TimeDifference { ns };
    }

    std::string to_string(TimeDifference time, bool show_us = true, bool UTC = false);

    void format(char* dest, TimeDifference time, bool show_us = true, bool UTC = false);

}

#endif

#endif //BASE_TIME_DIFFERENCE_HPP
