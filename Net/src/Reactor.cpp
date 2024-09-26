//
// Created by taganyer on 3/8/24.
//

#include "../Reactor.hpp"
#include "Base/Thread.hpp"
#include "Net/EventLoop.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Net/monitors/Poller.hpp"
#include "Net/monitors/EPoller.hpp"
#include "Net/monitors/Selector.hpp"

using namespace Net;

using namespace Base;


Reactor::Reactor(MOD mod, Time_difference link_timeout) :
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
    G_TRACE << "Reactor create.";
}

Reactor::~Reactor() {
    if (running()) {
        _loop->put_event([this] {
            _loop->shutdown();
        });
    }
    while (_running) CurrentThread::yield_this_thread();
    G_TRACE << "Reactor has been destroyed.";
}

void Reactor::add_NetLink(NetLink::LinkPtr &netLink, Event event) {
    assert(running() && netLink->fd() == event.fd);
    _loop->put_event([this, event, weak = netLink->weak_from_this()] {
        auto ptr = weak.lock();
        if (!ptr) return;
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
        G_FATAL << "Reactor add NetLink " << ptr->fd() << " failed.";
    });
}

void Reactor::start(int monitor_timeoutMS) {
    assert(!running() && _monitor);
    G_TRACE << "Reactor start.";
    Thread thread([this, monitor_timeoutMS] {
        _loop = new EventLoop();
        _monitor->set_tid(CurrentThread::tid());

        std::vector<Event> active;
        _running = true;

        _loop->set_distributor([this, monitor_timeoutMS, &active] {
            remove_timeouts();
            if (_queue.size() > 0) {
                invoke(monitor_timeoutMS, active);
                _loop->weak_up();
            }
        });
        _loop->loop();

        close_alive();
        delete _monitor;
        delete _loop;
        _loop = nullptr;
        _running = false;
    });

    thread.start();
    while (!_running) CurrentThread::yield_this_thread();
}

bool Reactor::in_reactor_thread() const {
    return _loop->object_in_thread();
}

void Reactor::remove_timeouts() {
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

void Reactor::invoke(int timeoutMS, std::vector<Event> &list) {
    int ret = _monitor->get_aliveEvent(timeoutMS, list);
    if (ret < 0) return;

    for (Event* event = list.data(); ret > 0; --ret, ++event) {
        create_task(event);
    }

    list.clear();
}

void Reactor::create_task(Event* event) {
    auto iter = Event::get_extra_data<LinkMap::iterator>(*event);
    assert(iter != _map.end());
    assert(iter->first == event->fd);

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

    iter->second.second->first = Unix_to_now();
    _queue.move_to(_queue.end(), iter->second.second);
}

void Reactor::erase_link(LinkMap::iterator iter) {
    _monitor->remove_fd(iter->first);
    _queue.erase(iter->second.second);
    _map.erase(iter);
}

void Reactor::close_alive() {
    if (!_map.empty()) {
        G_WARN << "Reactor force remove " << _map.size() << " NetLink.";
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

void Reactor::update_link(Event event) {
    auto iter = _map.find(event.fd);
    if (iter == _map.end()) return;
    iter->second.second->second.event = event.event;
    _monitor->update_fd(iter->second.second->second);
}
