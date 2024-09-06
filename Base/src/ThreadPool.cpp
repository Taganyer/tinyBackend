//
// Created by taganyer on 24-5-10.
//

#include "../ThreadPool.hpp"
#include "Base/Thread.hpp"

using namespace Base;


ThreadPool::ThreadPool(uint32 thread_size, uint32 max_task_size) :
        _max_tasks(max_task_size) {
    create_threads(thread_size);
}

ThreadPool::~ThreadPool() {
    shutdown();
    Lock l(lock);
    _submit.wait(l, [this]() {
        return _state.load() == TERMINATED;
    });
}

void ThreadPool::stop() {
    if (_state.load() == RUNNING) {
        _state.store(STOP);
        _submit.notify_all();
    }
}

void ThreadPool::start() {
    if (_state.load() == STOP) {
        Lock l(lock);
        _state.store(RUNNING);
        _submit.notify_all();
        _consume.notify_all();
    }
}

void ThreadPool::clear_task() {
    Lock l(lock);
    for (const auto &fun: _list)
        fun(true);
    _list.erase_after(_list.before_begin(), _list.end());
}

void ThreadPool::shutdown() {
    Lock l(lock);

    if (_state.load() > STOP) return;
    _state.store(SHUTTING);

    for (const auto &fun: _list)
        fun(true);
    _list.erase_after(_list.before_begin(), _list.end());

    for (Size i = 0; i < _core_threads; ++i) {
        _list.insert_after(_list.before_begin(), [](bool) {
            return true;
        });
    }
    _consume.notify_all();
}

void ThreadPool::resize_core_thread(uint32 size) {
    if (size > _core_threads) {
        create_threads(size - _core_threads);
    } else if (size < _core_threads) {
        delete_threads(_core_threads - size);
    }
}

void ThreadPool::create_threads(ThreadPool::Size size) {
    // FIXME 并发调用时，函数可能会返回时间过早。
    Size record = _core_threads.load() + size;
    for (Size i = 0; i < size; ++i) {
        Thread thread([this] {
            _core_threads.fetch_add(1);
            CurrentThread::thread_name().append("(pool)");
            while (true) {
                Fun fun;
                {
                    Lock l(lock);
                    _submit.notify_one();
                    _consume.wait(l, [this] {
                        return _list.size() > 0 && _state.load() != STOP;
                    });
                    fun = std::move(*_list.begin());
                    _list.erase_after(_list.end());
                }
                if (fun(false))
                    break;
            }
            end_thread();
        });
        thread.start();
    }

    Lock l(lock);
    _submit.wait(l, [this, record] { return _core_threads >= record; });
    if (_state.load() == TERMINATED)
        _state.store(RUNNING);
}

void ThreadPool::delete_threads(ThreadPool::Size size) {
    Lock l(lock);
    for (Size i = 0; i < size; ++i) {
        _list.insert_after(_list.before_begin(), [](bool) {
            return true;
        });
    }
}

void ThreadPool::end_thread() {
    if (_core_threads.fetch_sub(1) == 1) {
        _state.store(TERMINATED);
        _submit.notify_all();
    }
}

bool ThreadPool::waiting(Lock<Mutex> &l) {
    _submit.wait(l, [this] {
        return stopping() || _list.size() < _max_tasks;
    });
    return _state.load() == RUNNING;
}
