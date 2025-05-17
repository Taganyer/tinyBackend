//
// Created by taganyer on 24-10-15.
//

#ifndef LOGSYSTEM_LINKLOGCENTER_HPP
#define LOGSYSTEM_LINKLOGCENTER_HPP

#ifdef LOGSYSTEM_LINKLOGCENTER_HPP

#include "LinkLogHandler.hpp"
#include "LinkLogInterpreter.hpp"
#include "tinyBackend/Net/reactor/Reactor.hpp"
#include "tinyBackend/Net/InetAddress.hpp"

namespace Net {
    class MessageAgent;
}

namespace LogSystem {

    class LinkLogCenter {
    public:
        using Address = Net::InetAddress;
        using Type = LinkNodeType;
        using ID = Identification;
        using ServiceID = LinkServiceID;
        using NodeID = LinkNodeID;

        static constexpr uint32 REMOTE_SOCKET_INPUT_BUFFER_SIZE = (1 << 22);

        static_assert(REMOTE_SOCKET_INPUT_BUFFER_SIZE >= (1 << 16));

        static constexpr Base::TimeInterval SERVER_TIMEOUT = Base::operator ""_min(1);

        static constexpr Base::TimeInterval ILLEGAL_LOGGER_TIMOUT = Base::operator ""_s(10);

        static constexpr int32 GET_ACTIVE_TIMEOUT = SERVER_TIMEOUT.to_ms();

        using LogHandlerPtr = std::shared_ptr<LinkLogCenterHandler>;

        explicit LinkLogCenter(LogHandlerPtr handler, std::string dictionary_path,
                               Base::ScheduledThread& thread);

        bool add_server(const Address& server_address);

        void remove_server(const Address& server_address);

        void search_link(const ServiceID& service, LinkLogSearchHandler& handler,
                         uint32 buffer_size = (1 << 16));

        void flush();

        void delete_oldest_files(uint32 size);

        static uint64 replay_history(LinkLogReplayHandler& handler, const char *file_path);

    private:
        struct NodeData {
            Net::Channel channel;

            explicit NodeData(Net::Channel&& ch) :
                channel(std::move(ch)) {};
        };

        using NodeMap = std::map<Address, int>;

        using NodeMapIter = NodeMap::iterator;

        struct LoggerCheckData {
            LinkNodeID parent;
            LinkNodeType type;
            Base::TimeInterval init_time, parent_init_time;
            bool *logger_timout = nullptr;

            explicit LoggerCheckData(Register_Logger& logger) :
                parent(logger.parent_node()),
                type(logger.type()) {};

            explicit LoggerCheckData(Create_Logger& logger) :
                type(logger.type()),
                init_time(logger.init_time()) {};

            void create() {
                logger_timout = new bool(true);
            };

            void on_time() const {
                *logger_timout = false;
            }
        };

        using CheckMap = std::map<ID, LoggerCheckData>;

        using CheckMapIter = CheckMap::iterator;

        struct TimerData {
            Base::TimeInterval expire_time;
            CheckMapIter iter;
            bool *logger_timout = nullptr;
            Address address;

            explicit TimerData(Base::TimeInterval expire_time, CheckMapIter iter,
                               const Address& address) :
                expire_time(expire_time), iter(iter), logger_timout(iter->second.logger_timout),
                address(address) {};

            TimerData(const TimerData&) = default;

            [[nodiscard]] bool check_timeout() const {
                bool timeout = *logger_timout;
                delete logger_timout;
                return timeout;
            };

            friend bool operator<(const TimerData& lhs, const TimerData& rhs) {
                return lhs.expire_time > rhs.expire_time;
            };
        };

        using TimerQueue = std::priority_queue<TimerData>;

        NodeMap _nodes;

        CheckMap _check;

        TimerQueue _timers;

        LogHandlerPtr _handler;

        LinkLogStorage _storage;

        Net::Reactor _reactor;

        void handle_read(Net::MessageAgent& agent);

        bool handle_error(Net::MessageAgent& agent) const;

        void handle_close(Net::MessageAgent& agent);

        void check_timeout();

        uint32 handle_register_logger(NodeMapIter iter, const Base::RingBuffer& buffer);

        uint32 handle_create_logger(NodeMapIter iter, const Base::RingBuffer& buffer);

        uint32 handle_end_logger(NodeMapIter iter, const Base::RingBuffer& buffer);

        uint32 handle_log(NodeMapIter iter, const Base::RingBuffer& buffer);

        uint32 handle_error_logger(NodeMapIter iter, const Base::RingBuffer& buffer);

        uint32 handle_remove_server(NodeMapIter iter, Net::MessageAgent& agent);

        static uint32 replay_register_logger(LinkLogReplayHandler& handler, Base::RingBuffer& buffer);

        static uint32 replay_create_logger(LinkLogReplayHandler& handler, Base::RingBuffer& buffer);

        static uint32 replay_end_logger(LinkLogReplayHandler& handler, Base::RingBuffer& buffer);

        static uint32 replay_log(LinkLogReplayHandler& handler, Base::RingBuffer& buffer);

        static uint32 replay_error_logger(LinkLogReplayHandler& handler, Base::RingBuffer& buffer);

        static uint32 replay_remove_server(Base::RingBuffer& buffer);

    };

}

#endif

#endif //LOGSYSTEM_LINKLOGCENTER_HPP
