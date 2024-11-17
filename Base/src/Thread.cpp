//
// Created by taganyer on 24-2-5.
//

#include "../Thread.hpp"
#include "Base/Detail/config.hpp"


namespace Base {

    Thread::Thread(Thread &&other) noexcept:
        _pthread(other._pthread), _data(other._data) {
        other._pthread = -1;
        other._data = nullptr;
    }

    Thread::~Thread() {
        if (valid() && unlikely(pthread_detach(_pthread)))
            CurrentThread::emergency_exit("Thread::~Thread detach failed.");
        delete _data;
    }

    Thread& Thread::operator=(Thread &&other) noexcept {
        if (valid() && unlikely(pthread_detach(_pthread)))
            CurrentThread::emergency_exit("Thread::operator=: pthread_detach failed.");
        delete _data;
        _pthread = other._pthread;
        _data = other._data;
        other._pthread = -1;
        other._data = nullptr;
        return *this;
    }

    void Thread::start() {
        if (valid()) return;
        if (unlikely(pthread_create(&_pthread, nullptr, &Thread::invoke, _data)))
            CurrentThread::emergency_exit("Thread::start failed.");
        _data = nullptr;
    }

    void Thread::join() {
        if (!valid()) return;
        if (unlikely(pthread_join(_pthread, nullptr)))
            CurrentThread::emergency_exit("Thread::join failed.");
        _pthread = -1;
    }

    void* Thread::invoke(void* self) {
        auto* data = static_cast<Data *>(self);
        if (!data->_name.empty())
            CurrentThread::thread_name() = std::move(data->_name);
        try {
            data->_fun();
        } catch (...) {
            CurrentThread::print_error_message("Thread::fun throw error in "
                + CurrentThread::thread_name());
            throw; // rethrow
        }
        try {
            delete data;
        } catch (...) {
            CurrentThread::print_error_message("delete Thread::fun throw error in "
               + CurrentThread::thread_name());
            throw;
        }
        return nullptr;
    }

}
