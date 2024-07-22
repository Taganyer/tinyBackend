//
// Created by taganyer on 24-2-5.
//

#include "../Thread.hpp"
#include "Base/Exception.hpp"


namespace Base {

    Thread::Thread(Thread &&other) noexcept:
            _started(other._started), _joined(other._joined),
            name(std::move(other.name)),
            fun(std::move(other.fun)), pthread(other.pthread) {
        other._started = other._joined = false;
        other.pthread = -1;
    }

    Thread::~Thread() {
        if (started() && !joined()) {
            pthread_detach(pthread);
        }
    }

    void Thread::start() {
        if (started() || !fun) return;
        if (pthread_create(&pthread, nullptr, &Thread::invoke,
                           new Data(name, std::move(fun))))
            throw Exception("Thread::start failed\n");
        _started = true;
    }

    void Thread::join() {
        if (!started() || joined()) return;
        if (int flag = pthread_join(pthread, nullptr); flag)
            throw Exception("Thread::join failed\n" + std::to_string(flag) + ' ' + std::to_string(pthread));
        _joined = true;
    }

    void *Thread::invoke(void *self) {
        auto *data = static_cast<Data *>(self);
        if (!data->_name.empty())
            CurrentThread::thread_name() = std::move(data->_name);
        try {
            data->_fun();
            delete data;
        } catch (...) {
            delete data;
            throw; // rethrow
        }
        return nullptr;
    }

}
