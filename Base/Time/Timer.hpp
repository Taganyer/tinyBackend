//
// Created by taganyer on 24-2-13.
//

#ifndef BASE_TIMER_HPP
#define BASE_TIMER_HPP

#include <functional>
#include "TimeDifference.hpp"

namespace Base {

    class Timer {
    public:

        using Fun = std::function<void()>;

        Timer() = default;

        Timer(const Timer &other) : _time(other._time), _fun(other._fun) {};

        Timer(int64 ns) : _time(ns) {}

        Timer(int64 ns, Timer::Fun fun) : _time(ns), _fun(std::move(fun)) {};

        ~Timer();

        void invoke(int32 timeouts);

        void sleep(int32 timeouts = 1);

        int32 release_fd();

        void reset_time(int64 ns) { _time.nanoseconds = ns; };

        void reset_fun(Fun fun) { _fun = std::move(fun); };

        void close_fd();

        [[nodiscard]] TimeDifference get_timeout() const { return _time; };

        [[nodiscard]] int32 get_fd() const { return fd; };

    private:

        TimeDifference _time;

        Fun _fun;

        int32 fd = -1;

        void start();

        void stop();

    };

}


#endif //BASE_TIMER_HPP
