//
// Created by taganyer on 3/24/24.
//

#include "../Controller.hpp"
#include "Net/Reactor.hpp"
#include "Net/EventLoop.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Net/BalancedReactor.hpp"
#include "Net/monitors/Monitor.hpp"
#include "Net/functions/Interface.hpp"

using namespace Net;

Controller::Controller(const Shared &ptr, Reactor* reactor) :
    _weak(ptr), _reactor(reactor) {}

Controller::Controller(const Shared &ptr, BalancedReactor* reactor) :
    _weak(ptr), _balanced_reactor(reactor) {}

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
            if (ptr->_writeFun) ptr->_writeFun(*ptr->_socket);
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
        if (_reactor) _reactor->update_link(event);
        else _balanced_reactor->update_link(event);
    } else {
        send_to_loop([weak = _weak, reactor = _reactor,
            balanced_reactor = _balanced_reactor, event] {
            Shared data = weak.lock();
            if (data && data->valid()) {
                if (reactor) reactor->update_link(event);
                else balanced_reactor->update_link(event);
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
    if (_reactor) _reactor->_loop->put_event(std::move(fun));
    else _balanced_reactor->_loop->put_event(std::move(fun));
}

bool Controller::in_loop_thread() const {
    if (_reactor) return _reactor->in_reactor_thread();
    return _balanced_reactor->in_reactor_thread();
}
