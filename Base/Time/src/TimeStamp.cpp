//
// Created by taganyer on 24-2-12.
//

#include <sys/time.h>
#include "../TimeStamp.hpp"

namespace Base {

    TimeStamp::TimeStamp(const Time &time) :
        _us_SE(mktime((tm *) &time) * coefficient + time.tm_us) {}

    TimeStamp TimeStamp::now() {
        timeval tv {};
        gettimeofday(&tv, nullptr);
        return TimeStamp(tv);
    }

    Time TimeStamp::to_Time(bool UTC) const {
        auto seconds = static_cast<time_t>(_us_SE / coefficient);
        Time time;
        if (UTC) gmtime_r(&seconds, &time);
        else localtime_r(&seconds, &time);
        time.tm_us = _us_SE % coefficient;
        return time;
    }

    std::string to_string(const TimeStamp &time, bool show_us, bool UTC) {
        Time Tm = time.to_Time(UTC);
        return to_string(Tm, show_us);
    }

    void format(char* dest, const TimeStamp &time, bool show_us, bool UTC) {
        Time Tm = time.to_Time(UTC);
        return format(dest, Tm, show_us);
    }

}
