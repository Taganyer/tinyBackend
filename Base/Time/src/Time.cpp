//
// Created by taganyer on 24-11-17.
//

#include "../Time.hpp"
#include <sys/time.h>

using namespace Base;

Time Time::now(bool UTC) {
    timeval tv {};
    gettimeofday(&tv, nullptr);
    Time time;
    if (UTC) gmtime_r(&tv.tv_sec, &time);
    else localtime_r(&tv.tv_sec, &time);
    time.tm_us = tv.tv_usec;
    return time;
}

std::string Base::to_string(const Time& time, bool show_us) {
    if (show_us) {
        std::string temp(Time::Time_us_format_len, '\0');
        format(temp.data(), time, show_us);
        return temp;
    } else {
        std::string temp(Time::Time_format_len, '\0');
        format(temp.data(), time, show_us);
        return temp;
    }
}

void Base::format(char *dest, const Time& time, bool show_us) {
    if (show_us) {
        std::sprintf(dest, Time::Time_us_format,
                     time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
                     time.tm_hour, time.tm_min, time.tm_sec, time.tm_us);
    } else {
        std::sprintf(dest, Time::Time_format,
                     time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
                     time.tm_hour, time.tm_min, time.tm_sec);
    }
}
