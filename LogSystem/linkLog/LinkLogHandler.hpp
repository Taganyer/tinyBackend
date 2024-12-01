//
// Created by taganyer on 24-11-1.
//

#ifndef LOGSYSTEM_LINKLOGHANDLER_HPP
#define LOGSYSTEM_LINKLOGHANDLER_HPP

#ifdef LOGSYSTEM_LINKLOGHANDLER_HPP

#include <string>
#include "Net/InetAddress.hpp"
#include "LinkLogMessage.hpp"
#include "LogSystem/LogRank.hpp"
#include "LinkLogOperation.hpp"
#include "Net/error/error_mark.hpp"


namespace LogSystem {

    class LinkLogCenter;

    struct Link_Log {
        LinkServiceID service;

        Base::TimeDifference node_init_time;

        LinkNodeID node;

        LogRank rank = EMPTY;

        Base::TimeDifference time;

        std::string text;

        Link_Log() = default;

        Link_Log(Link_Log_Header &header, std::string text) :
            service(header.service()), node_init_time(header.init_time()),
            node(header.node()), rank(header.rank()), time(header.time()),
            text(std::move(text)) {};

    };

    class LinkLogServerHandler {
    public:
        using Address = Net::InetAddress;

        virtual ~LinkLogServerHandler() = default;

        virtual void create_head_logger(LinkServiceID service, LinkNodeID node,
                                        Base::TimeDifference time) = 0;

        virtual void register_logger(LinkServiceID service, LinkNodeID node,
                                     LinkNodeID parent_node, LinkNodeType type,
                                     Base::TimeDifference time, Base::TimeDifference expire_time) = 0;

        virtual void create_logger(LinkServiceID service, LinkNodeID node,
                                   Base::TimeDifference init_time) = 0;

        virtual void logger_end(LinkServiceID service, LinkNodeID node,
                                Base::TimeDifference end_time) = 0;

        virtual void handling_error(LinkServiceID service, LinkNodeID node,
                                    Base::TimeDifference time, LinkErrorType type) = 0;

        virtual void center_online(const Address &center_address) = 0;

        virtual void center_offline() = 0;

        virtual void center_error(Net::error_mark mark) = 0;

    };

    class LinkLogCenterHandler {
    public:
        using Address = Net::InetAddress;

        virtual ~LinkLogCenterHandler() = default;

        virtual void create_head_logger(const Address &address, LinkServiceID service, LinkNodeID node,
                                        Base::TimeDifference time) = 0;

        virtual void register_logger(const Address &address, LinkServiceID service, LinkNodeID node,
                                     LinkNodeID parent_node, LinkNodeType type,
                                     Base::TimeDifference time, Base::TimeDifference expire_time) = 0;

        virtual void create_logger(const Address &address, LinkServiceID service, LinkNodeID node,
                                   Base::TimeDifference init_time) = 0;

        virtual void logger_end(const Address &address, LinkServiceID service, LinkNodeID node,
                                Base::TimeDifference end_time) = 0;

        virtual void receive_log(const Address &address, Link_Log log) = 0;

        virtual void handling_error(const Address &address, LinkServiceID service, LinkNodeID node,
                                    Base::TimeDifference time, LinkErrorType type) = 0;

        virtual void node_online(const Address &address, Base::TimeDifference time) = 0;

        virtual void node_offline(const Address &address, Base::TimeDifference time) = 0;

        virtual bool node_timeout(const Address &address) = 0;

        virtual void node_error(const Address &address, Net::error_mark mark) = 0;

    };

    class LinkLogSearchHandler {
    public:
        virtual ~LinkLogSearchHandler() = default;

        virtual void receive_log(Link_Log) = 0;

        virtual bool node_filter(Index_Key key, Index_Value val) = 0;

    };

    class LinkLogReplayHandler {
    public:
        virtual ~LinkLogReplayHandler() = default;

        virtual void create_head_logger(LinkServiceID service, LinkNodeID node,
                                        Base::TimeDifference time) = 0;

        virtual void register_logger(LinkServiceID service, LinkNodeID node,
                                     LinkNodeID parent_node, LinkNodeType type,
                                     Base::TimeDifference time, Base::TimeDifference expire_time) = 0;

        virtual void create_logger(LinkServiceID service, LinkNodeID node,
                                   Base::TimeDifference init_time) = 0;

        virtual void logger_end(LinkServiceID service, LinkNodeID node,
                                Base::TimeDifference end_time) = 0;

        virtual void receive_log(Link_Log log) = 0;

        virtual void handling_error(LinkServiceID service, LinkNodeID node,
                                    Base::TimeDifference time, LinkErrorType type) = 0;

    };

}

#endif

#endif //LOGSYSTEM_LINKLOGHANDLER_HPP
