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

#define CAPI_CHECK(statement) ({ \
        __typeof(statement) flag = (statement); \
        if (__builtin_expect(flag != 0, 0)) \
            __assert_perror_fail (flag, __FILE__, __LINE__, __func__ ); \
});


namespace Base {

    class Condition;

    template <typename Mutex>
    class Lock : NoCopy {
    public:
        explicit Lock(Mutex& lock) : _lock(lock) {
            _lock.lock();
        };

        ~Lock() {
            _lock.unlock();
        };

    private:
        Mutex& _lock;

        friend class Condition;

    };


    class Mutex : NoCopy {
    public:
        Mutex() {
            CAPI_CHECK(pthread_mutex_init(&_lock, nullptr))
        };

        Mutex(Mutex&& other) noexcept: _lock(other._lock),
            _owner_thread(other._owner_thread) {
            other._lock = { 0 };
            other._owner_thread = 0;
        };

        ~Mutex() {
            if (unlikely(_owner_thread != 0))
                __assert_fail("dead lock", __FILE__, __LINE__, __func__);
            CAPI_CHECK(pthread_mutex_destroy(&_lock))
        };

        void lock() {
            if (unlikely(is_owner()))
                __assert_fail("Has been locked", __FILE__, __LINE__, __func__);
            CAPI_CHECK(pthread_mutex_lock(&_lock))
            _owner_thread = CurrentThread::tid();
        };

        bool try_lock() {
            int flag = pthread_mutex_trylock(&_lock);
            if (flag == 0) _owner_thread = CurrentThread::tid();
            return flag == 0;
        };

        void unlock() {
            if (!is_owner()) return;
            _owner_thread = 0;
            CAPI_CHECK(pthread_mutex_unlock(&_lock))
        };

        [[nodiscard]] bool is_owner() const {
            return CurrentThread::tid() == _owner_thread;
        };

        [[nodiscard]] pthread_t owner() const {
            return _owner_thread;
        };

    private:
        pthread_mutex_t _lock {};

        pthread_t _owner_thread = 0;

        friend class Condition;

    };


    class SpinMutex : NoCopy {
    public:
        SpinMutex() {
            CAPI_CHECK(pthread_spin_init(&_lock, PTHREAD_PROCESS_PRIVATE))
        };

        SpinMutex(SpinMutex&& other) noexcept: _lock(other._lock),
            _owner_thread(other._owner_thread) {
            other._lock = { 0 };
            other._owner_thread = 0;
        };

        ~SpinMutex() {
            if (unlikely(_owner_thread != 0))
                __assert_fail("dead lock", __FILE__, __LINE__, __func__);
            CAPI_CHECK(pthread_spin_destroy(&_lock))
        };

        void lock() {
            if (unlikely(is_owner()))
                __assert_fail("Has been locked", __FILE__, __LINE__, __func__);
            CAPI_CHECK(pthread_spin_lock(&_lock))
            _owner_thread = CurrentThread::tid();
        };

        bool try_lock() {
            int flag = pthread_spin_trylock(&_lock);
            if (flag == 0) _owner_thread = CurrentThread::tid();
            return flag == 0;
        };

        void unlock() {
            if (!is_owner()) return;
            _owner_thread = 0;
            CAPI_CHECK(pthread_spin_unlock(&_lock))
        };

        [[nodiscard]] bool is_owner() const {
            return CurrentThread::tid() == _owner_thread;
        };

        [[nodiscard]] pthread_t owner() const {
            return _owner_thread;
        };

    private:
        pthread_spinlock_t _lock {};

        pthread_t _owner_thread = 0;

        friend class Condition;

    };

    template <typename Mutex>
    class ReentrantMutex : Mutex {
    public:
        ReentrantMutex() = default;

        ReentrantMutex(ReentrantMutex&& other) noexcept:
            Mutex(std::move(other)), _counter(other._counter) {
            other._counter = 0;
        };

        ~ReentrantMutex() = default;

        void lock() {
            if (!Mutex::is_owner()) Mutex::lock();
            ++_counter;
        };

        bool try_lock() {
            if (Mutex::is_owner()) {
                ++_counter;
                return true;
            }
            if (Mutex::try_lock()) {
                ++_counter;
                return true;
            }
            return false;
        };

        void unlock() {
            if (!Mutex::is_owner()) return;
            if (--_counter == 0) Mutex::unlock();
        };


        [[nodiscard]] bool is_owner() const {
            return Mutex::is_owner();
        };

        [[nodiscard]] pthread_t owner() const {
            return Mutex::owner();
        };

    private:
        uint32 _counter = 0;

        friend class Condition;

    };

}


#endif //BASE_MUTEX_HPP
