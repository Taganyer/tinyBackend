//
// Created by taganyer on 24-10-13.
//

#ifndef LOGSYSTEM_LINKLOGSERVERE_HPP
#define LOGSYSTEM_LINKLOGSERVERE_HPP

#ifdef LOGSYSTEM_LINKLOGSERVERE_HPP

#include <map>
#include "Net/Acceptor.hpp"
#include "Net/InetAddress.hpp"
#include "LinkLogErrors.hpp"
#include "LinkLogHandler.hpp"
#include "Net/TcpMessageAgent.hpp"
#include "LinkLogInterpreter.hpp"


namespace Net {
    struct Event;
    class Poller;
}

namespace LogSystem {

    class LinkLogMessage;

    class LinkLogServer {
    public:
        static constexpr uint32 BLOCK_SIZE = Base::BufferPool::BLOCK_SIZE << 6;

        static_assert(BLOCK_SIZE >= 1 << 16);

        static constexpr uint32 LOCAL_SOCKET_INPUT_BUFFER_SIZE = 1 << 10;

        static_assert(LOCAL_SOCKET_INPUT_BUFFER_SIZE >= sizeof(LinkLogMessage));

        static constexpr uint32 REMOTE_SOCKET_INPUT_BUFFER_SIZE = 1 << 8;

        static_assert(REMOTE_SOCKET_INPUT_BUFFER_SIZE >= sizeof(LinkLogMessage));

        static constexpr uint32 REMOTE_SOCKET_OUTPUT_BUFFER_SIZE = 1 << 22;

        static_assert(REMOTE_SOCKET_OUTPUT_BUFFER_SIZE >= 1 << 16);

        static constexpr int32 GET_ACTIVE_TIMEOUT_MS = 500;

        static constexpr Base::TimeInterval BUFFER_FLUSH_TIME = Base::operator ""_s(1);

        static_assert(GET_ACTIVE_TIMEOUT_MS > 0 && BUFFER_FLUSH_TIME.to_ms() > GET_ACTIVE_TIMEOUT_MS,
                      "Unable to refresh buffers in time");

        using Address = Net::InetAddress;
        using Type = LinkNodeType;
        using ID = Identification;
        using ServiceID = LinkServiceID;
        using NodeID = LinkNodeID;
        using ServerHandlerPtr = std::shared_ptr<LinkLogServerHandler>;

        enum PartitionRank {
            Level1 = 2,
            Level2 = 3,
            Level3 = 4,
            Level4 = 5,
            Level5 = 6
        };

        LinkLogServer(const Address &listen_address,
                      ServerHandlerPtr handler,
                      std::string dictionary_path,
                      PartitionRank partition_rank = Level2);

        ~LinkLogServer();

        void flush() const;

    private:
        friend class LinkLogger;

        struct Buffer {
            Base::Mutex mutex;
            uint32 index = 0;
            uint32 ref_count = 0;
            Base::BufferPool::Buffer buf;
            Base::TimeInterval last_flush_time;
        };

        using BufferQueue = std::vector<Buffer>;
        using BufferIter = BufferQueue::iterator;

        struct NodeMessage {
            Type type = Invalid, subtype = Invalid;
            bool created = false, sub_decision_created = false;
            BufferIter buf_iter;
            Base::TimeInterval init_time;
            std::map<ID, NodeMessage>::iterator parent_iter;
            bool* logger_timout = nullptr;

            NodeMessage(Type t, std::map<ID, NodeMessage>::iterator pi)
                : type(t), parent_iter(pi) {};

            void create() {
                logger_timout = new bool(false);
            };

            void destroy() {
                if (*logger_timout) delete logger_timout;
                else *logger_timout = true;
                logger_timout = nullptr;
            };

            void complete_message(BufferIter iter) {
                created = true;
                buf_iter = iter;
                ++buf_iter->ref_count;
                init_time = Base::Unix_to_now();
            };

            void shutdown() {
                created = false;
                --buf_iter->ref_count;
            };
        };

        using LinkedID = LinkedMark;
        using Map = std::map<ID, NodeMessage>;
        using MapIter = Map::iterator;

        struct TimerData {
            Base::TimeInterval expire_time;
            MapIter iter;
            bool* logger_timout = nullptr;

            explicit TimerData(Base::TimeInterval expire_time, MapIter iter) :
                expire_time(expire_time), iter(iter), logger_timout(iter->second.logger_timout) {};

            TimerData(const TimerData &) = default;

            [[nodiscard]] bool check_timeout() const {
                if (!*logger_timout) {
                    *logger_timout = true;
                    return true;
                }
                delete logger_timout;
                return false;
            };

            friend bool operator<(const TimerData &lhs, const TimerData &rhs) {
                return lhs.expire_time > rhs.expire_time;
            };
        };

        using TimerQueue = std::priority_queue<TimerData>;

        using WaitQueue = std::queue<LinkLogMessage>;

        Base::Mutex _mutex;

        Base::Condition _condition;

        Base::BufferPool _pool;

        Map _nodes;

        Net::Acceptor _acceptor;

        Net::Socket _notifier;

        Net::TcpMessageAgent _receiver, _center;

        BufferQueue _buffers;

        ServerHandlerPtr _handler;

        LinkLogEncoder _encoder;

        TimerQueue _timers;

        Base::Thread _thread;

        bool create_local_link();

        void accept_center_link(Net::Poller &poller);

        void destroy_local_link(Net::Poller &poller);

        void destroy_center_link(Net::Poller &poller);


        [[nodiscard]] bool safe_notify(const LinkLogMessage &message) const;

        void safe_write(BufferIter buf_iter, const void* data, uint32 size);

        void safe_error(BufferIter buf_iter, const ServiceID &service,
                        const NodeID &node, LinkErrorType error);


        void start_thread();

        void init_thread(Net::Poller &poller, std::vector<BufferIter> &flush_order);

        bool accept_message(Net::Poller &poller, std::vector<Net::Event> &events);

        bool handle_local_message(WaitQueue &wait_queue);

        void handle_remote_message(WaitQueue &wait_queue) const;

        void send_center_message(WaitQueue &wait_queue, Net::Poller &poller, bool write_notify);

        void update_center_state(Net::Poller &poller) const;

        void save_logs(WaitQueue &wait_queue);

        void close_thread(WaitQueue &wait_queue, Net::Poller &poller, std::vector<Net::Event> &events);


        LinkErrorType register_logger(MapIter parent_iter, Type type, const NodeID &node_id,
                                      Base::TimeInterval timeout);

        LinkErrorType register_logger(MapIter parent_iter, Type type, const NodeID &node_id,
                                      Base::TimeInterval timeout, BufferIter &buf_iter,
                                      Register_Logger &logger);

        BufferIter get_buffer_iter();

        std::pair<LinkErrorType, MapIter>
        create_head_logger(const ServiceID &service, const NodeID &node,
                           Base::TimeInterval timeout, bool is_branch);

        std::pair<LinkErrorType, MapIter>
        create_head_logger(const ServiceID &service, const NodeID &node,
                           Base::TimeInterval timeout, bool is_branch,
                           BufferIter &buf_iter, Create_Logger &logger);

        std::pair<LinkErrorType, MapIter>
        create_logger(const ServiceID &service, const NodeID &node, Base::TimeInterval timeout);

        std::pair<LinkErrorType, MapIter>
        create_logger(const ServiceID &service, const NodeID &node, Base::TimeInterval timeout,
                      BufferIter &buf_iter, Create_Logger &logger);

        void destroy_logger(MapIter iter);

        void destroy_logger(MapIter iter, BufferIter &buf_iter, End_Logger &logger);

        void submit_buffer(Buffer &buf, Base::Lock<Base::Mutex> &lock);


        bool clear_buffer(LinkLogMessage &message, bool can_send);

        bool logger_timeout(const LinkLogMessage &message) const;

        bool node_offline(LinkLogMessage &message, Net::Poller &poller);

        void force_flush(std::vector<BufferIter> &flush_order, WaitQueue &wait_queue);

        void check_timeout(WaitQueue &wait_queue);

    };

}

#endif

#endif //LOGSYSTEM_LINKLOGSERVERE_HPP
