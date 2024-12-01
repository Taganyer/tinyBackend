//
// Created by taganyer on 24-9-6.
//

#include "../ScheduledThread.hpp"

using namespace Base;

ScheduledThread::ScheduledThread(TimeDifference flush_time) :
    _flush_time(flush_time) {
    _thread = Thread(
        string("ScheduledThread") + std::to_string(CurrentThread::tid()),
        [this] {
            std::vector<QueueIter> need_flush;
            while (!shutdown.load(std::memory_order_acquire)) {
                wait_if_no_scheduler();
                if (shutdown) break;
                _next_flush_time = Unix_to_now() + _flush_time;
                while (waiting(_next_flush_time)) invoke();
                invoke();
                get_need_flush(need_flush);
                flush_need(need_flush);
                set_need_flush(need_flush);
            }
            closing();
        });
    _thread.start();
}

ScheduledThread::~ScheduledThread() {
    shutdown_thread();
}

void ScheduledThread::add_scheduler(const ObjectPtr &ptr) {
    Lock l(_mutex);
    if (shutdown.load(std::memory_order_acquire)) return;
    _schedulers.insert(_schedulers.end(), ptr);
    ptr->self_location = _schedulers.tail();
    _condition.notify_one();
}

void ScheduledThread::remove_scheduler(const ObjectPtr &ptr) {
    Lock l(_mutex);
    if (shutdown.load(std::memory_order_acquire)) return;
    ptr->shutdown = true;
    ptr->need_flush = false;
}

void ScheduledThread::remove_scheduler_and_invoke(const ObjectPtr &ptr, void* arg) {
    Lock l(_mutex);
    if (shutdown.load(std::memory_order_acquire)) return;
    _ready.push_back(Task { ptr->self_location, arg });
    ptr->shutdown = true;
    ptr->need_flush = false;
    ptr->waiting_tasks.fetch_add(1, std::memory_order_release);
    _condition.notify_one();
}

void ScheduledThread::submit_task(Scheduler &scheduler, void* arg) {
    Lock l(_mutex);
    _ready.push_back(Task { scheduler.self_location, arg });
    scheduler.need_flush = false;
    scheduler.waiting_tasks.fetch_add(1, std::memory_order_release);
    _condition.notify_one();
}

void ScheduledThread::shutdown_thread() {
    shutdown.store(true, std::memory_order_release);
    _condition.notify_one();
    _thread.join();
    Lock l(_mutex);
    _schedulers.erase(_schedulers.begin(), _schedulers.end());
    _ready = {};
    _invoking = {};
}

void ScheduledThread::get_readyQueue() {
    Lock l(_mutex);
    _invoking.swap(_ready);
}

void ScheduledThread::wait_if_no_scheduler() {
    Lock l(_mutex);
    _condition.wait(l, [this] {
        return _schedulers.size() != 0 || shutdown.load(std::memory_order_consume);
    });
}

bool ScheduledThread::waiting(TimeDifference endTime) {
    Lock l(_mutex);
    bool waiting_again = _condition.wait_until(l, endTime.to_timespec());
    _invoking.swap(_ready);
    return waiting_again && !shutdown.load(std::memory_order_consume);
}

void ScheduledThread::get_need_flush(std::vector<QueueIter> &need_flush) {
    Lock l(_mutex);
    auto iter = _schedulers.begin(), end = _schedulers.end();
    while (iter != end) {
        if ((*iter)->need_flush)
            need_flush.push_back(iter);
        ++iter;
    }
}

void ScheduledThread::flush_need(std::vector<QueueIter> &need_flush) {
    for (auto iter : need_flush) {
        if (likely((*iter)->waiting_tasks.load(std::memory_order_acquire) == 0)) {
            (*iter)->force_invoke();
        } else {
            get_readyQueue();
            invoke();
        }
    }
    need_flush.clear();
}

void ScheduledThread::set_need_flush(std::vector<QueueIter> &need_flush) {
    Lock l(_mutex);
    if (need_flush.capacity() > _schedulers.size() * 4)
        need_flush.resize(_schedulers.size() * 4);
    auto iter = _schedulers.begin(), end = _schedulers.end();
    while (iter != end) {
        if ((*iter)->shutdown) {
            auto temp = iter++;
            if ((*temp)->waiting_tasks.load(std::memory_order_acquire) == 0)
                _schedulers.erase(temp);
        } else {
            (*iter)->need_flush = true;
            ++iter;
        }
    }
}

void ScheduledThread::invoke() {
    for (auto [scheduler, arg] : _invoking) {
        (*scheduler)->invoke(arg);
        (*scheduler)->waiting_tasks.fetch_sub(1, std::memory_order_release);
    }
    _invoking.clear();
}

void ScheduledThread::closing() {
    auto iter = _schedulers.end(), end = _schedulers.end();
    while (iter != end) {
        if (!(*iter)->shutdown)
            (*iter)->force_invoke();
        ++iter;
    }
    Lock l(_mutex);
    for (auto [scheduler, arg] : _ready)
        (*scheduler)->invoke(arg);
}
