//
// Created by taganyer on 24-2-21.
//

#ifndef BASE_MUTEX_HPP
#define BASE_MUTEX_HPP

#include <cassert>
#include "Detail/config.hpp"
#include "Detail/NoCopy.hpp"
#include "Detail/CurrentThread.hpp"


#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail(int errnum,
                                 const char *file,
                                 unsigned int line,
                                 const char *function)
noexcept __attribute__ ((__noreturn__));
__END_DECLS

__BEGIN_DECLS
extern void __assert_fail(const char *__assertion, const char *__file,
                          unsigned int __line, const char *__function)
__THROW __attribute__ ((__noreturn__));
__END_DECLS
#endif

#define check(statement) ({ \
        __typeof(statement) flag = (statement); \
        if (__builtin_expect(flag != 0, 0)) \
            __assert_perror_fail (flag, __FILE__, __LINE__, __func__ ); \
});


namespace Base {

    class Condition;

    template<typename Mutex>
    class Lock : NoCopy {
    public:

        Lock(Mutex &lock) : _lock(lock) {
            _lock.lock();
        };

        ~Lock() {
            _lock.unlock();
        };

    private:

        Mutex &_lock;

        friend class Condition;

    };


    class Mutex : NoCopy {
    public:

        Mutex() {
            check(pthread_mutex_init(&_lock, nullptr))
        };

        Mutex(Mutex &&other) noexcept: _lock(other._lock),
                                       owner_thread(other.owner_thread) {
            other._lock = {0};
            other.owner_thread = 0;
        };

        ~Mutex() {
            if (unlikely(owner_thread != 0))
                __assert_fail("dead lock", __FILE__, __LINE__, __func__);
            check(pthread_mutex_destroy(&_lock))
        };

        void lock() {
            if (unlikely(is_owner()))
                __assert_fail("Has been locked", __FILE__, __LINE__, __func__);
            check(pthread_mutex_lock(&_lock))
            owner_thread = CurrentThread::tid();
        };

        bool try_lock() {
            int flag = pthread_mutex_trylock(&_lock);
            if (flag == 0) owner_thread = CurrentThread::tid();
            return flag == 0;
        };

        void unlock() {
            if (!is_owner()) return;
            owner_thread = 0;
            check(pthread_mutex_unlock(&_lock))
        };

        [[nodiscard]] bool is_owner() const {
            return CurrentThread::tid() == owner_thread;
        };

        [[nodiscard]] pthread_t owner() const {
            return owner_thread;
        };

    private:

        pthread_mutex_t _lock;

        pthread_t owner_thread = 0;

        friend class Lock<Mutex>;

        friend class Condition;

    };


    class SpinMutex : NoCopy {
    public:

        SpinMutex() {
            check(pthread_spin_init(&_lock, PTHREAD_PROCESS_PRIVATE))
        };

        SpinMutex(SpinMutex &&other) noexcept: _lock(other._lock),
                                               owner_thread(other.owner_thread) {
            other._lock = {0};
            other.owner_thread = 0;
        };

        ~SpinMutex() {
            if (unlikely(owner_thread != 0))
                __assert_fail("dead lock", __FILE__, __LINE__, __func__);
            check(pthread_spin_destroy(&_lock))
        };

        void lock() {
            if (unlikely(is_owner()))
                __assert_fail("Has been locked", __FILE__, __LINE__, __func__);
            check(pthread_spin_lock(&_lock))
            owner_thread = CurrentThread::tid();
        };

        bool try_lock() {
            int flag = pthread_spin_trylock(&_lock);
            if (flag == 0) owner_thread = CurrentThread::tid();
            return flag == 0;
        };

        void unlock() {
            if (!is_owner()) return;
            owner_thread = 0;
            check(pthread_spin_unlock(&_lock))
        };

        [[nodiscard]] bool is_owner() const {
            return CurrentThread::tid() == owner_thread;
        };

        [[nodiscard]] pthread_t owner() const {
            return owner_thread;
        };

    private:

        pthread_spinlock_t _lock;

        pthread_t owner_thread = 0;

        friend class Lock<SpinMutex>;

        friend class Condition;

    };

}


#endif //BASE_MUTEX_HPP
