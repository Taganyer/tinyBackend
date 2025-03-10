//
// Created by taganyer on 25-3-3.
//

#include "../Channel.hpp"
#include "Net/error/errors.hpp"
#include "LogSystem/SystemLog.hpp"

using namespace Base;

using namespace Net;


void Channel::invoke_event(MessageAgent &agent) const {
    agent.set_running_thread();
    while (!agent.socket_event.is_NoEvent()) {
        if (agent.socket_event.hasError()) {
            handle_error(agent);
        }
        if (agent.socket_event.hasHangUp()) {
            handle_close(agent);
        }
        if (agent.socket_event.canRead()) {
            handle_read(agent);
        }
        if (agent.socket_event.canWrite()) {
            handle_write(agent);
        }
    }
}

void Channel::handle_read(MessageAgent &agent) const {
    agent.socket_event.unset_read();
    if (!agent.agent_valid()) {
        agent.error = { error_types::UnexpectedShutdown, 0 };
        handle_error(agent);
        return;
    }
    if (!agent.can_receive()) {
        G_TRACE << "MessageAgent " << agent.fd() << " input buffer is full cannot read.";
    } else {
        auto received = agent.receive_message();
        if (agent.socket_event.hasHangUp()) {
            G_TRACE << "MessageAgent " << agent.fd() << " read to end.";
            return;
        }
        if (received < 0) {
            G_ERROR << "MessageAgent" << agent.fd() << " handle_read " << ops::get_read_error(errno);
            agent.error = { error_types::Read, 0 };
            handle_error(agent);
            return;
        }
    }
    if (_readFun) _readFun(agent);
}

void Channel::handle_write(MessageAgent &agent) const {
    agent.socket_event.unset_write();
    if (!agent.agent_valid()) {
        agent.error = { error_types::UnexpectedShutdown, 0 };
        handle_error(agent);
        return;
    }
    if (!agent.can_send()) {
        G_TRACE << "MessageAgent " << agent.fd() << " output buffer is empty cannot sent.";
    } else {
        auto sent = agent.send_message();
        if (sent < 0) {
            G_ERROR << "MessageAgent " << agent.fd() << " handle_write " << ops::get_write_error(errno);
            agent.error = { error_types::Write, errno };
            handle_error(agent);
        }
    }
    if (_writeFun) _writeFun(agent);
}

void Channel::handle_error(MessageAgent &agent) const {
    agent.socket_event.unset_error();
    if (agent.agent_valid()) {
        G_ERROR << "error occur in Channel " << agent.fd();
    }
    if (!_errorFun) {
        agent.socket_event.set_HangUp();
    } else {
        _errorFun(agent);
    }
    agent.error = {};
}

void Channel::handle_close(MessageAgent &agent) const {
    if (_closeFun) _closeFun(agent);
    agent.close();
    agent.socket_event.set_NoEvent();
}
