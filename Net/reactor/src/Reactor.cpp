//
// Created by taganyer on 3/8/24.
//

#include "../Reactor.hpp"
#include "Net/EventLoop.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Net/Controller.hpp"
#include "Net/monitors/Poller.hpp"
#include "Net/monitors/EPoller.hpp"
#include "Net/monitors/Selector.hpp"

using namespace Net;

using namespace Base;


void Reactor::start(MOD mod, int monitor_timeoutMS, FixedFun fun) {
    if (running()) return;

    _thread = Thread([this, mod, monitor_timeoutMS, _fun = std::move(fun)] {
        std::vector<Event> active;
        create_source(mod);

        _loop->set_distributor([this, monitor_timeoutMS, &_fun, &active] {
            if (_fun) _fun();
            remove_timeouts();
            if (_queue.size() > 0) {
                invoke(monitor_timeoutMS, active);
                _loop->weak_up();
            }
        });
        _loop->loop();

        close_alive();
        destroy_source();
    });
    _thread.start();

    while (!running()) CurrentThread::yield_this_thread();
    G_TRACE << "Reactor start.";
}

void Reactor::stop() {
    if (!running()) return;
    _loop->put_event([this] { _loop->shutdown(); });
    _thread.join();
    G_TRACE << "Reactor stop.";
}

void Reactor::add_NetLink(NetLink::LinkPtr &netLink, Event event) {
    assert(running() && netLink->fd() == event.fd);
    _loop->put_event([this, event, shared = netLink->shared_from_this()] {
        if (!shared->valid()) return;
        auto [iter, success] = _map.try_emplace(shared->fd(), shared, event);
        if (success) {
            Event _event = event;
            _event.get_extra_data<LinkMap::iterator>() = iter;
            if (_monitor->add_fd(_event)) {
                _queue.insert(_queue.end(), shared->fd());
                iter->second.timer_iter = _queue.tail();
                G_TRACE << "Reactor add NetLink " << shared->fd();
                return;
            }
            _map.erase(iter);
        }
        G_FATAL << "Reactor add NetLink " << shared->fd() << " failed.";
    });
}

void Reactor::remove_NetLink(int fd) {
    assert(running());
    _loop->put_event([this, fd] {
        auto iter = _map.find(fd);
        if (iter == _map.end()) return;
        auto ptr = iter->second.link;
        Event event { fd };
        event.set_NoEvent();
        Controller controller(*ptr, event, *this, iter->second.event.event);
        erase_link(iter);
        ptr->handle_close(controller);
    });
}

void Reactor::send_to_loop(std::function<void()> fun) const {
    assert(running());
    _loop->put_event(std::move(fun));
}

void Reactor::update_link(Event event) {
    if (!in_reactor_thread()) {
        _loop->put_event([this, event] {
            update_link(event);
        });
        return;
    }
    auto iter = _map.find(event.fd);
    if (iter == _map.end() || !iter->second.link->valid()) return;
    iter->second.event = event;
    event.get_extra_data<LinkMap::iterator>() = iter;
    _monitor->update_fd(event);
}

void Reactor::weak_up_link(int fd, WeakUpFun fun) {
    _loop->put_event([this, fd, fun_ = std::move(fun)] {
        auto iter = _map.find(fd);
        if (iter == _map.end()) return;
        Event event(iter->second.event);
        Controller controller(*iter->second.link, event,
                              *this, iter->second.event.event);
        fun_(controller);
        if (!event.is_NoEvent()) {
            event.get_extra_data<LinkMap::iterator>() = iter;
            create_task(&event);
        }
    });
}

void Reactor::weak_up_all_link(WeakUpFun fun) {
    _loop->put_event([this, fun_ = std::move(fun)] {
        for (auto iter = _map.begin(); iter != _map.end(); ++iter) {
            Event event(iter->second.event);
            Controller controller(*iter->second.link, event,
                                  *this, iter->second.event.event);
            fun_(controller);
            if (!event.is_NoEvent()) {
                event.get_extra_data<LinkMap::iterator>() = iter;
                create_task(&event);
            }
        }
    });
}

bool Reactor::in_reactor_thread() const {
    return _loop->object_in_thread();
}

void Reactor::create_source(MOD mod) {
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
    _monitor->set_tid(CurrentThread::tid());
    _loop = new EventLoop();
    _running.store(true, std::memory_order_release);
}

void Reactor::destroy_source() {
    _running.store(false, std::memory_order_release);
    delete _loop;
    _loop = nullptr;
    delete _monitor;
    _monitor = nullptr;
}

void Reactor::remove_timeouts() {
    TimeInterval time = Unix_to_now() - timeout;
    auto iter = _queue.begin(), end = _queue.end();
    while (iter != end && iter->flush_time <= time) {
        auto m = _map.find(iter->fd);
        ++iter;
        auto ptr = m->second.link;
        Event event { ptr->fd() };
        event.set_NoEvent();
        Controller controller(*ptr, event, *this, m->second.event.event);
        if (ptr->handle_timeout(controller)) {
            erase_link(m);
            ptr->handle_close(controller);
        }
    }
}

void Reactor::invoke(int timeoutMS, std::vector<Event> &list) {
    int ret = _monitor->get_aliveEvent(timeoutMS, list);
    if (ret < 0) return;

    for (Event* event = list.data(); ret > 0; --ret, ++event) {
        create_task(event);
    }

    list.clear();
}

void Reactor::create_task(Event* event) {
    auto iter = event->get_extra_data<LinkMap::iterator>();
    assert(iter != _map.end());
    assert(iter->first == event->fd);

    auto ptr = iter->second.link;
    if (!ptr->valid()) {
        erase_link(iter);
        return;
    }
    event->extra_data = iter->second.event.extra_data;

    Controller controller(*ptr, *event, *this, iter->second.event.event);

    if (event->hasError() && !event->hasHangUp())
        ptr->handle_error({ error_types::ErrorEvent, 0 }, controller);

    if (event->canRead() && !event->hasHangUp())
        ptr->handle_read(controller);

    if (event->canWrite() && !event->hasHangUp())
        ptr->handle_write(controller);

    if (event->hasHangUp()) {
        erase_link(iter);
        ptr->handle_close(controller);
        return;
    }

    iter->second.timer_iter->flush_time = Unix_to_now();
    _queue.move_to(_queue.end(), iter->second.timer_iter);
}

void Reactor::erase_link(LinkMap::iterator iter) {
    _monitor->remove_fd(iter->first, !iter->second.link->valid());
    _queue.erase(iter->second.timer_iter);
    _map.erase(iter);
}

void Reactor::close_alive() {
    if (!_map.empty()) {
        G_WARN << "Reactor force remove " << _map.size() << " NetLink.";
        for (auto &[fd, data] : _map) {
            auto &ptr = data.link;
            if (!ptr->valid()) continue;
            Event event { fd, 0 };
            Controller controller(*ptr, event, *this, data.event.event);
            ptr->handle_error({ error_types::UnexpectedShutdown, 0 }, controller);
            if (event.hasHangUp())
                ptr->handle_close(controller);
        }
        _map.clear();
        _queue.erase(_queue.begin(), _queue.end());
    }
}
