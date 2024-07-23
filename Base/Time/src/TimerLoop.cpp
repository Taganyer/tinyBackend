//
// Created by taganyer on 24-5-11.
//

#include "../TimerLoop.hpp"
#include "Base/Thread.hpp"

using namespace Base;

TimerLoop::~TimerLoop() {
    shutdown();
}

void TimerLoop::loop() {
    Lock l(_mutex);
    if (_run) return;
    _quit = false;
    Thread thread(string("TimerLoop thread"), [this] {
        _run = true;
        while (!_quit)
            invoke_event();
        _run = false;
    });
    thread.start();
}

void TimerLoop::shutdown() {
    Lock l(_mutex);
    _quit = true;
    _con.notify_all();
    _con.wait(l, [this] { return !_run; });
    _list = {};
}

TimerLoop::EventID TimerLoop::put_event(TimerLoop::Event fun, Time_difference interval) {
    assert(interval > 0);
    Lock l(_mutex);
    if (!looping()) return { this, nullptr };
    Node node(std::move(fun), interval);
    _list.push(node);
    _con.notify_one();
    return { this, node._ptr };
}

TimerLoop::Node TimerLoop::get_event() {
    Lock l(_mutex);
    while (true) {
        _con.wait(l, [this] {
            return _quit || !_list.empty();
        });
        if (_quit) break;
        Node node = _list.top();
        _list.pop();
        if (node._ptr->first == -1) continue;
        return node;
    }
    return {};
}

void TimerLoop::invoke_event() {
    Node node = get_event();
    sleep(node._now - Unix_to_now());
    if (node.invoke()) {
        Lock l(_mutex);
        if (node.update())
            _list.push(node);
    }
}

TimerLoop::Node::Node(TimerLoop::Event fun, Time_difference interval) :
    _ptr(std::make_shared<Pair>(interval, std::move(fun))) {}

bool TimerLoop::Node::update() {
    if (_ptr->first == -1) return false;
    _now = _now + _ptr->first;
    return true;
}

bool TimerLoop::Node::invoke() {
    if (!_ptr || _ptr->first == -1 || !_ptr->second)
        return false;
    _ptr->second();
    return _ptr->first != -1;
}

void TimerLoop::EventID::reset(Time_difference interval) {
    assert(interval > 0);
    Lock l(_loop->_mutex);
    auto ptr = _ptr.lock();
    if (ptr) ptr->first = interval;
}

void TimerLoop::EventID::cancel() {
    Lock l(_loop->_mutex);
    auto ptr = _ptr.lock();
    if (ptr) ptr->first = -1;
}

bool TimerLoop::EventID::expired() const {
    return _ptr.expired();
}
