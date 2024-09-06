//
// Created by taganyer on 24-2-6.
//

#ifndef BASE_THREADPOOL_HPP
#define BASE_THREADPOOL_HPP

#ifdef BASE_THREADPOOL_HPP

#include "Condition.hpp"
#include "Detail/AsyncFun.hpp"
#include "Container/SingleList.hpp"

namespace Base {

    class ThreadPool : NoCopy {
    public:

        ThreadPool(uint32 thread_size, uint32 max_task_size);

        ~ThreadPool();

        template<typename Fun_, typename ...Args>
        void submit_to_top(Fun_ &&fun, Args &&... args);

        template<typename Fun_, typename ...Args>
        auto submit_to_top_with_future(Fun_ &&fun, Args &&... args);

        template<typename Fun_, typename ...Args>
        void submit_to_back(Fun_ &&fun, Args &&... args);

        template<typename Fun_, typename ...Args>
        auto submit_to_back_with_future(Fun_ &&fun, Args &&... args);

        void stop();

        void start();

        void clear_task();

        void shutdown();

        void resize_core_thread(uint32 size);

        void resize_max_queue(uint32 size) { _max_tasks = size; };

        [[nodiscard]]  uint32 get_max_queues() const { return _max_tasks; };

        [[nodiscard]] uint32 get_core_threads() const { return _core_threads; };

        [[nodiscard]] uint32 get_free_tasks() const { return _list.size(); };

        [[nodiscard]] bool stopping() const { return _state.load() > RUNNING; };

        [[nodiscard]] bool joinable() const {
            return _state == RUNNING && _list.size() < _max_tasks;
        };


    private:

        using Size = uint32;
        using A_Size = std::atomic<uint32>;
        using State = std::atomic<uint32>;
        using Condition = Base::Condition;
        using Fun = std::function<bool(bool)>;
        using List = SingleList<Fun>;

        enum { RUNNING, STOP, SHUTTING, TERMINATED };

        Mutex lock;

        A_Size _core_threads = 0;

        Size _max_tasks = 0;

        State _state = TERMINATED;

        Condition _consume, _submit;

        List _list;

        void create_threads(Size size);

        void delete_threads(Size size);

        void end_thread();

        auto joinable_fun() {
            return [this] {
                return stopping() || _list.size() < _max_tasks;
            };
        };

        bool waiting(Lock<Mutex> &l);

        template<typename Fun_, typename... Args>
        inline void create_fun(List::Iter dest, Fun_ &&fun, Args &&... args);

        template<typename Fun_, typename... Args>
        inline auto create_fun_with_future(List::Iter dest, Fun_ &&fun, Args &&... args);

    };

}

namespace Base {

    template<typename Fun_, typename... Args>
    void ThreadPool::submit_to_top(Fun_ &&fun, Args &&... args) {
        Lock l(lock);
        _submit.wait(l, joinable_fun());
        if (stopping())
            throw Exception("This Task have been interrupted");
        _consume.notify_one();
        return create_fun(_list.before_begin(), std::forward<Fun_>(fun), std::forward<Args>(args)...);
    }

    template<typename Fun_, typename... Args>
    auto ThreadPool::submit_to_top_with_future(Fun_ &&fun, Args &&... args) {
        Lock l(lock);
        _submit.wait(l, joinable_fun());
        if (stopping())
            throw Exception("This Task have been interrupted");
        _consume.notify_one();
        return create_fun_with_future(_list.before_begin(), std::forward<Fun_>(fun), std::forward<Args>(args)...);
    }

    template<typename Fun_, typename... Args>
    void ThreadPool::submit_to_back(Fun_ &&fun, Args &&... args) {
        Lock l(lock);
        _submit.wait(l, joinable_fun());
        if (stopping())
            throw Exception("This Task have been interrupted");
        _consume.notify_one();
        return create_fun(_list.tail(), std::forward<Fun_>(fun), std::forward<Args>(args)...);
    }

    template<typename Fun_, typename... Args>
    auto ThreadPool::submit_to_back_with_future(Fun_ &&fun, Args &&... args) {
        Lock l(lock);
        _submit.wait(l, joinable_fun());
        if (stopping())
            throw Exception("This Task have been interrupted");
        _consume.notify_one();
        return create_fun_with_future(_list.tail(), std::forward<Fun_>(fun), std::forward<Args>(args)...);
    }

    template<typename Fun_, typename... Args>
    void ThreadPool::create_fun(ThreadPool::List::Iter dest, Fun_ &&fun, Args &&... args) {
        _list.insert_after(dest, [f =
        std::function<void()>(std::forward<Fun_>(fun), std::forward<Args>(args)...)](bool kill) {
            if (!kill && f) f();
            return false;
        });
    }

    template<typename Fun_, typename... Args>
    auto ThreadPool::create_fun_with_future(ThreadPool::List::Iter dest, Fun_ &&fun, Args &&... args) {
        using Result_Type = std::result_of_t<Fun_(Args...)>;
        auto *ptr = new Detail::AsyncFun<Result_Type, Fun_, Args...>
                (std::forward<Fun_>(fun), std::forward<Args>(args)...);
        _list.insert_after(dest, [ptr](bool kill) {
            if (kill) ptr->kill_task();
            else (*ptr)();
            delete ptr;
            return false;
        });
        return ptr->get_future();
    }

}

#endif

#endif //BASE_THREADPOOL_HPP
