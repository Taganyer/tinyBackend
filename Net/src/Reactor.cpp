//
// Created by taganyer on 3/8/24.
//

#include "../Reactor.hpp"
#include "Net/monitors/Selector.hpp"
#include "Net/monitors/Poller.hpp"
#include "Net/monitors/EPoller.hpp"
#include "Base/Thread.hpp"

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
    close();
    while (_running);
    delete _monitor;
    G_TRACE << "Reactor close.";
}

void Reactor::add_NetLink(NetLink::LinkPtr &netLink, Event event) {
    assert(running() && netLink->fd() == event.fd);
    _loop->put_event([this, event, weak = netLink->weak_from_this()] {
        auto ptr = weak.lock();
        if (!ptr) return;
        auto [iter, success] = _map.try_emplace(ptr->fd(), weak, _queue.end());
        if (success) {
            if (_monitor->add_fd(event)) {
                _queue.insert(_queue.end(), Base::Unix_to_now(), event);
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
    Thread thread([this, monitor_timeoutMS] {
        EventLoop loop;
        std::vector<Event> active;

        _loop = &loop;
        _monitor->set_tid(Base::tid());
        _running = true;

        loop.set_distributor([this, monitor_timeoutMS, &active] {
            remove_timeouts();
            if (_queue.size() > 0) {
                invoke(monitor_timeoutMS, active);
                _loop->weak_up();
            }
        });

        loop.loop();

        _monitor->remove_all();
        _loop = nullptr;
        _running = false;
    });
    thread.start();
    while (!_running);
}

void Reactor::close() {
    if (running()) {
        _loop->put_event([this] {
            _loop->shutdown();
        });
    }
}

void Reactor::remove_timeouts() {
    Time_difference time = Base::Unix_to_now() - timeout;
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
        auto iter = _map.find(event->fd);
        assert(iter != _map.end());

        auto ptr = iter->second.first.lock();
        if (!ptr) {
            _monitor->remove_fd(event->fd);
            _queue.erase(iter->second.second);
            _map.erase(iter);
            continue;
        }

        if (event->hasError() && !event->hasHangUp())
            ptr->handle_error({ error_types::ErrorEvent, 0 }, event);

        if (event->canRead() && !event->hasHangUp())
            ptr->handle_read(event);

        if (event->canWrite() && !event->hasHangUp())
            ptr->handle_write(event);

        if (event->hasHangUp()) {
            ptr->handle_close();
            _queue.erase(iter->second.second);
            _monitor->remove_fd(event->fd);
            _map.erase(iter);
            continue;
        }

        iter->second.second->first = Base::Unix_to_now();
        _queue.move_to(_queue.end(), iter->second.second);
    }

    list.clear();

}
