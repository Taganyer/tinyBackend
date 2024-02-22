//
// Created by taganyer on 24-2-13.
//

#include "Timer.hpp"
#include <sys/timerfd.h>
#include <unistd.h>

#include "../Exception.hpp"

namespace Base {

    Timer::~Timer() { close_fd(); }

    void Timer::invoke(int32 timeouts) {
        if (timeouts == 0 || _time.nanoseconds <= 0) return;
        start();
        if (timeouts < 0) {
            while (true) {
                if (uint64 expirations; read(fd, &expirations, sizeof(expirations)) == -1)
                    throw Exception("Timer::invoke() running failed");
                if (_fun) _fun();
            }
        } else {
            while (timeouts) {
                if (uint64 expirations; read(fd, &expirations, sizeof(expirations)) == -1)
                    throw Exception("Timer::invoke() running failed");
                if (_fun) _fun();
                --timeouts;
            }
        }
        stop();
    }

    void Timer::sleep(int32 timeouts) {
        if (timeouts <= 0 || _time.nanoseconds <= 0) return;
        struct timespec time{};
        time.tv_sec = _time.nanoseconds / SEC_ * timeouts;
        time.tv_nsec = _time.nanoseconds % SEC_;
        time.tv_sec += timeouts * time.tv_nsec / SEC_;
        time.tv_nsec += timeouts * time.tv_nsec % SEC_;

        nanosleep(&time, nullptr);
    }

    void Timer::start() {
        struct itimerspec timeout{};
        timeout.it_value = timeout.it_interval = _time.to_timespec();

        if (fd == -1) {
            fd = timerfd_create(CLOCK_REALTIME, 0);
            if (fd == -1) throw Exception("Timer::start() fd create failed");
        }
        if (timerfd_settime(fd, 0, &timeout, nullptr) == -1)
            throw Exception("Timer::start() set failed");
    }

    void Timer::stop() {
        struct itimerspec zero = {};
        zero.it_interval.tv_sec = zero.it_value.tv_sec = 0;
        zero.it_interval.tv_nsec = zero.it_value.tv_nsec = 0;

        if (timerfd_settime(fd, 0, &zero, nullptr) == -1)
            throw Exception("Timer::stop() fail\n");
    }

    int32 Timer::release_fd() {
        start();
        int32 temp = fd;
        fd = -1;
        return temp;
    }

    void Timer::close_fd() {
        if (fd != -1) close(fd);
    }

    Time_difference Unix_to_now() {
        struct timespec now{};
        clock_gettime(CLOCK_REALTIME, &now);
        int64 ns = SEC_;
        ns *= now.tv_sec;
        ns += now.tv_nsec;
        return {ns};
    }

    void sleep(Time_difference time) {
        struct timespec timespec{};
        timespec.tv_sec = time.nanoseconds / SEC_;
        timespec.tv_nsec = time.nanoseconds % SEC_;
        nanosleep(&timespec, nullptr);
    }

}
