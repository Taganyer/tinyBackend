//
// Created by taganyer on 24-2-5.
//

#ifndef BASE_THREAD_HPP
#define BASE_THREAD_HPP

#ifdef BASE_THREAD_HPP

#include <functional>
#include "Detail/NoCopy.hpp"
#include "Detail/CurrentThread.hpp"

namespace Base {

    class Thread : NoCopy {
    public:
        using Thread_fun = std::function<void()>;

        using Id = pthread_t;

        Thread(Thread &&other) noexcept;

        template <typename Fun, typename...Args>
        Thread(string name, Fun fun, Args &&...args) :
            name(std::move(name)), fun(fun, args...) {};

        template <typename Fun, typename...Args>
        Thread(Fun fun, Args &&...args) : fun(fun, args...) {};

        ~Thread();

        void start();

        void join();

        [[nodiscard]] bool started() const { return _started; };

        [[nodiscard]] bool joined() const { return _joined; };

        [[nodiscard]] Id get_id() const { return pthread; };

        [[nodiscard]] const string& get_name() const { return name; };

    private:
        bool _started = false, _joined = false;

        string name;

        Thread_fun fun;

        pthread_t pthread = -1;

        struct Data {

            string _name;

            Thread_fun _fun;

            Data(string name, Thread_fun fun) : _name(std::move(name)), _fun(std::move(fun)) {};

        };

        static void* invoke(void* self);

    };

    inline bool operator==(const Thread &left, const Thread &right) {
        return pthread_equal(left.get_id(), right.get_id());
    }

    inline bool operator!=(const Thread &left, const Thread &right) {
        return !pthread_equal(left.get_id(), right.get_id());
    }

}

#endif

#endif //BASE_THREAD_HPP
