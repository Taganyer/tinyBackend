//
// Created by taganyer on 24-10-26.
//

#ifndef NET_TCPMESSAGEAGENT_HPP
#define NET_TCPMESSAGEAGENT_HPP

#ifdef NET_TCPMESSAGEAGENT_HPP


#include "Socket.hpp"
#include "MessageAgent.hpp"
#include "Base/Buffer/RingBuffer.hpp"
#include "Net/functions/Interface.hpp"

namespace Net {

    class TcpMessageAgent : public MessageAgent {
    public:
        using Buffer = Base::RingBuffer;

        TcpMessageAgent(Socket &&sock, uint32 input_size, uint32 output_size);

        ~TcpMessageAgent() override = default;

        [[nodiscard]] uint32 can_receive() const override { return _input.writable_len(); };

        [[nodiscard]] uint32 can_send() const override { return _output.readable_len(); };

        int64 send_message() override;

        int64 receive_message() override;

        void reset_socket(Socket &&sock);

        void close() override;

        const Base::InputBuffer& input() const override {
            assert_thread_safe();
            return _input;
        };

        const Base::OutputBuffer& output() const override {
            assert_thread_safe();
            return _output;
        };

        const Socket& get_socket() const {
            assert_thread_safe();
            return _socket;
        };

        [[nodiscard]] int fd() const override { return _socket.fd(); };

        [[nodiscard]] bool agent_valid() const override { return _socket.valid(); };

        [[nodiscard]] uint32 unread_size() const { return _input.readable_len(); };

        [[nodiscard]] uint32 remaining_input_size() const { return _input.writable_len(); };

        [[nodiscard]] uint32 unsent_size() const { return _output.readable_len(); };

        [[nodiscard]] uint32 remaining_output_size() const { return _output.writable_len(); };

    protected:
        Socket _socket;

        Buffer _input, _output;

    };

}

#endif

#endif //NET_TCPMESSAGEAGENT_HPP
