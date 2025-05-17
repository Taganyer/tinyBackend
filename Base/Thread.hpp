//
// Created by taganyer on 24-2-5.
//

#ifndef BASE_THREAD_HPP
#define BASE_THREAD_HPP

#include <functional>
#include "Detail/NoCopy.hpp"
#include "Detail/CurrentThread.hpp"


namespace Base {

    class Thread : NoCopy {
    public:
        using Thread_fun = std::function<void()>;

        using Id = pthread_t;

        Thread() = default;

        Thread(Thread&& other) noexcept;

        template <typename Fun, typename... Args>
        Thread(string name, Fun&& fun, Args&&... args) :
            _data(new Data(std::move(name),
                           Thread_fun(std::forward<Fun>(fun), std::forward<Args>(args)...))) {};

        ~Thread();

        template <typename Fun, typename... Args>
        explicit Thread(Fun fun, Args&&... args) :
            _data(new Data(string(), Thread_fun(std::forward<Fun>(fun), std::forward<Args>(args)...))) {};

        Thread& operator=(Thread&& other) noexcept;

        void start();

        void join();

        [[nodiscard]] Id get_id() const { return _pthread; };

        [[nodiscard]] bool valid() const { return _pthread != -1; };

    private:
        struct Data {

            Data(string name, Thread_fun fun) : _name(std::move(name)), _fun(std::move(fun)) {};

            string _name;

            Thread_fun _fun;

        };

        pthread_t _pthread = -1;

        Data *_data = nullptr;

        static void* invoke(void *self);

    };

    inline bool operator==(const Thread& left, const Thread& right) {
        return pthread_equal(left.get_id(), right.get_id());
    }

    inline bool operator!=(const Thread& left, const Thread& right) {
        return !pthread_equal(left.get_id(), right.get_id());
    }

}

#endif //BASE_THREAD_HPP
