//
// Created by taganyer on 25-3-3.
//

#ifndef NET_TCP_MULTIPLEXER_HPP
#define NET_TCP_MULTIPLEXER_HPP

#ifdef NET_TCP_MULTIPLEXER_HPP

#include <set>
#include <queue>
#include <vector>
#include "Socket.hpp"
#include "Channel.hpp"
#include "Base/Mutex.hpp"
#include "TcpMessageAgent.hpp"

namespace Net {

    /// 所有函数调用线程安全。
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

        struct UnconnectedData {
            UnconnectedData(std::string verify_message,
                            uint32 input_buffer_size,
                            uint32 output_buffer_size,
                            CreateCallback create_callback,
                            RejectCallback reject_callback) :
                verify_message(std::move(verify_message)),
                input_buffer_size(input_buffer_size),
                output_buffer_size(output_buffer_size),
                create_callback(std::move(create_callback)),
                reject_callback(std::move(reject_callback)) {};

            std::string verify_message;
            uint32 input_buffer_size, output_buffer_size;
            CreateCallback create_callback;
            RejectCallback reject_callback;
        };

        struct ChannelData : MessageAgent {
            explicit ChannelData(FD fd, Channel &&ch, Event event) :
                self(fd), channel(std::move(ch)), monitor_event(event) {};

            explicit ChannelData(FD fd, FD other, uint32 window, Channel &&ch,
                                 uint32 input_size, uint32 output_size) :
                self(fd), other(other), other_window(window), channel(std::move(ch)),
                input_buffer(input_size), output_buffer(output_size) {};

            int64 receive_message() override;

            int64 send_message() override;

            void close() override;

            [[nodiscard]] const Base::InputBuffer& input() const override { return input_buffer; };

            [[nodiscard]] const Base::OutputBuffer& output() const override { return output_buffer; };

            /// 这里的 fd 无法注册到 Reactor 中。
            [[nodiscard]] int fd() const override { return (self.id << 16) + self.index; };

            [[nodiscard]] bool agent_valid() const override { return other != FD(); };

            [[nodiscard]] uint32 can_receive() const override { return input_buffer.writable_len(); };

            [[nodiscard]] uint32 can_send() const override { return output_buffer.readable_len(); };

            FD self, other;
            uint32 other_window = 0;
            bool active_closer = false, passive_closer = false;
            Channel channel;
            Base::RingBuffer input_buffer { 0 }, output_buffer { 0 };
            Event monitor_event;
        };

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

        Base::Mutex _mutex;

        std::set<FD> _close_queue;

        std::queue<Header> _send_queue;

        Header _current_header;

        std::vector<ChannelData *> _channels;

        std::vector<uint16> _increase_id;

        AcceptCallback _accept_callback;

        int64 send_message_unsafe();

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
