//
// Created by taganyer on 24-2-12.
//

#ifndef BASE_TIMESTAMP_HPP
#define BASE_TIMESTAMP_HPP

#include <ctime>
#include <string>
#include <cstring>
#include "Base/Detail/config.hpp"

namespace Base {

    struct Time : public tm {
        int64 tm_us = -1;

        Time() = default;
    };


    class TimeStamp {
    public:

        static const char *Time_format;

        static const int32 Time_format_len;

        static const char *Time_us_format;

        static const int32 Time_us_format_len;

        static constexpr int64 coefficient = 1000 * 1000;

        static TimeStamp now();

        static Time get_time(const TimeStamp &ts, bool UTC = false);

        TimeStamp() = default;

        explicit TimeStamp(int64 us_SE) : _us_SE(us_SE) {};

        explicit TimeStamp(const Time &time);

        explicit TimeStamp(const timeval &tv) : _us_SE(expand_tv(tv)) {};

        std::string to_string(bool UTC = false, bool show_us = true);

        void format(char *dest, bool UTC = false, bool show_us = true);

        [[nodiscard]] Time get_time(bool UTC = false) const;

        [[nodiscard]] int64 us_since_epoch() const { return _us_SE; };

        [[nodiscard]] timespec to_timespec() const {
            return {_us_SE / coefficient, _us_SE % coefficient};
        };

    private:

        int64 _us_SE = 0;

        static int64 expand_tv(const timeval &tv) {
            return tv.tv_sec * coefficient + tv.tv_usec;
        }

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

    std::string to_string(const Time &time, bool show_us = true);

    void format(char *dest, const Time &time, bool show_us = true);

    Time get_time_now(bool UTC = false);

}


#endif //BASE_TIMESTAMP_HPP
