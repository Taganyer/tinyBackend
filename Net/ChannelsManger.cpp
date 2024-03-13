//
// Created by taganyer on 24-3-2.
//

#include "ChannelsManger.hpp"
#include "Channel.hpp"
#include "Poller.hpp"
#include "../Base/Log/Log.hpp"

using namespace Base;

using namespace Net;

ChannelsManger::ChannelsManger(EventLoop *loop, Poller *poller) :
        _loop(loop), _poller(poller) {}

ChannelsManger::~ChannelsManger() {
    close();
}

void ChannelsManger::add_channel(Channel *channel) {
    assert_in_right_thread();
    if (!channel) return;
    _poller->add_channel(channel);
    channel->_prev = nullptr;
    channel->_next = queue_head;
    queue_head = channel;
    ++_size;
}

void ChannelsManger::remove_channel(Channel *channel) {
    assert_in_right_thread();

    if (channel->_prev) channel->_prev->_next = channel->_next;
    else queue_head = channel->_next;
    if (channel->_next) channel->_next->_prev = channel->_prev;
    else queue_tail = channel->_prev;

    channel->_next = remove_head;
    remove_head->_prev = channel;
    remove_head = channel;
}

void ChannelsManger::set_timeout(const Base::Time_difference &timeout) {
    assert_in_right_thread();
    _timeout = timeout;
}

void ChannelsManger::assert_in_right_thread() const {
    if (unlikely(Base::tid() != _tid)) {
        G_WARN << "Channels " << _tid << " assert in " << Base::tid();
        assert("ChannelsManger in wrong thread");
    }
}

void ChannelsManger::update() {
    assert_in_right_thread();
    remove_timeout_channels();
    remove();
}

void ChannelsManger::put_to_top(Channel *channel) {
    if (channel->_prev) channel->_prev->_next = channel->_next;
    else return;
    if (channel->_next) channel->_next->_prev = channel->_prev;
    else queue_tail = channel->_prev;

    channel->_prev = nullptr;
    channel->_next = queue_head;
    queue_head = channel;
}

void ChannelsManger::remove_timeout_channels() {
    if (_timeout <= 0 || !queue_head) return;
    auto time = Unix_to_now() - _timeout;
    Channel *record = queue_tail;
    while (queue_tail && queue_tail->timeout(time)) {
        queue_tail = queue_tail->_prev;
    }
    if (queue_tail == record) return;
    record->_next = remove_head;
    if (remove_head) remove_head->_prev = record;
    if (!queue_tail) {
        remove_head = queue_head;
        queue_head = nullptr;
    } else {
        remove_head = queue_tail->_next;
        queue_tail->_next = nullptr;
    }
    remove_head->_prev = nullptr;
}

void ChannelsManger::close() {
    assert_in_right_thread();
    if (_size != 0)
        G_WARN << "ChannelsManger " << _tid << " has " << _size << " channels close abnormal";
    Channel *temp;
    while (queue_head) {
        temp = queue_head;
        queue_head = queue_head->_next;
        Channel::destroy_Channel(temp);
    }
    while (remove_head) {
        temp = remove_head;
        remove_head = remove_head->_next;
        Channel::destroy_Channel(temp);
    }
}

void ChannelsManger::remove() {
    Channel *temp = remove_head;
    remove_head = nullptr;
    while (temp) {
        Channel *t = temp->_next;
        if (!temp->is_nonevent())
            temp->set_nonevent();
        if (temp->has_aliveEvent()) {
            temp->_next = remove_head;
            if (remove_head) remove_head->_prev = temp;
            remove_head = temp;
        } else {
            _poller->remove_channel(temp->fd());
            Channel::destroy_Channel(temp);
            --_size;
        }
        temp = t;
    }
}
