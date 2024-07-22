//
// Created by taganyer on 24-2-21.
//

#ifndef BASE_CONDITION_HPP
#define BASE_CONDITION_HPP

#ifdef BASE_CONDITION_HPP

#include "Mutex.hpp"
#include "Time/TimeStamp.hpp"
#include "Time/Time_difference.hpp"

namespace Base {

    class Condition : NoCopy {
    public:
        Condition() {
            check(pthread_cond_init(&_cond, nullptr))
        };

        Condition(Condition &&other) noexcept: _cond(other._cond) {
            other._cond = { 0 };
        };

        ~Condition() {
            check(pthread_cond_destroy(&_cond))
        };

        void notify_one() {
            check(pthread_cond_signal(&_cond))
        };

        void notify_all() {
            check(pthread_cond_broadcast(&_cond))
        };

        template <typename Mutex>
        void wait(Lock<Mutex> &lock) {
            lock._lock.owner_thread = 0;
            check(pthread_cond_wait(&_cond, &lock._lock._lock))
            lock._lock.owner_thread = CurrentThread::tid();
        };

        template <typename Mutex, typename Fun>
        void wait(Lock<Mutex> &lock, Fun fun) {
            while (!fun()) wait(lock);
        };

        template <typename Mutex>
        bool wait_for(Lock<Mutex> &lock, Time_difference ns) {
            auto time = (ns + Unix_to_now()).to_timespec();
            return wait_until(lock, time);
        };

        template <typename Mutex, typename Fun>
        bool wait_for(Lock<Mutex> &lock, Time_difference ns, Fun fun) {
            auto time = (ns + Unix_to_now()).to_timespec();
            while (!fun()) {
                if (!wait_until(lock, time))
                    return fun();
            }
            return true;
        }

        template <typename Mutex, typename Fun>
        bool wait_until(Lock<Mutex> &lock, const timespec &endTime, Fun fun) {
            while (!fun()) {
                if (!wait_until(lock, endTime))
                    return fun();
            }
            return true;
        }

        /// 未超时返回 true
        bool wait_until(Lock<Mutex> &lock, const timespec &endTime) {
            lock._lock.owner_thread = 0;
            int t = pthread_cond_timedwait(&_cond, &lock._lock._lock, &endTime);
            lock._lock.owner_thread = CurrentThread::tid();
            return t == 0;
        }

    private:
        pthread_cond_t _cond;

    };

}

#endif

#endif //BASE_CONDITION_HPP
