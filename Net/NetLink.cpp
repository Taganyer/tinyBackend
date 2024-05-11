//
// Created by taganyer on 24-3-5.
//

#include "NetLink.hpp"
#include "error/errors.hpp"
#include "monitors/Monitor.hpp"
#include "functions/Interface.hpp"
#include "../Base/Loop/EventLoop.hpp"

using namespace Net;

using namespace Base;


void NetLink::handle_read(Event *event) {
    if (event) event->unset_read();
    if (!FD->valid()) {
        handle_error({error_types::UnexpectedShutdown, 0}, event);
        return;
    }
    struct iovec vec[2];
    vec[0].iov_base = _input.write_data();
    vec[0].iov_len = _input.continuously_writable();
    vec[1].iov_base = _input.data_begin();
    vec[1].iov_len = _input.writable_len() - _input.continuously_writable();
    auto size = ops::readv(FD->fd(), vec, 2);
    if (size == 0) {
        G_TRACE << "fd " << FD->fd() << " read to end.";
        handle_close(event);
    } else if (size < 0) {
        G_ERROR << FD->fd() << " handle_read " << ops::get_read_error(errno);
        handle_error({error_types::Read, errno}, event);
    } else {
        _input.write_advance(size);
        G_TRACE << FD->fd() << " read " << size << " bytes in readBuf";
        if (_readFun) _readFun(_input, *FD);
    }
}

void NetLink::handle_write(Event *event) {
    if (event) event->unset_write();
    if (!FD->valid()) {
        handle_error({error_types::UnexpectedShutdown, 0}, event);
        return;
    }
    struct iovec vec[2];
    vec[0].iov_base = _output.read_data();
    vec[0].iov_len = _input.continuously_readable();
    vec[1].iov_base = _input.data_begin();
    vec[1].iov_len = _input.readable_len() - _input.continuously_readable();
    auto size = ops::writev(FD->fd(), vec, 2);
    if (size < 0) {
        G_ERROR << FD->fd() << " handle_write " << ops::get_write_error(errno);
        handle_error({error_types::Write, errno}, event);
        return;
    } else {
        _output.read_advance(size);
        G_TRACE << FD->fd() << " write " << size << " bytes in writeBuf";
        if (_writeFun) _writeFun(_output, *FD);
    }
}

void NetLink::handle_error(error_mark mark, Event *event) {
    if (event) event->unset_error();
    if (FD->valid())
        G_ERROR << "error occur in Link " << FD->fd();
    if (_errorFun && _errorFun(mark, *FD))
        handle_close(event);
}

void NetLink::handle_close(Event *event) {
    if (event) event->set_NoEvent();
    G_TRACE << "Link " << FD->fd() << " close";
    if (_closeFun) _closeFun(*FD);
}

bool NetLink::handle_timeout() {
    G_WARN << "Link " << FD->fd() << " timeout";
    if (_errorFun({error_types::TimeoutEvent, 0}, *FD)) {
        G_TRACE << "Link " << FD->fd() << " close";
        if (_closeFun) _closeFun(*FD);
        return true;
    }
    return false;
}

NetLink::LinkPtr NetLink::create_NetLinkPtr(NetLink::FdPtr &&Fd) {
    return std::shared_ptr<NetLink>(new NetLink(std::move(Fd)));
}

NetLink::NetLink(FdPtr &&Fd) : FD(std::move(Fd)) {}

NetLink::~NetLink() {
    close_fd();
}

void NetLink::close_fd() {
    if (FD->valid()) {
        G_INFO << "NetLink fd " << FD->fd() << " closed";
        FD = std::make_unique<FileDescriptor>(-1);
    }
}
