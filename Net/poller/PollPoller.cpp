//
// Created by taganyer on 24-3-4.
//

#include <poll.h>
#include <cassert>

#include "../Channel.hpp"
#include "PollPoller.hpp"
#include "../../Base/Log/Log.hpp"

using namespace Net;

using namespace Base;

void PollPoller::poll(int timeoutMS) {
    assert_in_right_thread("PollPoller::poll ");
    if (_active_size != 0) return;
    _index = 0;
    int active = ::poll(_eventsQueue.data(), _eventsQueue.size(), timeoutMS);
    if (active > 0) {
        _active_size = active;
        G_TRACE << "PollPoller::poll " << _tid << " get " << _active_size << " events";
    } else if (active == 0) {
        G_INFO << "PollPoller::poll " << _tid << " timeout " << timeoutMS << " ms";
    } else {
        G_ERROR << "PollPoller::poll " << _tid << " error";
    }
}

Channel *PollPoller::get_events() {
    assert_in_right_thread("PollPoller::get_event ");
    if (_active_size < 1) return nullptr;
    while (_eventsQueue[_index].revents <= 0)
        ++_index;
    auto iter = _channels.find(_eventsQueue[_index].fd);
    assert(iter != _channels.end());
    auto [channel, index] = iter->second;
    assert(index == _index);
    channel->set_revents(_eventsQueue[index].revents);
    ++_index;
    --_active_size;
    return channel;
}

void PollPoller::add_channel(ConnectionsManger *manger, Channel *channel) {
    assert_in_right_thread("PollPoller::add_channel ");
    assert_Manger_in_same_thread(*manger);
    assert(_channels.find(channel->fd()) == _channels.end());
    _channels.insert({channel->fd(), {channel, _eventsQueue.size()}});
    _eventsQueue.push_back({channel->fd(), 0, 0});
    G_TRACE << "PollPoller::add_channel " << channel->fd();
}

void PollPoller::remove_channel(ConnectionsManger *manger, int fd) {
    assert_in_right_thread("PollPoller::remove_channel ");
    assert_Manger_in_same_thread(*manger);
    auto iter = _channels.find(fd);
    assert(iter != _channels.end());
    if (int i = iter->second.second; i != _eventsQueue.size() - 1) {
        auto ib = _channels.find(_eventsQueue.back().fd);
        assert(ib != _channels.end());
        ib->second.second = i;
        _eventsQueue[i] = _eventsQueue.back();
        _eventsQueue.pop_back();
    }
    _channels.erase(iter);
    G_TRACE << "PollPoller::remove_channel " << fd;
}

void PollPoller::update_channel(Channel *channel) {
    assert_in_right_thread("PollPoller::update_channel ");
    int fd = channel->fd();
    auto iter = _channels.find(fd);
    assert(iter != _channels.end());
    auto &pollfd = _eventsQueue[iter->second.second];
    assert(pollfd.fd == fd || pollfd.fd == -fd - 1);
    pollfd.events = channel->events();
    pollfd.revents = 0;
    if (!channel->has_event()) pollfd.fd = -fd - 1;
}
