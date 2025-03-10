//
// Created by taganyer on 25-3-3.
//

#ifndef NET_TCP_MULTIPLEXER_HPP
#define NET_TCP_MULTIPLEXER_HPP

#ifdef NET_TCP_MULTIPLEXER_HPP

#include <queue>
#include <vector>
#include "Socket.hpp"
#include "Channel.hpp"
#include "Base/Mutex.hpp"
#include "TcpMessageAgent.hpp"

namespace Net {

    /// 所有函数调用线程安全。
    /// 注意潜在的死锁问题，如果一个 Channel 不能及时读取处理数据，将会阻塞其他所有 Channel 的数据读取。
    class TCP_Multiplexer : public TcpMessageAgent {
    public:
        static constexpr uint64 FAIL = MAX_ULLONG;

        using AcceptCallback = std::function<Channel(std::string verify_string,
                                                     bool has_reject, bool &can_accept,
                                                     uint32 &input_buffer_size,
                                                     uint32 &output_buffer_size)>;

        using CreateCallback = std::function<void(MessageAgent &, Channel &)>;

        using RejectCallback = std::function<void(std::string verify_string, Channel &channel)>;

        using WeakUpFun = std::function<void(MessageAgent &, Channel &)>;

        TCP_Multiplexer(Socket &&socket, AcceptCallback accept_callback,
                        uint32 input_size, uint32 output_size, uint16 max_channels_size);

        ~TCP_Multiplexer() override;

        void close() override;

        int64 send_message() override;

        int64 receive_message() override;

        void add_channel(std::string verify_message, Channel channel, Event monitor_event,
                         uint32 input_buffer_size, uint32 output_buffer_size,
                         CreateCallback create_callback = nullptr,
                         RejectCallback reject_callback = nullptr);

        void update_channel(Event event);

        void weak_up_channel(int fd, const WeakUpFun &fun);

        /// error_event 和 hang_up_event 可能会在 send_message 和 receive_message 调用。
        /// 会轮询整个数组，调用有事件发生的 Channel。
        void invoke_event();

    private:
        struct FD {
            FD() = default;

            FD(const FD &) = default;

            FD(uint16 id, uint16 index, uint32 socket_fd) :
                id(id), index(index), socket_fd(socket_fd) {};

            friend bool operator==(FD lhs, FD rhs) {
                return lhs.id == rhs.id && lhs.index == rhs.index && lhs.socket_fd == rhs.socket_fd;
            };

            friend bool operator!=(FD lhs, FD rhs) {
                return !(lhs == rhs);
            };

            friend bool operator<(FD lhs, FD rhs) {
                if (lhs.id != rhs.id) return lhs.id < rhs.id;
                return lhs.index < rhs.index;
            };

            uint16 id = 0, index = 0;
            uint32 socket_fd = 0;
        };

        struct ChannelData;

        enum Operation {
            Null,
            Sent,
            Received,
            Connect,
            Accept,
            Reject,
            Close,
            Shutdown
        };

        struct Header {
            Operation op = Null;
            uint32 extra_size = 0;
            uint32 windows_size = 0;
            FD fd;
        };

        Base::ReentrantMutex<Base::Mutex> _mutex;

        std::queue<Header> _send_queue;

        Header _current_header;

        std::vector<ChannelData *> _channels;

        std::vector<uint16> _increase_id;

        AcceptCallback _accept_callback;

        void handle_Sent();

        void handle_Received() const;

        void handle_Connect();

        void handle_Accept();

        void handle_Reject();

        void handle_Close();

        void handle_Shutdown();

        void remove(FD fd);

        int find_empty();

    };

}

#endif

#endif //NET_TCP_MULTIPLEXER_HPP
