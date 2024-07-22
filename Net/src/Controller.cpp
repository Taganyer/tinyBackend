//
// Created by taganyer on 3/24/24.
//

#include "../Controller.hpp"
#include "Net/Reactor.hpp"
#include "Net/monitors/Monitor.hpp"
#include "Net/functions/Interface.hpp"

using namespace Net;

Controller::Controller(const Shared &ptr, Reactor* reactor) :
    _weak(ptr), _reactor(reactor) {}

uint32 Controller::send(const void* target, uint32 size) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return -1;
    int64 writen = 0;
    if (in_loop_thread() && ptr->_output.empty()) {
        writen = ops::write(ptr->fd(), target, size);
        if (writen < 0) {
            ptr->handle_error({ error_types::Write, errno }, nullptr);
            return -1;
        } else {
            G_TRACE << "write " << writen << " byte in " << ptr->fd();
            if (ptr->_writeFun) ptr->_writeFun(ptr->_output, *ptr->_socket);
        }
    }
    if (writen < size)
        writen += ptr->_output.write((const char *) target + writen, size - writen);
    return writen;
}

void Controller::reset_readCallback(ReadCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return;
    send_to_loop([weak = _weak, e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data && data->valid())
            data->set_readCallback(std::move(e));
    });
}

void Controller::reset_writeCallback(WriteCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return;
    send_to_loop([weak = _weak, e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data && data->valid())
            data->set_writeCallback(std::move(e));
    });
}

void Controller::reset_errorCallback(ErrorCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return;
    send_to_loop([weak = _weak, e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data) data->set_errorCallback(std::move(e));
    });
}

void Controller::reset_closeCallback(CloseCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return;
    send_to_loop([weak = _weak, e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data) data->set_closeCallback(std::move(e));
    });
}

void Controller::update_event(Event event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return;
    if (in_loop_thread()) {
        auto iter = _reactor->_map.find(ptr->fd());
        if (iter == _reactor->_map.end()) return;
        auto e = iter->second.second;
        e->second = event;
        _reactor->_monitor->update_fd(event);
    } else {
        send_to_loop([weak = _weak, reactor = _reactor, event] {
            Shared data = weak.lock();
            if (data && data->valid()) {
                auto iter = reactor->_map.find(data->fd());
                if (iter == reactor->_map.end()) return;
                auto e = iter->second.second;
                e->second = event;
                reactor->_monitor->update_fd(event);
            }
        });
    }
}

void Controller::close_fd() {
    Shared ptr = _weak.lock();
    if (!ptr) return;
    ptr->close_fd();
}

void Controller::send_to_loop(EventFun fun) {
    _reactor->_loop->put_event(std::move(fun));
}

bool Controller::in_loop_thread() const {
    return _reactor->_loop->object_in_thread();
}
