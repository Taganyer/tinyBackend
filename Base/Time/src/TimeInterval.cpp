//
// Created by taganyer on 24-5-11.
//

#include "../TimeInterval.hpp"

namespace Base {

    TimeInterval::TimeInterval(const Time &time) :
        nanoseconds(mktime((tm *) &time) * SEC_ + time.tm_us * US_) {}

    Time TimeInterval::to_Time(bool UTC) const {
        auto seconds = static_cast<time_t>(nanoseconds / SEC_);
        Time time;
        if (UTC) gmtime_r(&seconds, &time);
        else localtime_r(&seconds, &time);
        time.tm_us = nanoseconds % SEC_ / US_;
        return time;
    }

    TimeInterval TimeInterval::now() {
        return Unix_to_now();
    }

    TimeInterval Unix_to_now() {
        timespec now {};
        clock_gettime(CLOCK_REALTIME, &now);
        int64 ns = SEC_;
        ns *= now.tv_sec;
        ns += now.tv_nsec;
        return TimeInterval { ns };
    }

    void sleep(TimeInterval time) {
        if (time <= 0) return;
        timespec timespec {};
        timespec.tv_sec = time.nanoseconds / SEC_;
        timespec.tv_nsec = time.nanoseconds % SEC_;
        nanosleep(&timespec, nullptr);
    }

    std::string to_string(TimeInterval time, bool show_us, bool UTC) {
        Time Tm = time.to_Time(UTC);
        return to_string(Tm, show_us);
    }

    void format(char* dest, TimeInterval time, bool show_us, bool UTC) {
        Time Tm = time.to_Time(UTC);
        return format(dest, Tm, show_us);
    }

}
