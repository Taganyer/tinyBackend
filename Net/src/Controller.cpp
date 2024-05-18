//
// Created by taganyer on 3/24/24.
//

#include "../Reactor.hpp"
#include "Net/Controller.hpp"
#include "Net/monitors/Monitor.hpp"
#include "Net/functions/Interface.hpp"

using namespace Net;

Controller::Controller(const Controller::Shared &ptr, Reactor *reactor) :
        _weak(ptr), _reactor(reactor) {}

uint32 Controller::send(const void *target, uint32 size) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return -1;
    int64 writen = 0;
    if (in_loop_thread() && ptr->_output.empty()) {
        writen = ops::write(ptr->fd(), target, size);
        if (writen < 0) {
            ptr->handle_error({error_types::Write, errno}, nullptr);
            return -1;
        } else {
            G_TRACE << "write " << writen << " byte in " << ptr->fd();
            if (ptr->_writeFun) ptr->_writeFun(ptr->_output, *ptr->FD);
        }
    }
    if (writen < size)
        writen += ptr->_output.write((const char *) target + writen, size - writen);
    return writen;
}

bool Controller::reset_readCallback(Controller::ReadCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    send_to_loop([weak = _weak, e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data)
            data->set_readCallback(std::move(e));
    });
    return true;
}

bool Controller::reset_writeCallback(Controller::WriteCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    send_to_loop([weak = _weak, e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data)
            data->set_writeCallback(std::move(e));
    });
    return true;
}

bool Controller::reset_errorCallback(Controller::ErrorCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    send_to_loop([weak = _weak, e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data)
            data->set_errorCallback(std::move(e));
    });
    return true;
}

bool Controller::reset_closeCallback(Controller::CloseCallback event) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    send_to_loop([weak = _weak, e = std::move(event)]() mutable {
        Shared data = weak.lock();
        if (data)
            data->set_closeCallback(std::move(e));
    });
    return true;
}

bool Controller::set_read(bool turn_on) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    send_to_loop([this, turn_on] {
        Shared data = _weak.lock();
        if (data && data->valid()) {
            auto iter = _reactor->_map.find(data->fd());
            if (iter == _reactor->_map.end()) return;
            auto e = iter->second.second;
            if (turn_on && !e->second.canRead()) {
                e->second.set_read();
                _reactor->_monitor->update_fd(e->second);
            } else if (!turn_on && e->second.canRead()) {
                e->second.unset_read();
                _reactor->_monitor->update_fd(e->second);
            }
        }
    });
    return true;
}

bool Controller::set_write(bool turn_on) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    send_to_loop([this, turn_on] {
        Shared data = _weak.lock();
        if (data && data->valid()) {
            auto iter = _reactor->_map.find(data->fd());
            if (iter == _reactor->_map.end()) return;
            auto e = iter->second.second;
            if (turn_on && !e->second.canWrite()) {
                e->second.set_write();
                _reactor->_monitor->update_fd(e->second);
            } else if (!turn_on && e->second.canWrite()) {
                e->second.unset_write();
                _reactor->_monitor->update_fd(e->second);
            }
        }
    });
    return true;
}

bool Controller::wake_readCallback(bool after) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (in_loop_thread() && !after) {
        ptr->_readFun(ptr->_input, *ptr->FD);
    } else {
        send_to_loop([weak = _weak] {
            auto data = weak.lock();
            if (data && data->valid())
                data->_readFun(data->_input, *data->FD);
        });
    }
    return true;
}

bool Controller::wake_writeCallback(bool after) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (in_loop_thread() && !after) {
        ptr->_writeFun(ptr->_output, *ptr->FD);
    } else {
        send_to_loop([weak = _weak] {
            auto data = weak.lock();
            if (data && data->valid())
                data->_writeFun(data->_output, *data->FD);
        });
    }
    return true;
}

bool Controller::wake_error(error_mark mark, bool after) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (in_loop_thread() && !after) {
        ptr->handle_error(mark, nullptr);
    } else {
        send_to_loop([weak = _weak, mark] {
            auto data = weak.lock();
            if (data && data->valid())
                data->handle_error(mark, nullptr);
        });
    }
    return true;
}

bool Controller::wake_close(bool after) {
    Shared ptr = _weak.lock();
    if (!ptr || !ptr->valid()) return false;
    if (in_loop_thread() && !after) {
        ptr->handle_close(nullptr);
    } else {
        send_to_loop([weak = _weak] {
            auto data = weak.lock();
            if (data && data->valid())
                data->handle_close(nullptr);
        });
    }
    return true;
}

void Controller::close_fd() {
    Shared ptr = _weak.lock();
    if (!ptr) return;
    ptr->close_fd();
}

void Controller::send_to_loop(Controller::EventFun fun) {
    _reactor->_loop->put_event(std::move(fun));
}

bool Controller::in_loop_thread() const {
    return _reactor->_loop->object_in_thread();
}
