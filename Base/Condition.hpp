//
// Created by taganyer on 24-2-21.
//

#ifndef BASE_CONDITION_HPP
#define BASE_CONDITION_HPP

#include "Mutex.hpp"
#include "Time/Timer.hpp"
#include "Time/TimeStamp.hpp"

namespace Base {

    class Condition : NoCopy {
    public:
        Condition() {
            check(pthread_cond_init(&_cond, nullptr))
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

        template<typename Mutex>
        void wait(Lock<Mutex> &lock) {
            lock._lock.owner_thread = 0;
            check(pthread_cond_wait(&_cond, &lock._lock._lock))
            lock._lock.owner_thread = tid();
        };

        template<typename Mutex, typename Fun>
        void wait(Lock<Mutex> &lock, Fun fun) {
            while (!fun()) wait(lock);
        };

        template<typename Mutex>
        bool wait_for(Lock<Mutex> &lock, Time_difference ns) {
            auto time = (ns + Unix_to_now()).to_timespec();
            return wait_until(lock, time);
        };

        template<typename Mutex, typename Fun>
        bool wait_for(Lock<Mutex> &lock, Time_difference ns, Fun fun) {
            auto diff = ns + Unix_to_now();
            auto time = diff.to_timespec();
            while (!fun()) {
                if (!wait_until(lock, time))
                    return fun();
            }
            return true;
        }

        template<typename Mutex>
        bool wait_until(Lock<Mutex> &lock, TimeStamp endTime) {
            auto time = endTime.to_timespec();
            return wait_until(lock, time);
        };

        template<typename Mutex, typename Fun>
        bool wait_until(Lock<Mutex> &lock, TimeStamp endTime, Fun fun) {
            auto time = endTime.to_timespec();
            while (!fun()) {
                if (!wait_until(lock, time))
                    return fun();
            }
            return true;
        }

    private:

        pthread_cond_t _cond;

        bool wait_until(Lock<Mutex> &lock, const timespec &time) {
            lock._lock.owner_thread = 0;
            int t = pthread_cond_timedwait(&_cond, &lock._lock._lock, &time);
            if (t == 0) lock._lock.owner_thread = tid();
            return t == 0;
        }

    };

}


#endif //BASE_CONDITION_HPP