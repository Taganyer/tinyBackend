//
// Created by taganyer on 24-2-29.
//

#include "../EventLoop.hpp"

using namespace Base;

namespace {

    thread_local bool this_thread_have_object = false;

}


EventLoop::EventLoop() {
    if (!this_thread_have_object) {
        this_thread_have_object = true;
        G_TRACE << "EventLoop create in " << thread_name();
    } else {
        G_FATAL << "Define an EventLoop multiple times within " << thread_name();
        abort();
    }
}

EventLoop::~EventLoop() {
    assert_in_thread();
    shutdown();
    G_TRACE << "EventLoop in " << thread_name() << " has been destroyed.";
    this_thread_have_object = false;
}

void EventLoop::loop() {
    assert_in_thread();
    _run = true;
    while (!_quit) {
        if (_distributor)
            _distributor();
        loop_begin();
        if (_queue.size() > 0) {
            for (const auto &event : _queue)
                if (event) event();
            G_TRACE << "EventLoop " << _tid << " invoke " << _queue.size() << " events";
        }
        loop_end();
    }
    _run = false;
    G_TRACE << "end EventLoop::loop() " << _tid;
}

void EventLoop::shutdown() {
    _quit = true;
    _condition.notify_one();
}

void EventLoop::set_distributor(Event event) {
    assert_in_thread();
    _distributor = std::move(event);
}

void EventLoop::put_event(Event event) {
    Lock l(_mutex);
    _waiting.push_back(std::move(event));
    G_TRACE << "put a event in EventLoop "
        << _tid << " at " << thread_name();
    _condition.notify_one();
}

void EventLoop::weak_up() {
    Lock l(_mutex);
    _weak = true;
    _condition.notify_one();
}

void EventLoop::assert_in_thread() const {
    if (!object_in_thread()) {
        G_ERROR << "EventLoop " << _tid << " assert in " << tid();
    }
}

void EventLoop::loop_begin() {
    Lock l(_mutex);
    _condition.wait(l, [this] { return !_waiting.empty() || _weak; });
    _queue.swap(_waiting);
    _weak = false;
}

void EventLoop::loop_end() {
    Lock l(_mutex);
    _queue.clear();
}
