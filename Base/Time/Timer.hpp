//
// Created by taganyer on 24-2-13.
//

#ifndef BASE_TIMER_HPP
#define BASE_TIMER_HPP

#include <ctime>
#include <functional>
#include "../Detail/config.hpp"

namespace Base {

    constexpr int64 US_ = 1000;
    constexpr int64 MS_ = 1000000;
    constexpr int64 SEC_ = 1000000000;
    constexpr int64 MIN_ = 60 * SEC_;
    constexpr int64 HOUR_ = 60 * MIN_;

    struct Time_difference {

        int64 nanoseconds = 0;

        Time_difference() = default;

        Time_difference(int64 ns) : nanoseconds(ns) {};

        operator int64() const { return nanoseconds; };

        [[nodiscard]] double to_hour() const { return (double) nanoseconds / HOUR_; };

        [[nodiscard]] double to_min() const { return (double) nanoseconds / MIN_; };

        [[nodiscard]] double to_sec() const { return (double) nanoseconds / SEC_; };

        [[nodiscard]] double to_ms() const { return (double) nanoseconds / MS_; };

        [[nodiscard]] double to_us() const { return (double) nanoseconds / US_; };

        [[nodiscard]] timespec to_timespec() const {
            struct timespec time{};
            time.tv_sec = nanoseconds / SEC_;
            time.tv_nsec = nanoseconds % SEC_;
            return time;
        };

    };

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

        [[nodiscard]] Time_difference get_timeout() const { return _time; };

        [[nodiscard]] int32 get_fd() const { return fd; };

    private:

        Time_difference _time;

        Fun _fun;

        int32 fd = -1;

        void start();

        void stop();

    };

    Time_difference Unix_to_now();

    void sleep(Time_difference time);

    template<typename Fun, typename ...Args>
    Time_difference chronograph(Fun &&fun, Args &&...args) {
        struct timespec startTime{}, endTime{};
        clock_gettime(CLOCK_REALTIME, &startTime);
        fun(std::forward<Args>(args)...);
        clock_gettime(CLOCK_REALTIME, &endTime);
        int64 ns = SEC_;
        ns *= (endTime.tv_sec - startTime.tv_sec);
        ns += (endTime.tv_nsec - startTime.tv_nsec);
        return {ns};
    }

    inline int64 operator ""ns(uint64 ns) {
        return (int64) ns;
    }

    inline int64 operator ""us(uint64 us) {
        return (int64) us * US_;
    }

    inline int64 operator ""ms(uint64 ms) {
        return (int64) ms * MS_;
    }

    inline int64 operator ""s(uint64 sec) {
        return (int64) sec * SEC_;
    }

    inline int64 operator ""min(uint64 min) {
        return (int64) min * MIN_;
    }

    inline int64 operator ""h(uint64 hour) {
        return (int64) hour * HOUR_;
    }

    inline Time_difference operator+(const Time_difference &left, const Time_difference &right) {
        return {left.nanoseconds + right.nanoseconds};
    }

    inline Time_difference operator-(const Time_difference &left, const Time_difference &right) {
        return {left.nanoseconds - right.nanoseconds};
    }

}


#endif //BASE_TIMER_HPP
