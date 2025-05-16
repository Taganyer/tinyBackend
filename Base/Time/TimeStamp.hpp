//
// Created by taganyer on 24-2-12.
//

#ifndef BASE_TIMESTAMP_HPP
#define BASE_TIMESTAMP_HPP

#include "Base/Time/Time.hpp"

namespace Base {

    class TimeStamp {
    public:
        static constexpr char Time_format[] = "%4d%02d%02d-%02d:%02d:%02d";

        static constexpr int32 Time_format_len = 17;

        static constexpr char Time_us_format[] = "%4d%02d%02d-%02d:%02d:%02d.%06ld";

        static constexpr int32 Time_us_format_len = 24;

        static constexpr int64 coefficient = 1000 * 1000;

        TimeStamp() = default;

        explicit constexpr TimeStamp(int64 us_SE) : _us_SE(us_SE) {};

        explicit TimeStamp(const timeval &tv) : _us_SE(expand_tv(tv)) {};

        explicit TimeStamp(const Time &time);

        [[nodiscard]] constexpr int64 us_since_epoch() const { return _us_SE; };

        [[nodiscard]] constexpr timeval to_timeval() const {
            return { _us_SE / coefficient, _us_SE % coefficient / 1000 };
        };

        [[nodiscard]] constexpr timespec to_timespec() const {
            return { _us_SE / coefficient, _us_SE % coefficient };
        };

        [[nodiscard]] Time to_Time(bool UTC = false) const;

    private:
        int64 _us_SE = 0;

        static int64 expand_tv(const timeval &tv) {
            return tv.tv_sec * coefficient + tv.tv_usec;
        }

    public:
        static TimeStamp now();

    };

#define TS_operator(symbol) \
    inline bool operator symbol(const TimeStamp &left, const TimeStamp &right) { \
        return left.us_since_epoch() symbol right.us_since_epoch(); \
    }

    TS_operator(<)

    TS_operator(<=)

    TS_operator(>)

    TS_operator(>=)

    TS_operator(==)

    TS_operator(!=)

#undef TS_operator

    inline double operator-(const TimeStamp &left, const TimeStamp &right) {
        return (double) (left.us_since_epoch() - right.us_since_epoch()) / TimeStamp::coefficient;
    }

    inline TimeStamp operator+(const TimeStamp &time, double seconds) {
        auto us = (int64) (seconds * TimeStamp::coefficient);
        return TimeStamp(time.us_since_epoch() + us);
    }

    inline TimeStamp operator-(const TimeStamp &time, double seconds) {
        auto us = (int64) (seconds * TimeStamp::coefficient);
        return TimeStamp(time.us_since_epoch() - us);
    }

    std::string to_string(const TimeStamp &time, bool show_us = true, bool UTC = false);

    void format(char* dest, const TimeStamp &time, bool show_us = true, bool UTC = false);

}


#endif //BASE_TIMESTAMP_HPP
