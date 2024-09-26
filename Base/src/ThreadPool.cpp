//
// Created by taganyer on 24-9-25.
//

#include "../ThreadPool.hpp"
#include "Base/Thread.hpp"

using namespace Base;

ThreadPool::ThreadPool(uint32 threads_size, uint32 max_tasks_size) :
    _max_tasks(max_tasks_size) {
    _list.reserve(max_tasks_size);
    resize_core_threads(threads_size);
}

ThreadPool::~ThreadPool() {
    shutdown();
}

bool ThreadPool::stolen_a_task() {
    Fun fun;
    bool get = false;
    {
        Lock l(_lock);
        if (!_list.empty()) {
            fun = std::move(_list.back());
            _list.pop_back();
            _submit.notify_one();
            get = true;
        }
    }
    if (fun) fun(false);
    return get;
}

void ThreadPool::stop() {
    if (_state.load(std::memory_order_consume) == RUNNING) {
        _state.store(STOP, std::memory_order_release);
        _submit.notify_all();
    }
}

void ThreadPool::start() {
    if (_state.load(std::memory_order_consume) == STOP) {
        Lock l(_lock);
        _state.store(RUNNING, std::memory_order_release);
        _submit.notify_all();
        _consume.notify_all();
    }
}

void ThreadPool::clear_task() {
    Lock l(_lock);
    for (const auto &fun : _list)
        fun(true);
    _list.clear();
}

void ThreadPool::shutdown() {
    Lock l(_lock);

    if (_state.load(std::memory_order_acquire) > STOP) return;
    _state.store(SHUTTING, std::memory_order_release);

    for (const auto &fun : _list)
        fun(true);
    _list.clear();

    _target_thread_size.store(0, std::memory_order_release);
    _consume.notify_all();

    _submit.wait(l, [this] {
        return _state.load(std::memory_order_acquire) == TERMINATED;
    });
}

void ThreadPool::resize_core_threads(uint32 size) {
    if (_state.load(std::memory_order_acquire) == SHUTTING)
        return;
    _target_thread_size.store(size, std::memory_order_release);
    _consume.notify_all();
    while (_core_threads.load(std::memory_order_consume) <
        _target_thread_size.load(std::memory_order_consume)) {
        create_thread();
    }
}

void ThreadPool::resize_max_queue(uint32 size) {
    Lock l(_lock);
    if (size > _list.size()) _list.shrink_to_fit();
    else _list.resize(size);
    _max_tasks = size;
}

bool ThreadPool::current_thread_in_this_pool() const {
    return this == CurrentThread::thread_mark_ptr();
}

bool ThreadPool::need_delete() const {
    return _target_thread_size.load(std::memory_order_acquire)
        < _core_threads.load(std::memory_order_relaxed);
}

void ThreadPool::create_thread() {
    std::atomic_bool create_done(false);
    Thread thread([this, &create_done] {
        thread_begin(create_done);
        while (true) {
            Fun fun;
            {
                Lock l(_lock);
                _submit.notify_one();
                _consume.wait(l, [this] {
                    return !_list.empty()
                        && _state.load(std::memory_order_acquire) != STOP
                        || need_delete();
                });
                if (need_delete()) {
                    _core_threads.fetch_sub(1);
                    break;
                }
                if (_list.empty()
                    || _state.load(std::memory_order_acquire) == STOP) {
                    continue;
                }
                fun = std::move(_list.back());
                _list.pop_back();
            }
            fun(false);
        }
        thread_end();
    });
    thread.start();

    Lock l(_lock);
    _submit.wait(l, [this, &create_done] {
        return create_done.load(std::memory_order_acquire);
    });
}

void ThreadPool::thread_begin(std::atomic<bool> &create_done) {
    _core_threads.fetch_add(1, std::memory_order_release);
    create_done.store(true, std::memory_order_release);
    _submit.notify_all();
    CurrentThread::thread_name().append("(pool)");
    CurrentThread::thread_mark_ptr() = this;
    if (_state.load(std::memory_order_consume) == TERMINATED) {
        _state.store(RUNNING, std::memory_order_release);
    }
}

void ThreadPool::thread_end() {
    if (_core_threads.load(std::memory_order_consume) == 0) {
        _state.store(TERMINATED, std::memory_order_release);
        _submit.notify_all();
    }
}
