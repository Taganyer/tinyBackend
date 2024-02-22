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

        ThreadPool(Size thread_size, Size max_task_size);

        ~ThreadPool();

        ThreadPool(const ThreadPool &) = delete;

        ThreadPool &operator=(const ThreadPool &) = delete;

        template<typename Fun_, typename ...Args>
        inline auto submit(Fun_ &&fun, Args &&... args);

        template<typename Fun_, typename ...Args>
        inline auto submit_to_top(Fun_ &&fun, Args &&... args);

        inline void stop();

        inline void start();

        inline void clear();

        inline void shutdown();

        inline void resize_core_thread(Size size);

        inline void resize_max_queue(Size size);

        [[nodiscard]] inline Size get_max_queues();

        [[nodiscard]] inline Size get_core_threads();

        [[nodiscard]] inline Size get_free_tasks();

        [[nodiscard]] inline bool stopping() const;

        [[nodiscard]] inline bool joinable() const;


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

            inline ThreadPool_base create_thread();

            inline Task *close_thread();

            inline void thread_wait(Data::Lock &l, Data::Condition &con);

            inline bool invoke_task(Task *task);

            inline Task *create_task(Fun &&fun);

            inline void delete_task(Task *task);

            inline void task_wait(Data::Lock &l, Data::Condition &con);

        };

        template<typename Result_Type>
        auto create_fun(promise<Result_Type> *p, function<Result_Type()> &&fun);

        Data data;

        Detail::Pool<ThreadPool_base> *pool = new Detail::Pool<ThreadPool_base>(ThreadPool_base(this), &data);

        Size max_tasks_size;

    };


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

namespace Base {

    ThreadPool::ThreadPool(ThreadPool::Size thread_size, ThreadPool::Size max_task_size) :
            max_tasks_size(max_task_size) {
        pool->create_threads(thread_size);
    }

    ThreadPool::~ThreadPool() {
        delete pool;
    }

    void ThreadPool::stop() {
        pool->stop();
    }

    void ThreadPool::start() {
        pool->start();
    }

    void ThreadPool::clear() {
        pool->clear();
    }

    void ThreadPool::shutdown() {
        pool->shutdown();
    }

    void ThreadPool::resize_core_thread(ThreadPool::Size size) {
        if (size > data.core_threads) {
            pool->create_threads(size - data.core_threads);
        } else if (size < data.core_threads) {
            pool->delete_threads(data.core_threads - size);
        }
    }


    void ThreadPool::resize_max_queue(ThreadPool::Size size) {
        max_tasks_size = size;
    }

    ThreadPool::Size ThreadPool::get_max_queues() {
        return max_tasks_size;
    }

    ThreadPool::Size ThreadPool::get_core_threads() {
        return pool->threads_size();
    }

    ThreadPool::Size ThreadPool::get_free_tasks() {
        return pool->tasks_size();
    }

    bool ThreadPool::stopping() const {
        return pool->stopping();
    }

    bool ThreadPool::joinable() const {
        return data.state == Base::Detail::PoolData::RUNNING
               && data.waiting_tasks < max_tasks_size;
    }

    ThreadPool::ThreadPool_base ThreadPool::ThreadPool_base::create_thread() {
        return ThreadPool_base(ptr);
    }

    ThreadPool::ThreadPool_base::Task *ThreadPool::ThreadPool_base::close_thread() {
        return new Task([](bool) { return true; });
    }

    void ThreadPool::ThreadPool_base::thread_wait(Detail::PoolData::Lock &l, Detail::PoolData::Condition &con) {
        con.wait(l, [this] {
            return ptr->data.state != Base::Detail::PoolData::STOP
                   && ptr->pool->takeable();
        });
    }

    bool ThreadPool::ThreadPool_base::invoke_task(ThreadPool::ThreadPool_base::Task *task) {
        bool close = task->fun(false);
        delete task;
        return close;
    }

    ThreadPool::ThreadPool_base::Task *ThreadPool::ThreadPool_base::create_task(ThreadPool::Fun &&fun) {
        return new Task(std::move(fun));
    }

    void ThreadPool::ThreadPool_base::delete_task(ThreadPool::ThreadPool_base::Task *task) {
        task->fun(true);
        delete task;
    }

    void ThreadPool::ThreadPool_base::task_wait(Detail::PoolData::Lock &l, Detail::PoolData::Condition &con) {
        con.wait(l, [this] {
            return ptr->joinable();
        });
    }
}

#endif //BASE_THREADPOOL_HPP
