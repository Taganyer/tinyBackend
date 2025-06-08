//
// Created by taganyer on 24-10-16.
//

#include "../LinkLogServer.hpp"
#include "tinyBackend/Base/GlobalObject.hpp"
#include "tinyBackend/Net/monitors/Poller.hpp"
#include "tinyBackend/Net/functions/Interface.hpp"
#include "tinyBackend/LogSystem/linkLog/LinkLogMessage.hpp"

using namespace LogSystem;

using namespace Base;

using namespace Net;

#define CHECK(expr, error_handle) \
    if (unlikely(!(expr))) { G_ERROR << "LinkLogServer: " #expr " failed in " << __FUNCTION__; error_handle; }

#define ASSERT_WRONG_TYPE(type) \
    G_ERROR << "LinkLogServer: get wrong LinkLogMessage " << LinkLogMessage::getTypename(type) \
            << " in " << __FUNCTION__;


LinkLogServer::LinkLogServer(const Address& listen_address,
                             ServerHandlerPtr handler,
                             std::string dictionary_path,
                             PartitionRank partition_rank) :
    _pool((1 << partition_rank) * BLOCK_SIZE),
    _acceptor(listen_address),
    _receiver(Socket(), LOCAL_SOCKET_INPUT_BUFFER_SIZE, 0),
    _center(Socket(), REMOTE_SOCKET_INPUT_BUFFER_SIZE, REMOTE_SOCKET_OUTPUT_BUFFER_SIZE),
    _buffers(1 << (partition_rank - 1)),
    _handler(std::move(handler)),
    _encoder(std::move(dictionary_path)) {
    assert(_handler);
    if (!create_local_link()) {
        Global_Logger.flush();
        CurrentThread::emergency_exit("LinkLogServer: failed to create link log");
    }
}

LinkLogServer::~LinkLogServer() {
    LinkLogMessage message(LinkLogMessage::ShutDownServer);
    CHECK(safe_notify(message),
          Global_Logger.flush();
          CurrentThread::emergency_exit("LinkLogServer::close failed."))
    _thread.join();
}

void LinkLogServer::flush() const {
    _encoder.flush();
}

bool LinkLogServer::create_local_link() {
    CHECK(_acceptor.socket(), return false)
    auto pipe = Socket::create_pipe();
    CHECK(pipe.read, return false)
    CHECK(pipe.read.setNonBlock(true), return false)
    _receiver.reset_socket(std::move(pipe.read));
    _notifier = std::move(pipe.write);
    CHECK(_notifier.valid(), return false)

    start_thread();
    return true;
}

void LinkLogServer::accept_center_link(Poller& poller) {
    auto [socket, address] = _acceptor.accept_connection();
    CHECK(socket, return)
    CHECK(socket.setNonBlock(true), return)
    Event event { socket.fd() };
    event.set_read();
    event.set_error();
    CHECK(poller.add_fd(event), return)
    poller.remove_fd(_acceptor.socket().fd(), false);
    _handler->center_online(address);
    _center.reset_socket(std::move(socket));
    _center.set_running_thread();
    G_INFO << "Accepting center link: " << address.toIpPort();
}

void LinkLogServer::destroy_local_link(Poller& poller) {
    assert(_receiver.agent_valid());
    assert(_receiver.input().readable_len() == 0);
    poller.remove_fd(_receiver.fd(), false);
    poller.remove_fd(_acceptor.socket().fd(), false);
    _receiver.close();
    _notifier.close();
    _acceptor.close();
}

void LinkLogServer::destroy_center_link(Poller& poller) {
    _center.input().clear_input();
    _center.output().clear_output();
    poller.remove_fd(_center.fd(), false);
    _center.close();
    _handler->center_offline();
    if (_acceptor.socket())
        poller.add_fd(Event { _acceptor.socket().fd(), Event::Read });
}

bool LinkLogServer::safe_notify(const LinkLogMessage& message) const {
    return ops::write(_notifier.fd(), &message, sizeof(message)) == sizeof(message);
}

void LinkLogServer::safe_write(BufferIter buf_iter, const void *data, uint32 size) {
    Lock l(buf_iter->mutex);
    auto rest = buf_iter->buf.size() - buf_iter->index;
    if (rest < size)
        submit_buffer(*buf_iter, l);
    std::memcpy(buf_iter->buf.data() + buf_iter->index, data, size);
    buf_iter->index += size;
}

void LinkLogServer::safe_error(BufferIter buf_iter, const ServiceID& service,
                               const NodeID& node, LinkErrorType error) {
    Lock l(buf_iter->mutex);
    auto now = Unix_to_now();
    Error_Logger logger(service, node, now, error);
    auto rest = buf_iter->buf.size() - buf_iter->index;
    if (rest < sizeof(Error_Logger))
        submit_buffer(*buf_iter, l);
    std::memcpy(buf_iter->buf.data() + buf_iter->index, &logger, sizeof(Error_Logger));
    _handler->handling_error(service, node, now, error);
    buf_iter->index += sizeof(Error_Logger);
}

void LinkLogServer::start_thread() {
    _thread = Thread([this] {
        Poller poller;
        std::vector<Event> events;
        std::vector<BufferIter> flush_order;
        WaitQueue wait_queue;
        init_thread(poller, flush_order);

        bool thread_offline_notify = false;
        while (!thread_offline_notify) {
            bool center_can_send = accept_message(poller, events);
            thread_offline_notify = handle_local_message(wait_queue);
            handle_remote_message(wait_queue);
            force_flush(flush_order, wait_queue);
            check_timeout(wait_queue);
            send_center_message(wait_queue, poller, center_can_send);
            update_center_state(poller);
        }

        close_thread(wait_queue, poller, events);
    });
    _thread.start();
}

void LinkLogServer::init_thread(Poller& poller, std::vector<BufferIter>& flush_order) {
    Event event { _receiver.fd() };
    event.set_read();
    event.set_HangUp();
    event.set_error();
    poller.set_tid(CurrentThread::tid());
    poller.add_fd(event);
    _receiver.set_running_thread();
    event.fd = _acceptor.socket().fd();
    poller.add_fd(event);

    flush_order.reserve(_buffers.size());
    for (auto it = _buffers.begin(); it != _buffers.end(); ++it)
        flush_order.push_back(it);
}

bool LinkLogServer::accept_message(Poller& poller, std::vector<Event>& events) {
    int get = poller.get_aliveEvent(GET_ACTIVE_TIMEOUT_MS, events);
    CHECK(get >= 0, return false);
    bool center_can_send = false;
    for (auto event : events) {
        if (event.fd == _acceptor.socket().fd()) {
            accept_center_link(poller);
        } else if (event.fd == _receiver.fd()) {
            assert(!event.hasError() && !event.hasHangUp());
            assert(event.canRead());
            auto received = _receiver.receive_message();
            assert(received >= 0);
        } else {
            if (event.canRead()) {
                auto received = _receiver.receive_message();
                if (unlikely(received <= 0)) {
                    if (_center.socket_event.hasHangUp()) {
                        event.set_HangUp();
                    } else {
                        CHECK(received == 0,
                              _handler->center_error(error_mark { error_types::Read, errno });
                              destroy_center_link(poller);
                              continue)
                    }
                }
            }
            if (unlikely(event.hasError())) {
                G_ERROR << "LinkLogServer: "
                        << InetAddress::get_InetAddress(_center.fd()).toIpPort()
                        << " received an error.";
                _handler->center_error(error_mark { error_types::ErrorEvent, errno });
                event.set_HangUp();
            }
            if (unlikely(event.hasHangUp())) {
                destroy_center_link(poller);
                continue;
            }
            if (event.canWrite()) {
                center_can_send = true;
            }
        }
    }
    events.clear();
    return center_can_send;
}

bool LinkLogServer::handle_local_message(WaitQueue& wait_queue) {
    LinkLogMessage message;
    assert(_receiver.agent_valid());
    while (_receiver.input().fix_read(&message, sizeof(message))) {
        switch (message.type) {
            case LinkLogMessage::ClearBuffer:
                if (!clear_buffer(message, wait_queue.empty()))
                    wait_queue.push(message);
                break;
            case LinkLogMessage::ShutDownServer:
                return true;
            default:
                ASSERT_WRONG_TYPE(message.type)
        }
    }
    return false;
}

void LinkLogServer::handle_remote_message(WaitQueue& wait_queue) const {
    LinkLogMessage message;
    while (_center.input().fix_read(&message, sizeof(message))) {
        switch (message.type) {
            case LinkLogMessage::CentralOffline:
                wait_queue.emplace(LinkLogMessage::NodeOffline);
                return;
            default:
                ASSERT_WRONG_TYPE(message.type)
        }
    }
}

void LinkLogServer::send_center_message(WaitQueue& wait_queue, Poller& poller, bool write_notify) {
    if (_center.agent_valid() && _center.can_send() && write_notify) {
        CHECK(_center.send_message() > 0,
              _handler->center_error(error_mark { error_types::Write, errno });
              destroy_center_link(poller);)
    };

    while (!wait_queue.empty()) {
        auto& message = wait_queue.front();
        bool pop = true;
        switch (message.type) {
            case LinkLogMessage::ClearBuffer:
                pop = clear_buffer(message, true);
                break;
            case LinkLogMessage::TimeOut:
                pop = logger_timeout(message);
                break;
            case LinkLogMessage::NodeOffline:
                pop = node_offline(message, poller);
                break;
            default:
                ASSERT_WRONG_TYPE(message.type)
        }
        if (pop) wait_queue.pop();
        else break;
    }
}

void LinkLogServer::update_center_state(Poller& poller) const {
    if (_center.agent_valid()) {
        Event event { _center.fd() };
        event.set_read();
        if (_center.can_send()) {
            event.set_write();
        }
        event.set_HangUp();
        event.set_error();
        poller.update_fd(event);
    }
}

void LinkLogServer::save_logs(WaitQueue& wait_queue) {
    for (auto& [mutex, index, ref_count, buf, last_flush_time] : _buffers) {
        Lock l(mutex);
        if (!buf.data() || index == 0) continue;
        auto written = _encoder.write_to_file(buf.data(), index);
        CHECK(written > 0,);
        if (!_center.agent_valid()) continue;
        LinkLogMessage message(LinkLogMessage::ClearBuffer);
        message.get<LinkLogMessage::ClearBuffer_>().size = index;
        message.get<LinkLogMessage::ClearBuffer_>().flushed = true;
        message.get<LinkLogMessage::ClearBuffer_>().buf_ptr = new BufferPool::Buffer(std::move(buf));
        wait_queue.push(message);
    }
}

void LinkLogServer::close_thread(WaitQueue& wait_queue, Poller& poller, std::vector<Event>& events) {
    Lock l(_mutex);
    destroy_local_link(poller);
    save_logs(wait_queue);
    if (_center.agent_valid()) {
        LinkLogMessage message(LinkLogMessage::NodeOffline);
        wait_queue.push(message);
    }
    while (!wait_queue.empty() || _center.can_send()) {
        update_center_state(poller);
        send_center_message(wait_queue, poller, accept_message(poller, events));
    }

    assert(!_center.agent_valid());
}

LinkErrorType LinkLogServer::register_logger(MapIter parent_iter, Type type,
                                             const NodeID& node_id, TimeInterval timeout) {
    BufferIter buf_iter;
    Register_Logger logger;
    auto state = register_logger(parent_iter, type, node_id, timeout, buf_iter, logger);
    if (state != Success) {
        safe_error(buf_iter, parent_iter->first.serviceID(), node_id, state);
    } else {
        safe_write(buf_iter, &logger, sizeof(Register_Logger));
        _handler->register_logger(logger.service(), logger.node(),
                                  logger.parent_node(), logger.type(),
                                  logger.time(), logger.expire_time());
    }
    return state;
}

LinkErrorType LinkLogServer::register_logger(MapIter parent_iter, Type type,
                                             const NodeID& node_id, TimeInterval timeout,
                                             BufferIter& buf_iter, Register_Logger& logger) {
    Lock l(_mutex);
    buf_iter = parent_iter->second.buf_iter;
    if (parent_iter->second.subtype == Follow)
        return ExtraFollow;
    if (parent_iter->second.subtype == Invalid)
        parent_iter->second.subtype = type;
    if (parent_iter->second.subtype != type)
        return WrongType;
    if (parent_iter->second.subtype == Decision
    && parent_iter->second.sub_decision_created)
        return ExtraDecision;

    TimeInterval now = Unix_to_now();
    if (type <= Decision) {
        auto [new_iter, success] = _nodes.try_emplace(
            ID(parent_iter->first.serviceID(), node_id),
            NodeMessage(type, parent_iter));
        if (!success)
            return ConflictingNode;

        new_iter->second.create();
        _timers.emplace(now + timeout, new_iter);
    }

    buf_iter = parent_iter->second.buf_iter;
    LinkLogEncoder::register_logger(
        type, parent_iter->first.serviceID(), node_id,
        parent_iter->first.nodeID(), now,
        parent_iter->second.init_time, now + timeout,
        &logger, sizeof(Register_Logger));
    return Success;
}

LinkLogServer::BufferIter LinkLogServer::get_buffer_iter() {
    auto buf_iter = _buffers.begin(), end_iter = _buffers.end();
    uint32 min_ref = buf_iter->ref_count;
    for (auto iter = buf_iter + 1; iter < end_iter; ++iter) {
        if (iter->ref_count >= min_ref) continue;
        buf_iter = iter;
        min_ref = iter->ref_count;
    }
    return buf_iter;
}

std::pair<LinkErrorType, LinkLogServer::MapIter>
LinkLogServer::create_head_logger(const ServiceID& service, const NodeID& node,
                                  TimeInterval timeout, bool is_branch) {
    BufferIter buf_iter;
    Create_Logger logger;
    auto [state, iter] = create_head_logger(service, node, timeout, is_branch,
                                            buf_iter, logger);
    if (state != Success) {
        safe_error(buf_iter, service, node, state);
    } else {
        safe_write(buf_iter, &logger, sizeof(Create_Logger));
        _handler->create_head_logger(logger.service(), logger.node(), logger.init_time());
    }
    return std::make_pair(state, iter);
}

std::pair<LinkErrorType, LinkLogServer::MapIter>
LinkLogServer::create_head_logger(const ServiceID& service, const NodeID& node,
                                  TimeInterval timeout, bool is_branch,
                                  BufferIter& buf_iter, Create_Logger& logger) {
    Lock l(_mutex);
    Type type = is_branch ? BranchHead : Head;
    auto [new_iter, success] = _nodes.try_emplace(
        ID(service, node), NodeMessage(type, _nodes.end()));
    if (!success) {
        buf_iter = new_iter->second.buf_iter;
        return std::make_pair(ConflictingNode, new_iter);
    }

    buf_iter = get_buffer_iter();
    new_iter->second.complete_message(buf_iter);
    new_iter->second.create();
    _timers.emplace(new_iter->second.init_time + timeout, new_iter);

    LinkLogEncoder::create_logger(
        type, service, node, NodeID(),
        new_iter->second.init_time,
        TimeInterval(),
        &logger, sizeof(Create_Logger));

    return std::make_pair(Success, new_iter);
}

std::pair<LinkErrorType, LinkLogServer::MapIter>
LinkLogServer::create_logger(const ServiceID& service, const NodeID& node,
                             TimeInterval timeout) {
    BufferIter buf_iter;
    Create_Logger logger;
    auto [state, iter] = create_logger(service, node, timeout, buf_iter, logger);
    if (state != Success) {
        safe_error(buf_iter, service, node, state);
        if (state == NotRegister) {
            auto [state_, iter_] = create_head_logger(service, node, timeout, false);
            state = state_;
            iter = iter_;
        }
    } else {
        safe_write(buf_iter, &logger, sizeof(Create_Logger));
        _handler->create_logger(logger.service(), logger.node(), logger.init_time());
    }
    return std::make_pair(state, iter);
}

std::pair<LinkErrorType, LinkLogServer::MapIter>
LinkLogServer::create_logger(const ServiceID& service, const NodeID& node,
                             TimeInterval timeout,
                             BufferIter& buf_iter, Create_Logger& logger) {
    Lock l(_mutex);
    auto iter = _nodes.find(ID(service, node));
    if (iter == _nodes.end()) {
        buf_iter = get_buffer_iter();
        return std::make_pair(NotRegister, _nodes.end());
    }
    if (iter->second.created) {
        buf_iter = iter->second.buf_iter;
        return std::make_pair(AlreadyCreated, iter);
    }
    if (iter->second.type == Decision) {
        iter->second.parent_iter->second.sub_decision_created = true;
    }

    auto parent = iter->second.parent_iter;
    buf_iter = parent->second.buf_iter;
    iter->second.complete_message(buf_iter);
    iter->second.destroy();
    LinkLogEncoder::create_logger(iter->second.type,
                                  service, node, parent->first.nodeID(),
                                  iter->second.init_time,
                                  parent->second.init_time,
                                  &logger, sizeof(Create_Logger));
    iter->second.create();
    _timers.emplace(Unix_to_now() + timeout, iter);
    return std::make_pair(Success, iter);
}

void LinkLogServer::destroy_logger(MapIter iter) {
    BufferIter buf_iter;
    End_Logger logger;
    destroy_logger(iter, buf_iter, logger);
    safe_write(buf_iter, &logger, sizeof(End_Logger));
    _handler->logger_end(logger.service(), logger.node(), logger.end_time());
}

void LinkLogServer::destroy_logger(MapIter iter, BufferIter& buf_iter, End_Logger& logger) {
    Lock l(_mutex);
    assert(iter == _nodes.find(iter->first));

    buf_iter = iter->second.buf_iter;
    LinkLogEncoder::end_logger(
        iter->first.serviceID(), iter->first.nodeID(),
        iter->second.init_time, Unix_to_now(),
        &logger, sizeof(End_Logger));

    iter->second.shutdown();
    iter->second.destroy();
    _nodes.erase(iter);
}

void LinkLogServer::submit_buffer(Buffer& buf, Lock<Mutex>& lock) {
    if (buf.buf && buf.index != 0) {
        auto new_buf = new BufferPool::Buffer(_pool.get(BLOCK_SIZE));
        LinkLogMessage message(LinkLogMessage::ClearBuffer);
        message.get<LinkLogMessage::ClearBuffer_>().size = buf.index;
        message.get<LinkLogMessage::ClearBuffer_>().buf_ptr = new_buf;
        if (*new_buf) {
            std::swap(buf.buf, *new_buf);
            CHECK(safe_notify(message),
                  std::swap(buf.buf, *new_buf);
                  _encoder.write_to_file(buf.buf.data(), buf.index);
                  delete new_buf;
                  _condition.notify_one())
        } else {
            message.get<LinkLogMessage::ClearBuffer_>().flushed = true;
            auto written = _encoder.write_to_file(buf.buf.data(), buf.index);
            CHECK(written == buf.index,)
            if (_center.agent_valid()) {
                std::swap(buf.buf, *new_buf);
                CHECK(safe_notify(message),
                      std::swap(buf.buf, *new_buf);
                      delete new_buf)
            } else {
                delete new_buf;
            }
        }
    }
    buf.index = 0;
    buf.last_flush_time = Unix_to_now();
    if (buf.buf) return;
    buf.buf = _pool.get(BLOCK_SIZE);
    while (!buf.buf) {
        _condition.wait(lock);
        buf.buf = _pool.get(BLOCK_SIZE);
    }
}

bool LinkLogServer::clear_buffer(LinkLogMessage& message, bool can_send) {
    auto& [size, flushed, begin, buf_ptr] = message.get<LinkLogMessage::ClearBuffer_>();
    auto buf = (BufferPool::Buffer *) buf_ptr;
    if (!flushed) {
        _encoder.write_to_file(buf->data(), size);
        flushed = true;
    }
    if (!can_send) return false;
    if (_center.agent_valid()) {
        auto written = _center.output().write(buf->data() + begin, size - begin);
        if (written != size - begin) {
            begin += written;
            return false;
        }
    }
    delete buf;
    _condition.notify_one();
    return true;
}

bool LinkLogServer::logger_timeout(const LinkLogMessage& message) const {
    if (!_center.agent_valid()) return true;
    auto written = _center.output().fix_write(message.arr, sizeof(Error_Logger));
    return written == sizeof(Error_Logger);
}

bool LinkLogServer::node_offline(LinkLogMessage& message, Poller& poller) {
    if (!_center.agent_valid()) return true;

    if (message.get<LinkLogMessage::NodeOffline_>().sent) {
        if (!_center.can_send()) {
            destroy_center_link(poller);
            return true;
        }
    } else {
        Node_Offline node_offline(Unix_to_now());
        auto written = _center.output().fix_write(&node_offline, sizeof(Node_Offline));
        if (written == sizeof(Node_Offline))
            message.get<LinkLogMessage::NodeOffline_>().sent = true;
    }
    return false;
}

void LinkLogServer::force_flush(std::vector<BufferIter>& flush_order, WaitQueue& wait_queue) {
    std::sort(flush_order.begin(), flush_order.end(),
              [] (BufferIter a, BufferIter b) {
                  return a->last_flush_time < b->last_flush_time;
              });
    TimeInterval threshold = Unix_to_now() - BUFFER_FLUSH_TIME;

    for (auto it : flush_order) {
        if (it->last_flush_time <= threshold) {
            Lock l(it->mutex);
            if (!it->buf || it->index == 0) continue;
            _encoder.write_to_file(it->buf.data(), it->index);
            if (_center.agent_valid()) {
                auto new_buf = new BufferPool::Buffer(std::move(it->buf));
                LinkLogMessage message(LinkLogMessage::ClearBuffer);
                message.get<LinkLogMessage::ClearBuffer_>().size = it->index;
                message.get<LinkLogMessage::ClearBuffer_>().flushed = true;
                message.get<LinkLogMessage::ClearBuffer_>().buf_ptr = new_buf;
                wait_queue.push(message);
            }
            it->index = 0;
            it->last_flush_time = Unix_to_now();
        }
    }

    _encoder.flush();
}

void LinkLogServer::check_timeout(WaitQueue& wait_queue) {
    Lock l(_mutex);
    auto now = Unix_to_now();
    while (!_timers.empty() && _timers.top().expire_time <= now) {
        auto& timer = _timers.top();
        if (timer.check_timeout()) {
            auto iter = timer.iter;
            if (iter->second.created) {
                if (_center.agent_valid()) {
                    LinkLogMessage msg(LinkLogMessage::TimeOut);
                    auto& time_out = msg.get<LinkLogMessage::TimeOut_>().time_out;
                    time_out.ot = ErrorLogger;
                    time_out.service() = iter->first.serviceID();
                    time_out.node() = iter->first.nodeID();
                    time_out.time() = timer.expire_time;
                    time_out.error_type() = EndTimeOut;
                    wait_queue.push(msg);
                }
                _handler->handling_error(iter->first.serviceID(), iter->first.nodeID(),
                                         timer.expire_time, EndTimeOut);
            } else {
                if (iter->second.type != Decision) {
                    if (_center.agent_valid()) {
                        LinkLogMessage msg(LinkLogMessage::TimeOut);
                        auto& time_out = msg.get<LinkLogMessage::TimeOut_>().time_out;
                        time_out.ot = ErrorLogger;
                        time_out.service() = iter->first.serviceID();
                        time_out.node() = iter->first.nodeID();
                        time_out.time() = timer.expire_time;
                        time_out.error_type() = CreateTimeOut;
                        wait_queue.push(msg);
                    }
                    _handler->handling_error(iter->first.serviceID(), iter->first.nodeID(),
                                             timer.expire_time, CreateTimeOut);
                }
                iter->second.destroy();
                _nodes.erase(iter);
            }
        }
        _timers.pop();
    }
}
