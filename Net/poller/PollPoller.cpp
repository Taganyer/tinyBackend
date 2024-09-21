//
// Created by taganyer on 24-3-4.
//

#include <cassert>

#include "Channel.hpp"
#include "PollPoller.hpp"
#include "../functions/poll_interface.hpp"
#include "../../Base/Log/Log.hpp"

using namespace Net;

using namespace Base;

int PollPoller::poll(int timeoutMS, ChannelList &list) {
    assert_in_right_thread("PollPoller::poll ");
    int active = ops::poll(_eventsQueue.data(), _eventsQueue.size(), timeoutMS);
    if (active > 0) {
        get_events(list, active);
        G_TRACE << "PollPoller::poll " << _tid << " get " << active << " events";
    } else if (active == 0) {
        G_INFO << "PollPoller::poll " << _tid << " timeout " << timeoutMS << " ms";
    } else {
        G_ERROR << "PollPoller " << _tid << ' ' << ops::get_poll_error(errno);
    }
    return active;
}

void PollPoller::add_channel(Channel *channel) {
    assert_in_right_thread("PollPoller::add_channel ");
    assert(_channels.find(channel->fd()) == _channels.end());
    assert(_channels.try_emplace(channel->fd(), channel, _eventsQueue.size()).second);
    _eventsQueue.push_back({channel->fd(), 0, 0});
    G_TRACE << "PollPoller::add_channel " << channel->fd();
}

void PollPoller::remove_channel(int fd) {
    assert_in_right_thread("PollPoller::remove_channel ");
    auto sender = _channels.find(fd);
    assert(sender != _channels.end());
    if (int i = sender->second.second; i != _eventsQueue.size() - 1) {
        auto ib = _channels.find(_eventsQueue.back().fd);
        assert(ib != _channels.end());
        ib->second.second = i;
        _eventsQueue[i] = _eventsQueue.back();
        _eventsQueue.pop_back();
    }
    _channels.erase(sender);
    G_TRACE << "PollPoller::remove_channel " << fd;
}

void PollPoller::update_channel(Channel *channel) {
    assert_in_right_thread("PollPoller::update_channel ");
    int fd = channel->fd();
    auto sender = _channels.find(fd);
    assert(sender != _channels.end());
    auto &pollfd = _eventsQueue[sender->second.second];
    assert(pollfd.fd == fd || pollfd.fd == -fd - 1);
    pollfd.events = channel->events();
    pollfd.revents = 0;
    if (channel->is_nonevent()) pollfd.fd = -fd - 1;
    else pollfd.fd = fd;
}

void PollPoller::get_events(Poller::ChannelList &list, int size) {
    list.reserve(size + list.size());
    for (auto const & i : _eventsQueue) {
        if (i.revents <= 0)
            continue;
        auto sender = _channels.find(i.fd);
        assert(sender != _channels.end());
        sender->second.first->set_revents(i.revents);
        list.push_back(sender->second.first);
    }
}
