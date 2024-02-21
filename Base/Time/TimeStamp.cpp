//
// Created by taganyer on 24-2-12.
//

#include <sys/time.h>
#include "TimeStamp.hpp"

namespace Base {

    const char *TimeStamp::Time_format = "%4d%02d%02d-%02d:%02d:%02d";

    const int32 TimeStamp::Time_format_len = 17;

    const char *TimeStamp::Time_us_format = "%4d%02d%02d-%02d:%02d:%02d.%06ld";

    const int32 TimeStamp::Time_us_format_len = 24;

    std::string to_string(const Time &time, bool show_us) {
        if (show_us) {
            std::string temp(TimeStamp::Time_us_format_len, '\0');
            format(temp.data(), time, show_us);
            return temp;
        } else {
            std::string temp(TimeStamp::Time_format_len, '\0');
            format(temp.data(), time, show_us);
            return temp;
        }
    }

    void format(char *dest, const Time &time, bool show_us) {
        if (show_us) {
            std::sprintf(dest, TimeStamp::Time_us_format,
                         time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
                         time.tm_hour, time.tm_min, time.tm_sec, time.tm_us);
        } else {
            std::sprintf(dest, TimeStamp::Time_format,
                         time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
                         time.tm_hour, time.tm_min, time.tm_sec);
        }
    }

    TimeStamp TimeStamp::now() {
        struct timeval tv{};
        gettimeofday(&tv, nullptr);
        return TimeStamp(tv);
    }

    Time get_time_now(bool UTC) {
        struct timeval tv{};
        gettimeofday(&tv, nullptr);
        struct Time time;
        if (UTC) gmtime_r(&tv.tv_sec, &time);
        else localtime_r(&tv.tv_sec, &time);
        time.tm_us = tv.tv_usec;
        return time;
    }

    Time TimeStamp::get_time(const TimeStamp &ts, bool UTC) {
        auto seconds = static_cast<time_t>(ts._us_SE / coefficient);
        struct Time time;
        if (UTC) gmtime_r(&seconds, &time);
        else localtime_r(&seconds, &time);
        time.tm_us = ts._us_SE % coefficient;
        return time;
    }

    TimeStamp::TimeStamp(const Time &time) :
            _us_SE(mktime((tm *) &time) * coefficient + time.tm_us) {}

    std::string TimeStamp::to_string(bool UTC, bool show_us) {
        return Base::to_string(get_time(UTC), show_us);
    }

    void TimeStamp::format(char *dest, bool UTC, bool show_us) {
        Base::format(dest, get_time(UTC), show_us);
    }

    Time TimeStamp::get_time(bool UTC) const {
        return get_time(*this, UTC);
    }
}