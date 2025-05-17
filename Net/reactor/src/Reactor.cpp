//
// Created by taganyer on 3/8/24.
//

#include "../Reactor.hpp"
#include "Net/EventLoop.hpp"
#include "Base/SystemLog.hpp"
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

void Reactor::add_channel(MessageAgentPtr &&ptr, Channel channel, Event monitor_event) {
    assert(running() && ptr->fd() == monitor_event.fd && ptr->fd() > 0);
    if (!ptr || !ptr->agent_valid()) return;
    _loop->put_event([this, monitor_event,
        channel_data = new ChannelData(std::move(ptr), std::move(channel), monitor_event)] {
        int fd = channel_data->agent->fd();
        Event _event = monitor_event;
        auto [iter, success] = _map.try_emplace(fd, std::move(*channel_data));
        delete channel_data;
        if (success) {
            _event.get_extra_data<ChannelMap::iterator>() = iter;
            if (_monitor->add_fd(_event)) {
                _queue.insert(_queue.end(), fd);
                iter->second.timer_iter = _queue.tail();
                G_TRACE << "Reactor add MessageAgent " << fd;
                return;
            }
            _map.erase(iter);
        }
        G_FATAL << "Reactor add Channel " << fd << " failed.";
    });
}

void Reactor::send_to_loop(std::function<void()> fun) const {
    assert(running());
    _loop->put_event(std::move(fun));
}

void Reactor::update_channel(Event monitor_event) {
    if (!in_reactor_thread()) {
        _loop->put_event([this, monitor_event] {
            update_channel(monitor_event);
        });
        return;
    }
    auto iter = _map.find(monitor_event.fd);
    if (iter == _map.end()) return;
    assert(iter->second.agent->agent_valid());
    iter->second.monitor_event = monitor_event;
    monitor_event.get_extra_data<ChannelMap::iterator>() = iter;
    _monitor->update_fd(monitor_event);
}

void Reactor::weak_up_channel(int fd, WeakUpFun fun) {
    _loop->put_event([this, fd, weak_up_fun = std::move(fun)] {
        auto iter = _map.find(fd);
        if (iter == _map.end()) return;
        auto &agent = *iter->second.agent;
        auto &channel = iter->second.channel;
        agent.set_running_thread();
        weak_up_fun(agent, channel);
        if (!agent.agent_valid()) {
            erase_channel(iter);
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
        auto tmp = _map.find(iter->fd);
        ++iter;
        auto &agent = *tmp->second.agent;
        auto &channel = tmp->second.channel;
        agent.socket_event.set_error();
        agent.error = { error_types::TimeoutEvent, agent.fd() };
        channel.invoke_event(agent);
        if (!agent.agent_valid()) {
            erase_channel(tmp);
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
    auto iter = event->get_extra_data<ChannelMap::iterator>();
    assert(iter != _map.end());
    assert(iter->first == event->fd);

    auto &agent = *iter->second.agent;
    auto &channel = iter->second.channel;
    agent.socket_event.event = event->event;
    channel.invoke_event(agent);
    if (!agent.agent_valid()) {
        erase_channel(iter);
        return;
    }
    iter->second.timer_iter->flush_time = Unix_to_now();
    _queue.move_to(_queue.end(), iter->second.timer_iter);
}

void Reactor::erase_channel(ChannelMap::iterator iter) {
    _monitor->remove_fd(iter->first, !iter->second.agent || !iter->second.agent->agent_valid());
    _queue.erase(iter->second.timer_iter);
    _map.erase(iter);
}

void Reactor::close_alive() {
    if (!_map.empty()) {
        G_WARN << "Reactor force remove " << _map.size() << " NetLink.";
        for (auto &[fd, data] : _map) {
            auto &agent = *data.agent;
            auto &channel = data.channel;
            agent.socket_event.set_error();
            agent.error = { error_types::UnexpectedShutdown, 0 };
            channel.invoke_event(agent);
            if (agent.agent_valid()) {
                agent.socket_event.set_HangUp();
                channel.invoke_event(agent);
            }
        }
        _map.clear();
        _queue.erase(_queue.begin(), _queue.end());
    }
}
