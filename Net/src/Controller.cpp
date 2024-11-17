//
// Created by taganyer on 3/24/24.
//


#include "../Controller.hpp"
#include "Net/EventLoop.hpp"
#include "Net/reactor/Reactor.hpp"

using namespace Net;

int64 Controller::send(const void* target, uint32 size, bool invoke_write_fun) const {
    assert(in_loop_thread());

    int64 written = _link._agent.direct_send(target, size);
    if (written < 0) {
        _link.handle_error({ error_types::Write, errno }, *this);
    } else if (invoke_write_fun && _link._writeFun) {
        _link._writeFun(*this);
    }
    return written;
}

uint32 Controller::indirect_send(const void* target, uint32 size,
                                 bool fixed, bool invoke_write_fun) const {
    assert(in_loop_thread());

    uint32 written = _link._agent.indirect_send(target, size, fixed);
    if (invoke_write_fun && _link._writeFun)
        _link._writeFun(*this);
    return written;
}

void Controller::reset_readCallback(ReadCallback event) const {
    send_to_loop([weak = _link.weak_from_this(), e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data && data->valid())
            data->set_readCallback(std::move(e));
    });
}

void Controller::reset_writeCallback(WriteCallback event) const {
    send_to_loop([weak = _link.weak_from_this(), e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data && data->valid())
            data->set_writeCallback(std::move(e));
    });
}

void Controller::reset_errorCallback(ErrorCallback event) const {
    send_to_loop([weak = _link.weak_from_this(), e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data) data->set_errorCallback(std::move(e));
    });
}

void Controller::reset_closeCallback(CloseCallback event) const {
    send_to_loop([weak = _link.weak_from_this(), e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data) data->set_closeCallback(std::move(e));
    });
}

void Controller::update_event(Event event) const {
    assert(in_loop_thread());
    _reactor.update_link(event);
}

void Controller::send_to_loop(EventFun fun) const {
    assert(in_loop_thread());
    _reactor._loop->put_event(std::move(fun));
}

Event& Controller::current_event() const {
    assert(in_loop_thread());
    return _event;
}

Event Controller::registered_event() const {
    assert(in_loop_thread());
    Event event(_event);
    event.event = _registered_event;
    return event;
}

bool Controller::in_loop_thread() const {
    return _reactor.in_reactor_thread();
}

Controller::Controller(NetLink &link, Event &event, Reactor &reactor, int registered_event) :
    _link(link), _event(event), _reactor(reactor), _registered_event(registered_event) {}
