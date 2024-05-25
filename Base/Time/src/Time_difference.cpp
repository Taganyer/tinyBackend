//
// Created by taganyer on 24-5-11.
//

#include "../Time_difference.hpp"

namespace Base {

    Time_difference Unix_to_now() {
        struct timespec now {};
        clock_gettime(CLOCK_REALTIME, &now);
        int64 ns = SEC_;
        ns *= now.tv_sec;
        ns += now.tv_nsec;
        return { ns };
    }

    void sleep(Time_difference time) {
        if (time <= 0) return;
        struct timespec timespec {};
        timespec.tv_sec = time.nanoseconds / SEC_;
        timespec.tv_nsec = time.nanoseconds % SEC_;
        nanosleep(&timespec, nullptr);
    }

}
