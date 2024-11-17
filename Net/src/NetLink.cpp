//
// Created by taganyer on 24-3-5.
//

#include "../NetLink.hpp"
#include "Net/error/errors.hpp"
#include "LogSystem/SystemLog.hpp"
#include "Net/Controller.hpp"
#include "Net/monitors/Monitor.hpp"
#include "Net/functions/Interface.hpp"

using namespace Net;

using namespace Base;


void NetLink::handle_read(const Controller &controller) {
    if (!valid()) {
        handle_error({ error_types::UnexpectedShutdown, 0 }, controller);
        return;
    }
    if (!_agent.can_receive()) {
        G_TRACE << "read buffer is full cannot read.";
    } else {
        bool success = _agent.receive_message();
        if (!success) {
            if (_agent.has_hang_up()) {
                G_TRACE << "NetLink " << fd() << " read to end.";
                controller.current_event().set_HangUp();
            } else {
                G_ERROR << fd() << " handle_read " << ops::get_read_error(errno);
                handle_error({ error_types::Read, errno }, controller);
            }
            return;
        }
    }
    if (_readFun) _readFun(controller);
}

void NetLink::handle_write(const Controller &controller) {
    if (!valid()) {
        handle_error({ error_types::UnexpectedShutdown, 0 }, controller);
        return;
    }
    if (!_agent.can_send()) {
        G_TRACE << "write buffer is empty cannot sent.";
    } else {
        bool success = _agent.send_message();
        if (!success) {
            G_ERROR << fd() << " handle_write " << ops::get_write_error(errno);
            handle_error({ error_types::Write, errno }, controller);
        }
    }
    if (_writeFun) _writeFun(controller);
}

void NetLink::handle_error(error_mark mark, const Controller &controller) const {
    controller.current_event().set_NoEvent();
    if (valid())
        G_ERROR << "error occur in NetLink " << fd();
    if (!_errorFun || _errorFun(mark, controller))
        controller.current_event().set_HangUp();
}

void NetLink::handle_close(const Controller &controller) {
    if (_closeFun) _closeFun(controller);
    close_fd();
}

bool NetLink::handle_timeout(const Controller &controller) const {
    G_WARN << "NetLink " << fd() << " timeout.";
    return !_errorFun || _errorFun({ error_types::TimeoutEvent, 0 }, controller);
}

NetLink::LinkPtr NetLink::create_NetLinkPtr(Socket &&socket,
                                            uint32 inputBufferSize,
                                            uint32 outputBufferSize) {
    return LinkPtr(new NetLink(std::move(socket), inputBufferSize, outputBufferSize));
}

NetLink::NetLink(Socket &&socket, uint32 inputBufferSize, uint32 outputBufferSize) :
    _agent(std::move(socket), inputBufferSize, outputBufferSize) {}

NetLink::~NetLink() {
    close_fd();
}

void NetLink::close_fd() {
    if (valid()) {
        G_INFO << "NetLink " << fd() << " closed.";
        _agent.close();
    }
}
