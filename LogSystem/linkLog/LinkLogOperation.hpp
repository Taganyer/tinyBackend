//
// Created by taganyer on 24-11-4.
//

#ifndef LOGSYSTEM_LINKLOGOPERATION_HPP
#define LOGSYSTEM_LINKLOGOPERATION_HPP

#ifdef LOGSYSTEM_LINKLOGOPERATION_HPP

#include "LinkLogErrors.hpp"
#include "LogSystem/LogRank.hpp"
#include "Base/Time/Time_difference.hpp"


namespace LogSystem {

    class Index_Key {
    public:
        static constexpr uint32 size =
            sizeof(LinkServiceID) +
            sizeof(Base::Time_difference) +
            sizeof(LinkNodeID);

        Index_Key(const LinkServiceID &service_, Base::Time_difference init_time_,
                  const LinkNodeID &node_) {
            service() = service_;
            init_time() = init_time_;
            node() = node_;
        };

        Index_Key() = default;

        LinkServiceID& service() {
            return *(LinkServiceID *) object.data();
        };

        Base::Time_difference& init_time() {
            return *(Base::Time_difference *) (object.data() + sizeof(LinkServiceID));
        };

        LinkNodeID& node() {
            return *(LinkNodeID *) (object.data() + sizeof(LinkServiceID) + sizeof(Base::Time_difference));
        };

        friend bool operator==(const Index_Key &lhs, const Index_Key &rhs) {
            return rhs.object == lhs.object;
        };

        friend bool operator!=(const Index_Key &lhs, const Index_Key &rhs) {
            return lhs.object != rhs.object;
        };

        friend bool operator<(const Index_Key &lhs, const Index_Key &rhs) {
            return lhs.object < rhs.object;
        };

        friend bool operator>(const Index_Key &lhs, const Index_Key &rhs) {
            return lhs.object > rhs.object;
        }

        Mark<size> object;

    };

    class Index_Value {
    public:
        static constexpr uint32 size =
            sizeof(Base::Time_difference) +
            sizeof(LinkNodeID) +
            sizeof(LinkNodeType) +
            sizeof(Base::Time_difference) +
            sizeof(uint32);

        Index_Value(Base::Time_difference parent_init_time_, const LinkNodeID &parent_node_,
                    LinkNodeType type_, Base::Time_difference latest_file_, uint32 latest_index_) {
            parent_init_time() = parent_init_time_;
            parent_node() = parent_node_;
            type() = type_;
            latest_file() = latest_file_;
            latest_index() = latest_index_;
        };

        Index_Value() = default;

        Base::Time_difference& parent_init_time() {
            return *(Base::Time_difference *) object.data();
        };

        LinkNodeID& parent_node() {
            return *(LinkNodeID *) (object.data() + sizeof(Base::Time_difference));
        };

        LinkNodeType& type() {
            return *(LinkNodeType *) (object.data() + sizeof(Base::Time_difference) + sizeof(LinkNodeID));
        };

        Base::Time_difference& latest_file() {
            return *(Base::Time_difference *) (object.data() + sizeof(Base::Time_difference)
                + sizeof(LinkNodeID) + sizeof(LinkNodeType));
        };

        uint32& latest_index() {
            return *(uint32 *) (object.data() + sizeof(Base::Time_difference)
                + sizeof(LinkNodeID) + sizeof(LinkNodeType) + sizeof(Base::Time_difference));
        };

        Mark<size> object;

    };

    enum OperationType : uint8 {
        Null,
        RegisterLogger,
        CreateLogger,
        EndLogger,
        LinkLog,
        ErrorLogger,
        NodeOffline
    };

    inline const char* getOperationTypeName(OperationType type) {
        constexpr const char* name[] {
            "Null",
            "RegisterLogger",
            "CreateLogger",
            "EndLogger",
            "LinkLog",
            "ErrorLogger",
            "NodeOffline"
        };
        return name[type];
    };

    class Register_Logger {
    public:
        static constexpr uint32 size =
            sizeof(LinkNodeType) +
            sizeof(LinkServiceID) +
            sizeof(LinkNodeID) +
            sizeof(LinkNodeID) +
            sizeof(Base::Time_difference) +
            sizeof(Base::Time_difference) +
            sizeof(Base::Time_difference);

        Register_Logger(LinkNodeType type_, const LinkServiceID &service_,
                        const LinkNodeID &node_, const LinkNodeID &parent_node_,
                        Base::Time_difference time_,
                        Base::Time_difference parent_init_time_,
                        Base::Time_difference expire_time_) {
            type() = type_;
            service() = service_;
            node() = node_;
            parent_node() = parent_node_;
            time() = time_;
            parent_init_time() = parent_init_time_;
            expire_time() = expire_time_;
        };

        Register_Logger() = default;

        LinkNodeType& type() {
            return *(LinkNodeType *) object.data();
        };

        LinkServiceID& service() {
            return *(LinkServiceID *) (object.data() + sizeof(LinkNodeType));
        };

        LinkNodeID& node() {
            return *(LinkNodeID *) (object.data() + sizeof(LinkNodeType) + sizeof(LinkServiceID));
        };

        LinkNodeID& parent_node() {
            return *(LinkNodeID *) (object.data() + sizeof(LinkNodeType) + sizeof(LinkServiceID)
                + sizeof(LinkNodeID));
        };

        Base::Time_difference& time() {
            return *(Base::Time_difference *) (object.data() + sizeof(LinkNodeType) + sizeof(LinkServiceID)
                + sizeof(LinkNodeID) + sizeof(LinkNodeID));
        };

        Base::Time_difference& parent_init_time() {
            return *(Base::Time_difference *) (object.data() + sizeof(LinkNodeType) + sizeof(LinkServiceID)
                + sizeof(LinkNodeID) + sizeof(LinkNodeID) + sizeof(Base::Time_difference));
        };

        Base::Time_difference& expire_time() {
            return *(Base::Time_difference *) (object.data() + sizeof(LinkNodeType) + sizeof(LinkServiceID)
                + sizeof(LinkNodeID) + sizeof(LinkNodeID) + sizeof(Base::Time_difference)
                + sizeof(Base::Time_difference));
        };

        OperationType ot = RegisterLogger;

        Mark<size> object;

    };

    class Create_Logger {
    public:
        static constexpr uint32 size =
            sizeof(LinkNodeType) +
            sizeof(LinkServiceID) +
            sizeof(LinkNodeID) +
            sizeof(LinkNodeID) +
            sizeof(Base::Time_difference) +
            sizeof(Base::Time_difference);

        Create_Logger(LinkNodeType type_, const LinkServiceID &service_,
                      const LinkNodeID &node_, const LinkNodeID &parent_node_,
                      Base::Time_difference init_time_, Base::Time_difference parent_init_time_) {
            type() = type_;
            service() = service_;
            node() = node_;
            parent_node() = parent_node_;
            init_time() = init_time_;
            parent_init_time() = parent_init_time_;
        };

        Create_Logger() = default;

        LinkNodeType& type() {
            return *(LinkNodeType *) object.data();
        };

        LinkServiceID& service() {
            return *(LinkServiceID *) (object.data() + sizeof(LinkNodeType));
        };

        LinkNodeID& node() {
            return *(LinkNodeID *) (object.data() + sizeof(LinkNodeType) + sizeof(LinkServiceID));
        };

        LinkNodeID& parent_node() {
            return *(LinkNodeID *) (object.data() + sizeof(LinkNodeType) + sizeof(LinkServiceID)
                + sizeof(LinkNodeID));
        };

        Base::Time_difference& init_time() {
            return *(Base::Time_difference *) (object.data() + sizeof(LinkNodeType) + sizeof(LinkServiceID)
                + sizeof(LinkNodeID) + sizeof(LinkNodeID));
        };

        Base::Time_difference& parent_init_time() {
            return *(Base::Time_difference *) (object.data() + sizeof(LinkNodeType) + sizeof(LinkServiceID)
                + sizeof(LinkNodeID) + sizeof(LinkNodeID) + sizeof(Base::Time_difference));
        };

        OperationType ot = CreateLogger;

        Mark<size> object;

    };

    class End_Logger {
    public:
        static constexpr uint32 size =
            sizeof(LinkServiceID) +
            sizeof(LinkNodeID) +
            sizeof(Base::Time_difference) +
            sizeof(Base::Time_difference);

        End_Logger(const LinkServiceID &service_, const LinkNodeID &node_,
                   Base::Time_difference init_time_, Base::Time_difference end_time_) {
            service() = service_;
            node() = node_;
            init_time() = init_time_;
            end_time() = end_time_;
        };

        End_Logger() = default;

        LinkServiceID& service() {
            return *(LinkServiceID *) object.data();
        };

        LinkNodeID& node() {
            return *(LinkNodeID *) (object.data() + sizeof(LinkServiceID));
        };

        Base::Time_difference& init_time() {
            return *(Base::Time_difference *) (object.data() + sizeof(LinkServiceID)
                + sizeof(LinkNodeID));
        };

        Base::Time_difference& end_time() {
            return *(Base::Time_difference *) (object.data() + sizeof(LinkServiceID)
                + sizeof(LinkNodeID) + sizeof(Base::Time_difference));
        };

        OperationType ot = EndLogger;

        Mark<size> object;

    };

    class Link_Log_Header {
    public:
        static constexpr uint32 size =
            sizeof(uint16) +
            sizeof(Base::Time_difference) +
            sizeof(uint32) +
            sizeof(Index_Key) +
            sizeof(Base::Time_difference) +
            sizeof(LogRank);

        Link_Log_Header(uint16 log_size_, const LinkServiceID &service_, Base::Time_difference init_time_,
                        const LinkNodeID &node_, Base::Time_difference time_, LogRank rank_) {
            log_size() = log_size_;
            service() = service_;
            init_time() = init_time_;
            node() = node_;
            time() = time_;
            rank() = rank_;
        };

        Link_Log_Header() = default;

        uint16& log_size() {
            return *(uint16 *) object.data();
        };

        Base::Time_difference& latest_file() {
            return *(Base::Time_difference *) (object.data() + sizeof(uint16));
        };

        uint32& latest_index() {
            return *(uint32 *) (object.data() + sizeof(uint16) + sizeof(Base::Time_difference));
        };

        LinkServiceID& service() {
            return *(LinkServiceID *) (object.data() + sizeof(uint16)
                + sizeof(Base::Time_difference) + sizeof(uint32));
        };

        Base::Time_difference& init_time() {
            return *(Base::Time_difference *) (object.data() + sizeof(uint16) + sizeof(Base::Time_difference)
                + sizeof(uint32) + sizeof(LinkServiceID));
        };

        LinkNodeID& node() {
            return *(LinkNodeID *) (object.data() + sizeof(uint16) + sizeof(Base::Time_difference)
                + sizeof(uint32) + sizeof(LinkServiceID) + sizeof(Base::Time_difference));
        };

        Base::Time_difference& time() {
            return *(Base::Time_difference *) (object.data() + sizeof(uint16) + sizeof(Base::Time_difference)
                + sizeof(uint32) + sizeof(LinkServiceID) + sizeof(Base::Time_difference)
                + sizeof(LinkNodeID));
        };

        LogRank& rank() {
            return *(LogRank *) (object.data() + sizeof(uint16) + sizeof(Base::Time_difference)
                + sizeof(uint32) + sizeof(LinkServiceID) + sizeof(Base::Time_difference)
                + sizeof(LinkNodeID) + sizeof(Base::Time_difference));
        };

        OperationType ot = LinkLog;

        Mark<size> object;

    };

    class Error_Logger {
    public:
        static constexpr uint32 size =
            sizeof(LinkServiceID) +
            sizeof(LinkNodeID) +
            sizeof(Base::Time_difference) +
            sizeof(LinkErrorType);

        Error_Logger(const LinkServiceID &service_, const LinkNodeID &node_,
                     Base::Time_difference time_, LinkErrorType error_type_) {
            service() = service_;
            node() = node_;
            time() = time_;
            error_type() = error_type_;
        };

        Error_Logger() = default;

        LinkServiceID& service() {
            return *(LinkServiceID *) object.data();
        };

        LinkNodeID& node() {
            return *(LinkNodeID *) (object.data() + sizeof(LinkServiceID));
        };

        Base::Time_difference& time() {
            return *(Base::Time_difference *) (object.data() + sizeof(LinkServiceID)
                + sizeof(LinkNodeID));
        };

        LinkErrorType& error_type() {
            return *(LinkErrorType *) (object.data() + sizeof(LinkServiceID) + sizeof(LinkNodeID)
                + sizeof(Base::Time_difference));
        };

        OperationType ot = ErrorLogger;

        Mark<size> object;

    };

    class Node_Offline {
    public:
        static constexpr uint32 size =
            sizeof(Base::Time_difference);

        explicit Node_Offline(Base::Time_difference time_) {
            time() = time_;
        };

        Node_Offline() = default;

        Base::Time_difference& time() {
            return *(Base::Time_difference *) object.data();
        };

        OperationType ot = NodeOffline;

        Mark<size> object;

    };

}

namespace std {

    template <>
    struct hash<LogSystem::Index_Key> {
        size_t operator()(const LogSystem::Index_Key &key) const noexcept {
            return _Hash_impl::hash(&key, LogSystem::Index_Key::size);
        };
    };

}

#endif

#endif //LOGSYSTEM_LINKLOGOPERATION_HPP
