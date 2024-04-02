//
// Created by taganyer on 3/28/24.
//

#include "SendThread.hpp"
#include "../Thread.hpp"

using namespace Base;

const Time_difference SendThread::FLUSH_TIME = 1s;

SendThread::SendThread() {
    Thread thread(string("SendThread"), [this] {
        std::vector<SenderIter> need_flush;
        running = true;
        while (!shutdown) {
            wait_if_no_sender();
            if (shutdown) break;
            _next_flush_time = Unix_to_now() + FLUSH_TIME;
            while (waiting(_next_flush_time))
                send();
            send();
            get_need_flush(need_flush);
            flush_need(need_flush);
        }
        closing();
        running = false;
        _condition.notify_all();
    });
    thread.start();
}

SendThread::~SendThread() {
    Lock l(_mutex);
    shutdown = true;
    _condition.notify_one();
    _condition.wait(l, [this] { return !running; });
}

void SendThread::add_sender(const Sender::SenderPtr &sender, Data &data) {
    Lock l(_mutex);
    _senders.insert(_senders.end(), sender);
    data.sender = _senders.tail();
    data.buffer = get_empty();
    _condition.notify_one();
}

void SendThread::remove_sender(Data &data) {
    Lock l(_mutex);
    _ready.push_back(data);
    data.sender->shutdown = true;
    data.sender->need_flush = false;
    ++data.sender->waiting_buffers;
    data.buffer = _buffers.end();
    _condition.notify_one();
}

void SendThread::put_buffer(SendThread::Data &data) {
    Lock l(_mutex);
    _ready.push_back(data);
    data.sender->need_flush = false;
    ++data.sender->waiting_buffers;
    data.buffer = get_empty();
    _condition.notify_one();
}

SendThread::BufferPtr SendThread::get_empty() {
    if (_empty.empty()) {
        _buffers.insert(_buffers.end());
        return _buffers.tail();
    }
    auto iter(_empty.back());
    _empty.pop_back();
    return iter;
}

void SendThread::get_readyQueue() {
    Lock l(_mutex);
    assert(_invoking.empty());
    _invoking.swap(_ready);
}

void SendThread::wait_if_no_sender() {
    Lock l(_mutex);
    _condition.wait(l, [this] {
        return _senders.size() != 0 || shutdown;
    });
}

bool SendThread::waiting(Time_difference endTime) {
    Lock l(_mutex);
    bool waiting_again = _condition.wait_until(l, endTime.to_timespec());
    assert(_invoking.empty());
    _invoking.swap(_ready);
    return waiting_again;
}

void SendThread::get_need_flush(std::vector<SenderIter> &need_flush) {
    Lock l(_mutex);
    auto iter = _senders.begin(), end = _senders.end();
    while (iter != end) {
        if (iter->need_flush)
            need_flush.push_back(iter);
        ++iter;
    }
}

void SendThread::flush_need(std::vector<SenderIter> &need_flush) {
    for (auto iter: need_flush) {
        iter->_sender->force_flush();
        get_readyQueue();
        send();
    }
    need_flush.clear();
    Lock l(_mutex);
    auto iter = _senders.begin(), end = _senders.end();
    while (iter != end) {
        if (iter->shutdown) {
            auto temp = iter++;
            if (temp->waiting_buffers == 0)
                _senders.erase(temp);
        } else {
            iter->need_flush = true;
            ++iter;
        }
    }
}

void SendThread::send() {
    for (auto [sender, buffer]: _invoking) {
        sender->_sender->send(buffer->data(), buffer->size());
        buffer->flush();
        Lock l(_mutex);
        --sender->waiting_buffers;
        if (_empty.size() >= _senders.size() * 3 / 2)
            _buffers.erase(buffer);
        else
            _empty.push_back(buffer);
    }
    _invoking.clear();
}

void SendThread::closing() {
    auto iter = _senders.end(), end = _senders.end();
    while (iter != end) {
        if (!iter->shutdown)
            iter->_sender->force_flush();
        ++iter;
    }
    Lock l(_mutex);
    for (auto [sender, buffer]: _ready)
        sender->_sender->send(buffer->data(), buffer->size());
}
