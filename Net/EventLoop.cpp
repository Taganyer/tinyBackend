//
// Created by taganyer on 24-2-29.
//

#include "EventLoop.hpp"

using namespace Base;

using namespace Net;

namespace {

    thread_local bool this_thread_have_object = false;

}


EventLoop::EventLoop() {
    if (!this_thread_have_object) {
        this_thread_have_object = true;
    } else {
        G_FATAL << "Define an EventLoop multiple times within a thread";
        abort();
    }
}

EventLoop::~EventLoop() {
    assert_in_thread();
    shutdown();
    G_TRACE << "EventLoop " << _tid << "has been destroyed";
}

void EventLoop::loop() {
    assert_in_thread();
    while (!quit) {
        if (_distributor)
            _distributor();
        loop_begin();
        for (auto const &event: _queue)
            if (event) event();
        G_TRACE << "invoke " << _queue.size() << "events";
        loop_end();
    }
    G_TRACE << "end EventLoop::loop() " << _tid;
}

void EventLoop::shutdown() {
    quit = true;
}

void EventLoop::set_distributor(EventLoop::Event event) {
    assert_in_thread();
    _distributor = std::move(event);
}

void EventLoop::put_event(EventLoop::Event event) {
    Base::Lock l(_mutex);
    _waiting.push_back(std::move(event));
    G_TRACE << "put 1 " << "event in EventLoop "
            << _tid << " at " << Base::thread_name();
    _condition.notify_one();
}

void EventLoop::assert_in_thread() const {
    if (!object_in_thread()) {
        G_ERROR << "EventLoop " << _tid << " assert in " << tid();
    }
}

void EventLoop::loop_begin() {
    Lock l(_mutex);
    _condition.wait(l, [this] { return !_waiting.empty(); });
    _queue.swap(_waiting);
    run = true;
}

void EventLoop::loop_end() {
    Lock l(_mutex);
    _queue.clear();
    run = false;
}
