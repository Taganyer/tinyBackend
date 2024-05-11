//
// Created by taganyer on 3/8/24.
//

#include "Reactor.hpp"
#include "error/errors.hpp"
#include "monitors/Selector.hpp"
#include "monitors/Poller.hpp"
#include "monitors/EPoller.hpp"

using namespace Net;

using namespace Base;


Reactor::Reactor(MOD mod, Base::Time_difference link_timeout) :
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
    G_DEBUG << "Reactor create.";
}

Reactor::~Reactor() {
    close();
    delete _monitor;
}

void Reactor::add_NetLink(NetLink &netLink, Event event) {
    assert(running() && netLink.fd() == event.fd);
    _loop->put_event([this, event, weak = netLink.weak_from_this()] {
        auto ptr = weak.lock();
        if (!ptr) return;
        _queue.emplace_back(Base::Unix_to_now(), event);
        if (_map.try_emplace(ptr->fd(), weak, --_queue.end()).second) {
            if (!_monitor->add_fd(event))
                G_FATAL << "add NetLink " << ptr->fd() << " failed.";
        } else {
            _queue.pop_back();
        }
    });
}

void Reactor::start(int monitor_timeoutMS) {
    assert(!running() && _monitor);
    EventLoop loop;
    std::vector<Event> active;

    _loop = &loop;
    _monitor->set_tid(Base::tid());

    loop.set_distributor([this, monitor_timeoutMS, &active] {
        remove_timeouts();
        if (!_queue.empty())
            invoke(monitor_timeoutMS, active);
    });

    loop.loop();

    _monitor->remove_all();
    _monitor->set_tid(-1);
    _loop = nullptr;
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
        }
    }
    _queue.erase(_queue.begin(), iter);
}

void Reactor::invoke(int timeoutMS, std::vector<Event> &list) {
    int ret = _monitor->get_aliveEvent(timeoutMS, list);
    if (unlikely(ret < 0)) {
        G_ERROR << "Reactor in " << Base::tid() << ' '
                << ops::get_error(_monitor->error_);
        ret = 0;
    }

    for (Event *event = list.data(); ret > 0; --ret, ++event) {
        auto iter = _map.find(event->fd);
        auto ptr = iter->second.first.lock();

        if (!ptr) {
            _monitor->remove_fd(event->fd);
            _queue.erase(iter->second.second);
            _map.erase(iter);
            continue;
        }

        if (event->hasError()) {
            G_ERROR << "fd " << event->fd << " error() called";
            ptr->handle_error({error_types::ErrorEvent, 0}, event);
        }

        if (event->hasHangUp() && !event->canRead()) {
            G_TRACE << "fd " << event->fd << " is hang up and no data to read. close() called";
            ptr->handle_close(event);
        }

        if (event->canRead()) {
            ptr->handle_read(event);
        }

        if (event->canWrite()) {
            ptr->handle_write(event);
        }

        iter->second.second->first = Base::Unix_to_now();
        _queue.push_back(*iter->second.second);
        _queue.erase(iter->second.second);
    }

}
