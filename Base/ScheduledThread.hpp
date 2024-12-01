//
// Created by taganyer on 24-9-6.
//

#ifndef BASE_SCHEDULEDTHREAD_HPP
#define BASE_SCHEDULEDTHREAD_HPP

#ifdef BASE_SCHEDULEDTHREAD_HPP

#include <atomic>
#include <memory>
#include "Base/Thread.hpp"
#include "Base/Condition.hpp"
#include "Base/Container/List.hpp"
#include "Base/Time/TimeDifference.hpp"

namespace Base {

    class ScheduledThread;

    class Scheduler : public std::enable_shared_from_this<Scheduler> {
    public:
        using SchedulerPtr = std::shared_ptr<Scheduler>;

        using SchedulerWeakPtr = std::weak_ptr<Scheduler>;

        virtual ~Scheduler() = default;

        virtual void invoke(void* arg) = 0;

        virtual void force_invoke() = 0;

    private:
        friend class ScheduledThread;

        std::atomic<int> waiting_tasks = 0;

        bool need_flush = true;

        bool shutdown = false;

        List<SchedulerPtr>::Iter self_location;

    };

    class ScheduledThread : NoCopy {
    public:
        explicit ScheduledThread(TimeDifference flush_time);

        ~ScheduledThread();

        using ObjectPtr = Scheduler::SchedulerPtr;

        void add_scheduler(const ObjectPtr &ptr);

        void remove_scheduler(const ObjectPtr &ptr);

        void remove_scheduler_and_invoke(const ObjectPtr &ptr, void* arg);

        void submit_task(Scheduler &scheduler, void *arg);

        void shutdown_thread();

        [[nodiscard]] bool closed() const { return shutdown; };

    private:
        std::atomic_bool shutdown = false;

        using Queue = List<ObjectPtr>;

        using QueueIter = Queue::Iter;

        struct Task {
            QueueIter iter;
            void *arg = nullptr;
        };

        Mutex _mutex;

        Condition _condition;

        TimeDifference _flush_time;

        TimeDifference _next_flush_time;

        Queue _schedulers;

        std::vector<Task> _ready;

        std::vector<Task> _invoking;

        Thread _thread;

        void get_readyQueue();

        void wait_if_no_scheduler();

        bool waiting(TimeDifference endTime);

        void get_need_flush(std::vector<QueueIter> &need_flush);

        void flush_need(std::vector<QueueIter> &need_flush);

        void set_need_flush(std::vector<QueueIter> &need_flush);

        void invoke();

        void closing();

    };
}

#endif

#endif //BASE_SCHEDULEDTHREAD_HPP
