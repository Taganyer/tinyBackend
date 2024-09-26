//
// Created by taganyer on 24-9-25.
//

#include "../BalancedReactor.hpp"
#include "Base/Thread.hpp"
#include "Net/EventLoop.hpp"
#include "Base/ThreadPool.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Net/monitors/Poller.hpp"
#include "Net/monitors/EPoller.hpp"
#include "Net/monitors/Selector.hpp"

using namespace Net;

using namespace Base;

BalancedReactor::BalancedReactor(MOD mod, Time_difference link_timeout) :
    timeout(link_timeout) {
    switch (mod) {
    case SELECT:
        _monitor = new Selector();
        break;
    case POLL:
        _monitor = new Poller();
        break;
    case EPOLL:
        _monitor = new EPoller();
        break;
    }
    G_TRACE << "BalancedReactor create.";
}

BalancedReactor::~BalancedReactor() {
    if (running()) {
        _loop->put_event([this] {
            _loop->shutdown();
        });
    }
    Lock l(_mutex);
    _condition.wait(l, [this] {
        return !_running.load(std::memory_order_acquire);
    });
    G_TRACE << "BalancedReactor has been destroyed.";
}

void BalancedReactor::add_NetLink(NetLink::LinkPtr &netLink, Event event) {
    assert(running() && netLink->fd() == event.fd);
    _loop->put_event([this, event, weak = netLink->weak_from_this()] {
        auto ptr = weak.lock();
        if (!ptr) return;
        G_TRACE << "BalancedReactor add NetLink " << ptr->fd();
        auto [iter, success] = _map.try_emplace(ptr->fd(), weak, _queue.end());
        if (success) {
            Event _event = event;
            Event::write_to_extra_data(_event, iter);
            if (_monitor->add_fd(_event)) {
                _queue.insert(_queue.end(), Unix_to_now(), _event);
                iter->second.second = _queue.tail();
                return;
            }
            _map.erase(iter);
        }
        G_FATAL << "BalancedReactor add NetLink " << ptr->fd() << " failed.";
    });
}

void BalancedReactor::start(int monitor_timeoutMS, uint32 thread_size) {
    assert(!running() && _monitor);
    G_TRACE << "BalancedReactor start.";
    Thread thread([this, monitor_timeoutMS, thread_size] {
        _loop = new EventLoop();
        _monitor->set_tid(CurrentThread::tid());
        _thread_pool = new ThreadPool(thread_size, DEFAULT_TASK_SIZE);

        std::vector<Event> active;
        _running.store(true, std::memory_order_release);
        _condition.notify_one();

        _loop->set_distributor([this, monitor_timeoutMS, &active] {
            remove_timeouts();
            if (_queue.size() > 0) {
                invoke(monitor_timeoutMS, active, *_thread_pool);
                _loop->weak_up();
            }
        });
        _loop->loop();

        close_alive();
        delete _thread_pool;
        delete _monitor;
        delete _loop;
        _loop = nullptr;
        _thread_pool = nullptr;
        _running.store(false, std::memory_order_release);
        _condition.notify_all();
    });

    thread.start();
    Lock l(_mutex);
    _condition.wait(l, [this] {
        return _running.load(std::memory_order_acquire);
    });
}

bool BalancedReactor::in_reactor_thread() const {
    return _loop->object_in_thread()
        || _thread_pool->current_thread_in_this_pool();
}

void BalancedReactor::remove_timeouts() {
    Time_difference time = Unix_to_now() - timeout;
    auto iter = _queue.begin(), end = _queue.end();
    while (iter != end && iter->first <= time) {
        auto m = _map.find(iter->second.fd);
        auto ptr = m->second.first.lock();
        if (!ptr || ptr->handle_timeout()) {
            _monitor->remove_fd(iter->second.fd);
            _map.erase(m);
            _queue.erase(iter++);
        } else ++iter;
    }
}

void BalancedReactor::invoke(int timeoutMS, std::vector<Event> &list, ThreadPool &pool) {
    _running_tasks.store(_monitor->get_aliveEvent(timeoutMS, list));
    if (_running_tasks < 0) return;

    uint32 ret = _running_tasks;
    if (pool.get_max_queues() < ret)
        pool.resize_max_queue(ret);

    for (Event* event = list.data(); ret > 0; --ret, ++event) {
        create_task(event, pool);
    }

    while (pool.stolen_a_task()) {}

    Lock l(_mutex);
    _condition.wait(l, [this] {
        return _running_tasks.load(std::memory_order_acquire) == 0;
    });

    list.clear();
}

void BalancedReactor::create_task(Event* event, ThreadPool &pool) {
    auto iter = Event::get_extra_data<LinkMap::iterator>(*event);
    assert(iter != _map.end());
    assert(iter->first == event->fd);

    pool.submit([this, iter, event] {
        auto ptr = iter->second.first.lock();
        if (!ptr) {
            erase_link(iter);
            return;
        }

        if (event->hasError() && !event->hasHangUp())
            ptr->handle_error({ error_types::ErrorEvent, 0 }, event);

        if (event->canRead() && !event->hasHangUp())
            ptr->handle_read(event);

        if (event->canWrite() && !event->hasHangUp())
            ptr->handle_write(event);

        if (event->hasHangUp()) {
            ptr->handle_close();
            erase_link(iter);
            return;
        }

        Lock l(_mutex);
        iter->second.second->first = Unix_to_now();
        _queue.move_to(_queue.end(), iter->second.second);
        if (_running_tasks.fetch_sub(1, std::memory_order_release) == 1) {
            _condition.notify_one();
        }
    });
}

void BalancedReactor::erase_link(LinkMap::iterator iter) {
    Lock l(_mutex);
    _monitor->remove_fd(iter->first);
    _queue.erase(iter->second.second);
    _map.erase(iter);
    if (_running_tasks.fetch_sub(1, std::memory_order_release) == 1) {
        _condition.notify_one();
    }
}

void BalancedReactor::close_alive() {
    Lock l(_mutex);
    if (!_map.empty()) {
        G_WARN << "BalancedReactor force remove " << _map.size() << " NetLink.";
        for (const auto &[fd, data] : _map) {
            auto ptr = data.first.lock();
            if (!ptr) continue;
            Event event { fd, 0 };
            ptr->handle_error({ error_types::UnexpectedShutdown, 0 }, &event);
            if (event.hasHangUp()) ptr->handle_close();
        }
        _map.clear();
        _queue.erase(_queue.begin(), _queue.end());
    }
}


void BalancedReactor::update_link(Event event) {
    Lock l(_mutex);
    auto iter = _map.find(event.fd);
    if (iter == _map.end()) return;
    iter->second.second->second.event = event.event;
    _monitor->update_fd(iter->second.second->second);
}
