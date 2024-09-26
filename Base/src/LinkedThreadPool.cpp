//
// Created by taganyer on 24-5-10.
//

#include "../LinkedThreadPool.hpp"
#include "Base/Thread.hpp"

using namespace Base;


LinkedThreadPool::LinkedThreadPool(uint32 threads_size, uint32 max_tasks_size) :
    _max_tasks(max_tasks_size) {
    create_threads(threads_size);
}

LinkedThreadPool::~LinkedThreadPool() {
    shutdown();
    Lock l(_lock);
    _submit.wait(l, [this] {
        return _state.load(std::memory_order_acquire) == TERMINATED;
    });
}

void LinkedThreadPool::stop() {
    if (_state.load(std::memory_order_consume) == RUNNING) {
        _state.store(STOP, std::memory_order_release);
        _submit.notify_all();
    }
}

void LinkedThreadPool::start() {
    if (_state.load(std::memory_order_consume) == STOP) {
        Lock l(_lock);
        _state.store(RUNNING, std::memory_order_release);
        _submit.notify_all();
        _consume.notify_all();
    }
}

void LinkedThreadPool::clear_task() {
    Lock l(_lock);
    for (const auto &fun : _list)
        fun(true);
    _list.erase_after(_list.before_begin(), _list.end());
}

void LinkedThreadPool::shutdown() {
    Lock l(_lock);

    if (_state.load(std::memory_order_acquire) > STOP) return;
    _state.store(SHUTTING, std::memory_order_release);

    for (const auto &fun : _list)
        fun(true);
    _list.erase_after(_list.before_begin(), _list.end());

    for (Size i = 0; i < _core_threads; ++i) {
        _list.insert_after(_list.before_begin(), [] (bool) {
            return true;
        });
    }
    _consume.notify_all();
}

void LinkedThreadPool::resize_core_threads(uint32 size) {
    if (size > _core_threads) {
        create_threads(size - _core_threads);
    } else if (size < _core_threads) {
        delete_threads(_core_threads - size);
    }
}

void LinkedThreadPool::create_threads(Size size) {
    // FIXME 并发调用时，函数可能会返回时间过早。
    Size record = _core_threads.load() + size;
    for (Size i = 0; i < size; ++i) {
        Thread thread([this] {
            _core_threads.fetch_add(1);
            CurrentThread::thread_name().append("(pool)");
            while (true) {
                Fun fun;
                {
                    Lock l(_lock);
                    _submit.notify_one();
                    _consume.wait(l, [this] {
                        return _list.size() > 0
                            && _state.load(std::memory_order_acquire) != STOP;
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

    Lock l(_lock);
    _submit.wait(l, [this, record] { return _core_threads >= record; });
    if (_state.load(std::memory_order_consume) == TERMINATED)
        _state.store(RUNNING, std::memory_order_release);
}

void LinkedThreadPool::delete_threads(Size size) {
    Lock l(_lock);
    for (Size i = 0; i < size; ++i) {
        _list.insert_after(_list.before_begin(), [] (bool) {
            return true;
        });
    }
}

void LinkedThreadPool::end_thread() {
    if (_core_threads.fetch_sub(1) == 1) {
        _state.store(TERMINATED, std::memory_order_release);
        _submit.notify_all();
    }
}
