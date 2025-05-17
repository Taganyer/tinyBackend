//
// Created by taganyer on 24-9-25.
//

#ifndef BASE_THREADPOOL_HPP
#define BASE_THREADPOOL_HPP

#ifdef BASE_THREADPOOL_HPP

#include <vector>
#include "Condition.hpp"
#include "Detail/AsyncFun.hpp"

namespace Base {

    class ThreadPool {
    public:
        ThreadPool(uint32 threads_size, uint32 max_tasks_size);

        ~ThreadPool();

        template <typename Fun_, typename... Args>
        void submit(Fun_&& fun, Args&&... args);

        template <typename Fun_, typename... Args>
        auto submit_with_future(Fun_&& fun, Args&&... args);

        bool stolen_a_task();

        void stop();

        void start();

        void clear_task();

        void shutdown();

        void resize_core_threads(uint32 size);

        void resize_max_queue(uint32 size);

        [[nodiscard]] uint32 get_max_queues() const { return _max_tasks; };

        [[nodiscard]] uint32 get_core_threads() const { return _core_threads; };

        [[nodiscard]] uint32 get_free_tasks() const { return _list.size(); };

        [[nodiscard]] bool stopping() const { return _state.load(std::memory_order_consume) > RUNNING; };

        [[nodiscard]] bool joinable() const {
            return _state.load(std::memory_order_consume) == RUNNING && _list.size() < _max_tasks;
        };

        [[nodiscard]] bool current_thread_in_this_pool() const;

    private:
        using Size = uint32;
        using A_Size = std::atomic<uint32>;
        using State = std::atomic<uint32>;
        using Condition = Base::Condition;
        using Fun = std::function<void(bool)>;
        using List = std::vector<Fun>;

        enum { RUNNING, STOP, SHUTTING, TERMINATED };

        Mutex _lock;

        A_Size _core_threads = 0, _target_thread_size = 0;

        Size _max_tasks = 0;

        State _state = TERMINATED;

        Condition _consume, _submit;

        List _list;

        [[nodiscard]] bool need_delete() const;

        void create_thread();

        void thread_begin(std::atomic<bool>& create_done);

        void thread_end();

        auto joinable_fun() {
            return [this] {
                return stopping() || _list.size() < _max_tasks;
            };
        };

        template <typename Fun_, typename... Args>
        void create_fun(Fun_&& fun, Args&&... args);

        template <typename Fun_, typename... Args>
        auto create_fun_with_future(Fun_&& fun, Args&&... args);

    };

}

namespace Base {

    template <typename Fun_, typename... Args>
    void ThreadPool::submit(Fun_&& fun, Args&&... args) {
        Lock l(_lock);
        _submit.wait(l, joinable_fun());
        if (stopping())
            throw Exception("This Task have been interrupted");
        _consume.notify_one();
        create_fun(std::forward<Fun_>(fun), std::forward<Args>(args)...);
    }

    template <typename Fun_, typename... Args>
    auto ThreadPool::submit_with_future(Fun_&& fun, Args&&... args) {
        Lock l(_lock);
        _submit.wait(l, joinable_fun());
        if (stopping())
            throw Exception("This Task have been interrupted");
        _consume.notify_one();
        return create_fun_with_future(std::forward<Fun_>(fun), std::forward<Args>(args)...);
    }

    template <typename Fun_, typename... Args>
    void ThreadPool::create_fun(Fun_&& fun, Args&&... args) {
        auto fun_ptr = new Detail::FunPack<Fun_, Args...>(std::forward<Fun_>(fun),
                                                          std::forward<Args>(args)...);
        _list.emplace_back([fun_ptr] (bool kill) {
                if (!kill) (*fun_ptr)();
                delete fun_ptr;
            }
        );
    }

    template <typename Fun_, typename... Args>
    auto ThreadPool::create_fun_with_future(Fun_&& fun, Args&&... args) {
        using Result_Type = std::result_of_t<Fun_(Args...)>;
        auto fun_ptr = new Detail::AsyncFun<Result_Type, Fun_, Args...>(std::forward<Fun_>(fun),
                                                                        std::forward<Args>(args)...);
        _list.emplace_back([fun_ptr] (bool kill) {
            if (kill) fun_ptr->kill_task();
            else (*fun_ptr)();
            delete fun_ptr;
        });
        return fun_ptr->get_future();
    }

}

#endif

#endif //BASE_THREADPOOL_HPP
