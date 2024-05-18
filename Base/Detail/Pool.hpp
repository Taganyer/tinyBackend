//
// Created by taganyer on 24-2-6.
//

#ifndef BASE_POOL_HPP
//#define BASE_POOL_HPP

#ifdef BASE_POOL_HPP

#include <atomic>
#include "Net/Thread.hpp"
#include "Net/Condition.hpp"

namespace Base::Detail {

    struct PoolData {
        using A_Size = std::atomic<uint32>;
        using Size = uint32;
        using State = std::atomic<uint8>;
        using Mutex = Base::Mutex;
        using Condition = Base::Condition;
        using Lock = Base::Lock<Mutex>;

        enum { RUNNING = 0, RESUMING = 2, STOP = 4, SHUTTING = 8, TERMINATED = 16 };

        A_Size core_threads = 0;

        Size waiting_tasks = 0;

        State state = TERMINATED;

        Mutex lock;

        Condition consume, submit;
    };


    template<typename Creator>
    class Pool {
    public:
        using Fun = typename Creator::Fun;
        using Task = typename Creator::Task;
        using Size = PoolData::Size;
        using Lock = PoolData::Lock;

        explicit Pool(Creator &&creator, PoolData *data) :
                creator(std::move(creator)), data(data) {};

        ~Pool();

        void create_threads(Size size);

        void delete_threads(Size size);

        void submit_to_top(Fun &&fun);

        void submit_to_tail(Fun &&fun);

        void delete_tasks(Size size);

        void stop();

        void start();

        void clear();

        void shutdown();

        [[nodiscard]] Size threads_size() const { return data->core_threads.load(); };

        [[nodiscard]] Size tasks_size() const { return data->waiting_tasks; };

        [[nodiscard]] bool stopping() const { return data->state.load() > Base::Detail::PoolData::RESUMING; };

        [[nodiscard]] bool takeable() const { return head; };


    private:

        Creator creator;

        PoolData *data;

        Task *head = nullptr, *tail = nullptr;

        inline Task *get_task();

        inline void end_thread();

        inline void task_to_top(Task *task);

        inline void task_to_tail(Task *task);

    };
}

namespace Base::Detail {

    template<typename Creator>
    Pool<Creator>::~Pool() {
        shutdown();
        Lock l(data->lock);
        data->submit.wait(l, [this]() {
            return data->state.load() == Base::Detail::PoolData::TERMINATED;
        });
    }

    template<typename Creator>
    void Pool<Creator>::create_threads(Pool::Size size) {
        Size record = data->core_threads.load() + size;
        for (Size i = 0; i < size; ++i) {
            Thread thread([this] {
                Creator item = creator.create_thread();
                data->core_threads.fetch_add(1);
                Detail::this_thread_name.append("(pool)");
                Task *task;
                while (true) {
                    {
                        Lock l(data->lock);
                        data->submit.notify_one();
                        item.thread_wait(l, data->consume);
                        task = get_task();
                    }
                    if (item.invoke_task(task))
                        break;
                }
                end_thread();
            });
            thread.start();
        }
        Lock l(data->lock);
        data->submit.wait(l, [this, record] { return data->core_threads == record; });
        if (data->state.load() == Base::Detail::PoolData::TERMINATED)
            data->state.store(Base::Detail::PoolData::RUNNING);
    }

    template<typename Creator>
    void Pool<Creator>::delete_threads(Pool::Size size) {
        Lock l(data->lock);
        for (Size i = 0; i < size; ++i) {
            task_to_top(creator.close_thread());
        }
    }

    template<typename Creator>
    void Pool<Creator>::submit_to_top(Pool::Fun &&fun) {
        auto task = creator.create_task(std::move(fun));
        if (stopping()) {
            creator.delete_task(task);
            return;
        }
        Lock l(data->lock);
        creator.task_wait(l, data->submit);
        if (stopping()) {
            creator.delete_task(task);
        } else {
            task_to_top(task);
            data->consume.notify_one();
        }
    }

    template<typename Creator>
    void Pool<Creator>::submit_to_tail(Pool::Fun &&fun) {
        auto task = creator.create_task(std::move(fun));
        if (stopping()) {
            creator.delete_task(task);
            return;
        }
        Lock l(data->lock);
        creator.task_wait(l, data->submit);
        if (stopping()) {
            creator.delete_task(task);
        } else {
            task_to_tail(task);
            data->consume.notify_one();
        }
    }

    template<typename Creator>
    void Pool<Creator>::delete_tasks(Size size) {
        Lock l(data->lock);
        while (size && head) {
            auto task = get_task();
            creator.delete_task(task);
            --size;
        }
    }

    template<typename Creator>
    void Pool<Creator>::stop() {
        if (data->state.load() == Base::Detail::PoolData::RUNNING) {
            data->state.store(Base::Detail::PoolData::STOP);
            data->submit.notify_all();
        }
    }


    template<typename Creator>
    void Pool<Creator>::start() {
        if (data->state.load() == Base::Detail::PoolData::STOP) {
            Lock l(data->lock);
            data->state.store(Base::Detail::PoolData::RESUMING);
            task_to_tail(creator.create_task([this](bool) {
                data->state.store(Base::Detail::PoolData::RUNNING);
                return false;
            }));
            data->consume.notify_all();
        }
    }

    template<typename Creator>
    void Pool<Creator>::clear() {
        Lock l(data->lock);
        while (head) {
            auto task = get_task();
            creator.delete_task(task);
        }
    }

    template<typename Creator>
    void Pool<Creator>::shutdown() {
        Lock l(data->lock);
        if (data->state.load() > Base::Detail::PoolData::STOP) return;
        data->state.store(Base::Detail::PoolData::SHUTTING);
        while (head) {
            auto task = get_task();
            creator.delete_task(task);
        }
        for (Size i = 0; i < data->core_threads; ++i) {
            task_to_top(creator.close_thread());
        }
        data->consume.notify_all();
    }


    template<typename Creator>
    typename Pool<Creator>::Task *Pool<Creator>::get_task() {
        auto temp = head;
        head = head->next;
        if (!head) tail = nullptr;
        --data->waiting_tasks;
        return temp;
    }

    template<typename Creator>
    void Pool<Creator>::end_thread() {
        if (data->core_threads.fetch_sub(1) == 1) {
            data->state.store(Base::Detail::PoolData::TERMINATED);
            data->submit.notify_all();
        }
    }

    template<typename Creator>
    void Pool<Creator>::task_to_top(Pool::Task *task) {
        if (head) task->next = head;
        else tail = task;
        head = task;
        ++data->waiting_tasks;
    }

    template<typename Creator>
    void Pool<Creator>::task_to_tail(Pool::Task *task) {
        if (tail) tail->next = task;
        else head = task;
        tail = task;
        ++data->waiting_tasks;
    }

}

#endif

#endif //BASE_POOL_HPP
