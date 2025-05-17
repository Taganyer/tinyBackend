//
// Created by taganyer on 24-11-1.
//

#include "../LinkLogCenter.hpp"
#include "Net/InetAddress.hpp"
#include "Base/SystemLog.hpp"
#include "Net/TcpMessageAgent.hpp"
#include "Net/error/error_mark.hpp"

using namespace Base;

using namespace Net;

using namespace LogSystem;

#define CHECK(expr, error_handle) \
    if (unlikely(!(expr))) { G_ERROR << "LinkLogCenter: " #expr " failed in " << __FUNCTION__; error_handle; }

#define ASSERT_WRONG_TYPE(type) \
    G_ERROR << "LinkLogCenter: get wrong LinkLogMessage " << getOperationTypeName(type) \
            << " in " << __FUNCTION__;


LinkLogCenter::LinkLogCenter(LogHandlerPtr handler, std::string dictionary_path,
                             ScheduledThread &thread) :
    _handler(std::move(handler)), _storage(std::move(dictionary_path), thread),
    _reactor(SERVER_TIMEOUT) {
    assert(_handler);
    _reactor.start(Reactor::EPOLL, GET_ACTIVE_TIMEOUT,
                   [this] { check_timeout(); });
}

bool LinkLogCenter::add_server(const Address &server_address) {
    Socket socket(server_address.is_IPv4() ? AF_INET : AF_INET6, SOCK_STREAM);
    CHECK(socket.connect(server_address), return false)
    CHECK(socket.setNonBlock(true), return false)
    CHECK(socket.setTcpNoDelay(true), return false)

    auto agent_ptr = std::make_unique<TcpMessageAgent>(std::move(socket),
                                                       REMOTE_SOCKET_INPUT_BUFFER_SIZE, 0);
    Channel channel;
    channel.set_readCallback(
        [this] (MessageAgent &agent) {
            handle_read(agent);
        });

    channel.set_errorCallback(
        [this] (MessageAgent &agent) {
            return handle_error(agent);
        });

    channel.set_closeCallback(
        [this] (MessageAgent &agent) {
            handle_close(agent);
        });

    auto [iter, success] = _nodes.try_emplace(server_address, agent_ptr->fd());
    CHECK(success, return false)
    Event event { agent_ptr->fd() };
    event.set_read();
    event.set_error();
    agent_ptr->socket_event.get_extra_data<NodeMapIter>() = iter;
    _reactor.add_channel(std::move(agent_ptr), std::move(channel), event);
    _handler->node_online(server_address, Unix_to_now());
    return true;
}

void LinkLogCenter::remove_server(const Address &server_address) {
    auto iter = _nodes.find(server_address);
    if (iter == _nodes.end()) return;
    _reactor.weak_up_channel(iter->second,
                             [] (MessageAgent &agent, Channel &) {
                                 LinkLogMessage message(LinkLogMessage::CentralOffline);
                                 uint32 written = agent.output().fix_write(&message, sizeof(LinkLogMessage));
                                 CHECK(written == sizeof(LinkLogMessage),);
                                 agent.send_message();
                                 agent.close();
                             });
}

void LinkLogCenter::search_link(const ServiceID &service, LinkLogSearchHandler &handler,
                                uint32 buffer_size) {
    assert(buffer_size >= (1 << 16));

    auto point = _storage.get_query_set(
        service,
        [&handler] (const Index_Key &key, const Index_Value &val) {
            return handler.node_filter(key, val);
        });
    char* buffer = new char[buffer_size];
    char* now = buffer;
    assert(buffer != nullptr);

    uint32 rest = 0;
    while (point || rest > 0) {
        if (rest == 0) {
            auto [written, ptr] = _storage.query(buffer, buffer_size, point);
            now = buffer;
            rest = written;
            point = ptr;
            assert(!(written == 0 && ptr != nullptr));
            if (rest == 0) break;
        }
        auto header = reinterpret_cast<Link_Log_Header *>(now);
        uint16 log_size = header->log_size();
        assert(rest >= log_size + sizeof(Link_Log_Header));
        std::string text(now + sizeof(Link_Log_Header), log_size);
        handler.receive_log(Link_Log(*header, std::move(text)));
        rest -= sizeof(Link_Log_Header) + log_size;
        now += sizeof(Link_Log_Header) + log_size;
    }
    delete[] buffer;
}

void LinkLogCenter::flush() {
    _storage.flush_log_file();
    _storage.flush_index_file();
    _storage.flush_deletion_file();
    _storage.flush_file_name();
}

void LinkLogCenter::delete_oldest_files(uint32 size) {
    _storage.delete_oldest_files(size);
}

uint64 LinkLogCenter::replay_history(LinkLogReplayHandler &handler, const char* file_path) {
    iFile file(file_path, true);
    CHECK(file, return 0;)
    RingBuffer buffer(1 << 16);
    CHECK(buffer.buffer_size() == (1 << 16), return 0;)
    uint64 total_read = 0;
    int64 file_read = 0;
    while ((file_read = file.read(buffer.writable_array())) > 0) {
        buffer.write_advance(file_read);
        uint32 read = 1;
        while (read != 0 && buffer.readable_len() > sizeof(OperationType)) {
            OperationType ot;
            buffer.try_read(&ot, sizeof(OperationType), 0);
            switch (ot) {
            case RegisterLogger:
                read = replay_register_logger(handler, buffer);
                break;
            case CreateLogger:
                read = replay_create_logger(handler, buffer);
                break;
            case EndLogger:
                read = replay_end_logger(handler, buffer);
                break;
            case LinkLog:
                read = replay_log(handler, buffer);
                break;
            case ErrorLogger:
                read = replay_error_logger(handler, buffer);
                break;
            case NodeOffline:
                read = replay_remove_server(buffer);
                break;
            default:
                ASSERT_WRONG_TYPE(ot)
                Global_Logger.flush();
                CurrentThread::emergency_exit(__PRETTY_FUNCTION__);
            }
            total_read += read;
        }
    }
    return total_read;
}

void LinkLogCenter::handle_read(MessageAgent &agent) {
    auto iter = agent.socket_event.get_extra_data<NodeMapIter>();
    auto &buffer = static_cast<const RingBuffer&>(agent.input());

    uint32 read = 1, total_read = 0;
    while (read != 0 && buffer.readable_len() > sizeof(OperationType)) {
        OperationType ot;
        buffer.try_read(&ot, sizeof(OperationType), 0);
        switch (ot) {
        case RegisterLogger:
            read = handle_register_logger(iter, buffer);
            break;
        case CreateLogger:
            read = handle_create_logger(iter, buffer);
            break;
        case EndLogger:
            read = handle_end_logger(iter, buffer);
            break;
        case LinkLog:
            read = handle_log(iter, buffer);
            break;
        case ErrorLogger:
            read = handle_error_logger(iter, buffer);
            break;
        case NodeOffline:
            read = handle_remove_server(iter, agent);
            assert(buffer.readable_len() == 0 || read == 0);
            break;
        default:
            ASSERT_WRONG_TYPE(ot)
            Global_Logger.flush();
            CurrentThread::emergency_exit(__PRETTY_FUNCTION__);
        }
        total_read += read;
    }
    buffer.read_back(total_read);
    _storage.write_to_file(buffer, total_read);
}

bool LinkLogCenter::handle_error(MessageAgent &agent) const {
    auto iter = agent.socket_event.get_extra_data<NodeMapIter>();
    switch (agent.error.types) {
    case error_types::TimeoutEvent:
        G_WARN << "LinkLogCenter: service node timeout.";
        return _handler->node_timeout(iter->first);
    default:
        G_ERROR << "LinkLogCenter: " << get_error_type_name(agent.error.types)
                << " " << strerror(agent.error.codes);
        _handler->node_error(iter->first, agent.error);
        return true;
    }
}

void LinkLogCenter::handle_close(MessageAgent &agent) {
    auto iter = agent.socket_event.get_extra_data<NodeMapIter>();
    _nodes.erase(iter);
    agent.socket_event.get_extra_data<NodeMapIter>() = _nodes.end();
}

void LinkLogCenter::check_timeout() {
    auto now = Unix_to_now();
    while (!_timers.empty() && _timers.top().expire_time <= now) {
        if (_timers.top().check_timeout()) {
            auto &[expire_time, iter, ptr, address] = _timers.top();
            if (iter->second.type != RpcDecision) {
                _handler->handling_error(address, iter->first.serviceID(),
                                         iter->first.nodeID(), expire_time,
                                         iter->second.type == BranchHead
                                             ? NotRegister : CreateTimeOut);
            }
            _check.erase(iter);
        }
        _timers.pop();
    }
}

uint32 LinkLogCenter::handle_register_logger(NodeMapIter iter, const RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(Register_Logger))
        return 0;
    Register_Logger logger;
    buffer.read(&logger, sizeof(Register_Logger));
    if (logger.type() > Decision) {
        auto [new_iter, success] = _check.try_emplace(
            ID(logger.service(), logger.node()), logger);
        if (!success) {
            _storage.update_logger(logger.service(), logger.node(), logger.parent_init_time(),
                                   logger.parent_node(), logger.type(), new_iter->second.init_time);
            new_iter->second.on_time();
            _check.erase(new_iter);
        } else {
            new_iter->second.create();
            _timers.emplace(logger.expire_time(), new_iter, iter->first);
        }
    }
    _handler->register_logger(iter->first, logger.service(), logger.node(), logger.parent_node(),
                              logger.type(), logger.time(), logger.parent_init_time());
    _storage.add_record(sizeof(Register_Logger));
    return sizeof(Register_Logger);
}

uint32 LinkLogCenter::handle_create_logger(NodeMapIter iter, const RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(Create_Logger))
        return 0;
    Create_Logger logger;
    buffer.read(&logger, sizeof(Create_Logger));
    if (logger.type() == BranchHead) {
        auto [new_iter, success] = _check.try_emplace(
            ID(logger.service(), logger.node()), logger);
        if (!success) {
            logger.type() = new_iter->second.type;
            logger.parent_node() = new_iter->second.parent;
            logger.parent_init_time() = new_iter->second.parent_init_time;
            new_iter->second.on_time();
            _check.erase(new_iter);
        } else {
            new_iter->second.create();
            _timers.emplace(Unix_to_now() + ILLEGAL_LOGGER_TIMOUT, new_iter, iter->first);
        }
    }
    bool success = _storage.create_logger(logger.service(), logger.node(), logger.parent_init_time(),
                                          logger.parent_node(), logger.type(), logger.init_time());
    if (likely(success)) {
        if (logger.type() == Head) {
            _handler->create_head_logger(iter->first, logger.service(),
                                         logger.node(), logger.init_time());
        } else {
            _handler->create_logger(iter->first, logger.service(),
                                    logger.node(), logger.init_time());
        }
    } else {
        _handler->handling_error(iter->first, logger.service(), logger.node(), logger.init_time(),
                                 ConflictingNode);
    }
    _storage.add_record(sizeof(Create_Logger));
    return sizeof(Create_Logger);
}

uint32 LinkLogCenter::handle_end_logger(NodeMapIter iter, const RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(End_Logger))
        return 0;
    End_Logger logger;
    buffer.read(&logger, sizeof(End_Logger));
    _storage.end_logger(logger.service(), logger.node(), logger.init_time());
    _handler->logger_end(iter->first, logger.service(), logger.node(), logger.end_time());
    _storage.add_record(sizeof(End_Logger));
    return sizeof(End_Logger);
}

uint32 LinkLogCenter::handle_log(NodeMapIter iter, const RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(Link_Log_Header))
        return 0;
    Link_Log_Header header;
    buffer.try_read(&header, sizeof(Link_Log_Header), 0);
    if (buffer.readable_len() < header.log_size() + sizeof(Link_Log_Header))
        return 0;

    _storage.handle_a_log(header, buffer);
    buffer.read_advance(sizeof(Link_Log_Header));
    std::string text(header.log_size(), '\0');
    buffer.read(text.data(), header.log_size());
    Link_Log log(header, std::move(text));
    _handler->receive_log(iter->first, log);
    _storage.add_record(sizeof(Link_Log_Header) + header.log_size());
    return sizeof(Link_Log_Header) + header.log_size();
}

uint32 LinkLogCenter::handle_error_logger(NodeMapIter iter, const RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(Error_Logger))
        return 0;
    Error_Logger logger;
    buffer.read(&logger, sizeof(Error_Logger));
    _handler->handling_error(iter->first, logger.service(), logger.node(),
                             logger.time(), logger.error_type());
    _storage.add_record(sizeof(Error_Logger));
    return sizeof(Error_Logger);
}

uint32 LinkLogCenter::handle_remove_server(NodeMapIter iter, MessageAgent &agent) {
    auto &buffer = agent.input();
    if (buffer.readable_len() < sizeof(Node_Offline))
        return 0;
    Node_Offline offline;
    buffer.read(&offline, sizeof(Node_Offline));
    agent.socket_event.set_HangUp();
    _handler->node_offline(iter->first, offline.time());
    _storage.add_record(sizeof(Node_Offline));
    return sizeof(Node_Offline);
}

uint32 LinkLogCenter::replay_register_logger(LinkLogReplayHandler &handler,
                                             RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(Register_Logger))
        return 0;
    Register_Logger logger;
    buffer.read(&logger, sizeof(Register_Logger));
    handler.register_logger(logger.service(), logger.node(), logger.parent_node(),
                            logger.type(), logger.time(), logger.parent_init_time());
    return sizeof(Register_Logger);
}

uint32 LinkLogCenter::replay_create_logger(LinkLogReplayHandler &handler, RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(Create_Logger))
        return 0;
    Create_Logger logger;
    buffer.read(&logger, sizeof(Create_Logger));
    if (logger.type() == Head) {
        handler.create_head_logger(logger.service(), logger.node(), logger.init_time());
    } else {
        handler.create_logger(logger.service(), logger.node(), logger.init_time());
    }
    return sizeof(Create_Logger);
}

uint32 LinkLogCenter::replay_end_logger(LinkLogReplayHandler &handler, RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(End_Logger))
        return 0;
    End_Logger logger;
    buffer.read(&logger, sizeof(End_Logger));
    handler.logger_end(logger.service(), logger.node(), logger.end_time());
    return sizeof(End_Logger);
}

uint32 LinkLogCenter::replay_log(LinkLogReplayHandler &handler, RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(Link_Log_Header))
        return 0;
    Link_Log_Header header;
    buffer.try_read(&header, sizeof(Link_Log_Header), 0);
    if (buffer.readable_len() < header.log_size() + sizeof(Link_Log_Header))
        return 0;

    buffer.read_advance(sizeof(Link_Log_Header));
    std::string text(header.log_size(), '\0');
    buffer.read(text.data(), header.log_size());
    Link_Log log(header, std::move(text));
    handler.receive_log(log);
    return sizeof(Link_Log_Header) + header.log_size();
}

uint32 LinkLogCenter::replay_error_logger(LinkLogReplayHandler &handler, RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(Error_Logger))
        return 0;
    Error_Logger logger;
    buffer.read(&logger, sizeof(Error_Logger));
    handler.handling_error(logger.service(), logger.node(),
                           logger.time(), logger.error_type());
    return sizeof(Error_Logger);
}

uint32 LinkLogCenter::replay_remove_server(RingBuffer &buffer) {
    if (buffer.readable_len() < sizeof(Node_Offline))
        return 0;
    Node_Offline offline;
    buffer.read(&offline, sizeof(Node_Offline));
    return sizeof(Node_Offline);
}
