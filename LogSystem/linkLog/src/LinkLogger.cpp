//
// Created by taganyer on 24-10-16.
//

#include "../LinkLogger.hpp"
#include "LogSystem/linkLog/LinkLogErrors.hpp"

using namespace LogSystem;

using namespace Base;


LinkLogger::LinkLogger(LogRank rank, const ServiceID &service, const NodeID &node,
                       bool is_branch, TimeDifference end_timeout,
                       LinkLogServer &server) :
    _rank(rank), _server(&server) {
    auto [state, iter] = _server->create_head_logger(service, node, end_timeout, is_branch);
    if (state != Success) {
        _server = nullptr;
        throw LinkLogCreateError(service, node, state);
    }
    _iter = iter;
}

LinkLogger::LinkLogger(LogRank rank, const ID &head_id,
                       bool is_branch, TimeDifference end_timeout,
                       LinkLogServer &server) :
    LinkLogger(rank, head_id.serviceID(), head_id.nodeID(), is_branch, end_timeout, server) {}

LinkLogger::LinkLogger(LogRank rank, const ServiceID &service,
                       const NodeID &node, TimeDifference end_timeout,
                       LinkLogServer &server) :
    _rank(rank), _server(&server) {
    auto [state, iter] = _server->create_logger(service, node, end_timeout);
    if (state != Success) {
        _server = nullptr;
        throw LinkLogCreateError(service, node, state);
    }
    _iter = iter;
}

LinkLogger::LinkLogger(LogRank rank, const ID &complete_id,
                       TimeDifference end_timeout, LinkLogServer &server):
    LinkLogger(rank, complete_id.serviceID(), complete_id.nodeID(), end_timeout, server) {}

LinkLogger::LinkLogger(LogRank rank, const LinkLogger &parent, const NodeID &node,
                       TimeDifference end_timeout) :
    _rank(rank), _server(parent._server) {
    auto [state, iter] = _server->create_logger(parent._iter->first.serviceID(),
                                                node, end_timeout);
    if (state != Success) {
        _server = nullptr;
        throw LinkLogCreateError(parent._iter->first.serviceID(), node, state);
    }
    _iter = iter;
}

LinkLogger::LinkLogger(LinkLogger &&other) noexcept :
    _rank(other._rank), _iter(other._iter), _server(other._server) {
    other._server = nullptr;
}

LinkLogger::~LinkLogger() {
    close();
}

void LinkLogger::push(LogRank rank, const void* data, uint16 size) const {
    if (rank < _rank) return;
    auto buf_iter = _iter->second.buf_iter;
    Lock l(buf_iter->mutex);
    auto rest = buf_iter->buf.size() - buf_iter->index;
    if (!LinkLogEncoder::can_write_log(size, rest))
        _server->submit_buffer(*buf_iter, l);
    buf_iter->index += LinkLogEncoder::write_log(
        _iter->first.serviceID(), _iter->second.init_time,
        _iter->first.nodeID(), Unix_to_now(), rank,
        data, size,
        buf_iter->buf.data() + buf_iter->index);
}

void LinkLogger::register_child_node(Type type, const NodeID &node_id,
                                     TimeDifference create_timeout) const {
    if (type == BranchHead || type == Head) {
        throw LinkLogRegisterError(_iter->first.serviceID(), node_id, WrongType);
    }
    auto state = _server->register_logger(_iter, type, node_id, create_timeout);
    if (state != Success) {
        throw LinkLogRegisterError(_iter->first.serviceID(), node_id, state);
    }
}

void LinkLogger::fork(const NodeID &child_node, TimeDifference create_timeout) const {
    register_child_node(Fork, child_node, create_timeout);
}

void LinkLogger::follow(const NodeID &child_node, TimeDifference create_timeout) const {
    register_child_node(Follow, child_node, create_timeout);
}

void LinkLogger::decision(const NodeID &child_node, TimeDifference create_timeout) const {
    register_child_node(Decision, child_node, create_timeout);
}

void LinkLogger::rpc_fork(const NodeID &child_node, TimeDifference create_timeout) const {
    register_child_node(RpcFork, child_node, create_timeout);
}

void LinkLogger::rpc_follow(const NodeID &child_node, TimeDifference create_timeout) const {
    register_child_node(RpcFollow, child_node, create_timeout);
}

void LinkLogger::rpc_decision(const NodeID &child_node, TimeDifference create_timeout) const {
    register_child_node(RpcDecision, child_node, create_timeout);
}

void LinkLogger::close() {
    if (_server) {
        _server->destroy_logger(_iter);
        _iter = Iter();
        _server = nullptr;
    }
}

LinkLogStream LinkLogger::stream(LogRank rank) {
    return { *this, rank };
}
