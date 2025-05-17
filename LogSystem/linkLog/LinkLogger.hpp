//
// Created by taganyer on 24-10-15.
//

#ifndef LOGSYSTEM_LINKLOGGER_HPP
#define LOGSYSTEM_LINKLOGGER_HPP

#ifdef LOGSYSTEM_LINKLOGGER_HPP

#include "LinkLogServer.hpp"
#include "Base/LogRank.hpp"


namespace LogSystem {

    class LinkLogServer;

    class LinkLogStream;

    class LinkLogger : Base::NoCopy {
    public:
        using Server = LinkLogServer;

        using Type = Server::Type;

        using ID = Server::ID;

        using ServiceID = Server::ServiceID;

        using NodeID = Server::NodeID;

        LinkLogger(LogRank rank, const ServiceID &service, const NodeID &node,
                   bool is_branch, Base::TimeInterval end_timeout, LinkLogServer &server);

        LinkLogger(LogRank rank, const ID &head_id,
                   bool is_branch, Base::TimeInterval end_timeout, LinkLogServer &server);

        LinkLogger(LogRank rank, const ServiceID &service, const NodeID &node,
                   Base::TimeInterval end_timeout, LinkLogServer &server);

        LinkLogger(LogRank rank, const ID &complete_id,
                   Base::TimeInterval end_timeout, LinkLogServer &server);

        LinkLogger(LogRank rank, const LinkLogger &parent, const NodeID &node,
                   Base::TimeInterval end_timeout);

        LinkLogger(LinkLogger &&other) noexcept;

        ~LinkLogger();

        void push(LogRank rank, const void* data, uint16 size) const;

        void register_child_node(Type type, const NodeID &node_id,
                                 Base::TimeInterval create_timeout) const;

        void fork(const NodeID &child_node, Base::TimeInterval create_timeout) const;

        void follow(const NodeID &child_node, Base::TimeInterval create_timeout) const;

        void decision(const NodeID &child_node, Base::TimeInterval create_timeout) const;

        void rpc_fork(const NodeID &child_node, Base::TimeInterval create_timeout) const;

        void rpc_follow(const NodeID &child_node, Base::TimeInterval create_timeout) const;

        void rpc_decision(const NodeID &child_node, Base::TimeInterval create_timeout) const;

        void close();

        LinkLogStream stream(LogRank rank);

        void set_rank(LogRank rank) { _rank = rank; };

        [[nodiscard]] LogRank get_rank() const { return _rank; };

        [[nodiscard]] bool valid() const { return _server != nullptr; };

    private:
        using Iter = LinkLogServer::MapIter;

        LogRank _rank;

        Iter _iter;

        LinkLogServer* _server;

    };

    class LinkLogStream : Base::NoCopy {
    public:
        static constexpr uint16 BUFFER_SIZE = 256;

        LinkLogStream(LinkLogger &log, LogRank rank) : _log(&log), _rank(rank) {};

        ~LinkLogStream() {
            _log->push(_rank, _message, _index);
        };

        LinkLogStream& operator<<(const std::string &val) {
            if (_log->get_rank() > _rank) return *this;
            auto len = val.size() > BUFFER_SIZE - _index ? BUFFER_SIZE - _index : val.size();
            memcpy(_message + _index, val.data(), len);
            _index += len;
            return *this;
        };

        LinkLogStream& operator<<(const std::string_view &val) {
            if (_log->get_rank() > _rank) return *this;
            auto len = val.size() > BUFFER_SIZE - _index ? BUFFER_SIZE - _index : val.size();
            memcpy(_message + _index, val.data(), len);
            _index += len;
            return *this;
        };

#define LinkStreamOperator(type, f) LinkLogStream &operator<<(type val) { \
            if (_log->get_rank() > _rank) return *this;       \
            _index += snprintf(_message + _index, BUFFER_SIZE - _index, f, val); \
            return *this;                                     \
        };

        LinkStreamOperator(char, "%c")

        LinkStreamOperator(const char *, "%s")

        LinkStreamOperator(int, "%d")

        LinkStreamOperator(long, "%ld")

        LinkStreamOperator(long long, "%lld")

        LinkStreamOperator(unsigned, "%u")

        LinkStreamOperator(unsigned long, "%lu")

        LinkStreamOperator(unsigned long long, "%llu")

        LinkStreamOperator(double, "%lf")

#undef LinkStreamOperator

    private:
        LinkLogger* _log;

        LogRank _rank;

        uint32 _index = 0;

        char _message[BUFFER_SIZE] {};

    };

}

#endif

#endif //LOGSYSTEM_LINKLOGGER_HPP
