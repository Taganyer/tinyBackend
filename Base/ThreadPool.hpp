//
// Created by taganyer on 24-2-6.
//

#ifndef BASE_THREADPOOL_HPP
#define BASE_THREADPOOL_HPP

#include "Detail/Pool.hpp"
#include "Exception.hpp"
#include <functional>
#include <future>

namespace Base {
    using std::future;
    using std::promise;
    using std::function;


    class ThreadPool {
    public:

        using Fun = std::function<bool(bool)>;
        using Size = Base::Detail::PoolData::Size;

        ThreadPool(Size thread_size, Size max_task_size) : max_tasks_size(max_task_size) {
            pool->create_threads(thread_size);
        };

        ThreadPool(ThreadPool &&) = delete;

        ~ThreadPool() { delete pool; };

        template<typename Fun_, typename ...Args>
        inline auto submit(Fun_ &&fun, Args &&... args);

        template<typename Fun_, typename ...Args>
        inline auto submit_to_top(Fun_ &&fun, Args &&... args);

        void stop() { pool->stop(); };

        void start() { pool->start(); };

        void clear() { pool->clear(); };

        void shutdown() { pool->shutdown(); };

        void resize_core_thread(Size size) {
            if (size > data.core_threads) {
                pool->create_threads(size - data.core_threads);
            } else if (size < data.core_threads) {
                pool->delete_threads(data.core_threads - size);
            }
        };

        void resize_max_queue(Size size) { max_tasks_size = size; };

        [[nodiscard]]  Size get_max_queues() const { return max_tasks_size; };

        [[nodiscard]] Size get_core_threads() const { return pool->threads_size(); };

        [[nodiscard]] Size get_free_tasks() const { return pool->tasks_size(); };

        [[nodiscard]] bool stopping() const { return pool->stopping(); };

        [[nodiscard]] bool joinable() const {
            return data.state == Base::Detail::PoolData::RUNNING
                   && data.waiting_tasks < max_tasks_size;
        };


    private:

        using Data = Base::Detail::PoolData;

        struct ThreadPool_base {

            using Fun = ThreadPool::Fun;

            ThreadPool *ptr;

            struct Task {

                Task *next = nullptr;
                Fun fun;

                explicit Task(Fun &&fun) : fun(std::move(fun)) {};

            };

            explicit ThreadPool_base(ThreadPool *data) : ptr(data) {};

            ThreadPool_base create_thread() { return ThreadPool_base(ptr); };

            Task *close_thread() { return new Task([](bool) { return true; }); };

            void thread_wait(Data::Lock &l, Data::Condition &con) {
                con.wait(l, [this] {
                    return ptr->data.state != Base::Detail::PoolData::STOP
                           && ptr->pool->takeable();
                });
            };

            bool invoke_task(Task *task) {
                bool close = task->fun(false);
                delete task;
                return close;
            };

            Task *create_task(Fun &&fun) { return new Task(std::move(fun)); };

            void delete_task(Task *task) {
                task->fun(true);
                delete task;
            };

            void task_wait(Data::Lock &l, Data::Condition &con) {
                con.wait(l, [this] {
                    return ptr->joinable();
                });
            };

        };

        template<typename Result_Type>
        auto create_fun(promise<Result_Type> *p, function<Result_Type()> &&fun);

        Data data;

        Detail::Pool<ThreadPool_base> *pool = new Detail::Pool<ThreadPool_base>(ThreadPool_base(this), &data);

        Size max_tasks_size;

    };

}

namespace Base {

    template<typename Fun_, typename... Args>
    auto ThreadPool::submit_to_top(Fun_ &&fun, Args &&... args) {
        using Result_Type = std::result_of_t<Fun_(Args...)>;
        auto *p = new promise<Result_Type>();
        future<Result_Type> result = p->get_future();
        pool->submit_to_top(create_fun<Result_Type>(p, function<Result_Type()>(
                std::forward<Fun_>(fun), std::forward<Args>(args)...)));
        return result;
    }

    template<typename Fun_, typename... Args>
    auto ThreadPool::submit(Fun_ &&fun, Args &&... args) {
        using Result_Type = std::result_of_t<Fun_(Args...)>;
        auto *p = new promise<Result_Type>();
        future<Result_Type> result = p->get_future();
        pool->submit_to_tail(create_fun<Result_Type>(p, function<Result_Type()>(
                std::forward<Fun_>(fun), std::forward<Args>(args)...)));
        return result;
    }

    template<typename Result_Type>
    auto ThreadPool::create_fun(promise<Result_Type> *pro, function<Result_Type()> &&fun) {
        return [pro, pack = std::move(fun)](bool interrupt) mutable {
            if (interrupt) {
                pro->set_exception(std::make_exception_ptr(Exception("This Task have been interrupted")));
                return false;
            }
            try {
                if constexpr (std::is_same_v<Result_Type, void>) {
                    pack();
                    pro->set_value();
                } else {
                    pro->set_value(pack());
                }
            } catch (...) {
                pro->set_exception(std::current_exception());
            }
            delete pro;
            return false;
        };
    }

}

#endif //BASE_THREADPOOL_HPP
