//
// Created by taganyer on 24-10-26.
//

#ifndef NET_TCPMESSAGEAGENT_HPP
#define NET_TCPMESSAGEAGENT_HPP

#ifdef NET_TCPMESSAGEAGENT_HPP

#include <cerrno>
#include "Socket.hpp"
#include "Base/Buffer/RingBuffer.hpp"
#include "Net/functions/Interface.hpp"

namespace Net {

    class TcpMessageAgent {
    public:
        using Buffer = Base::RingBuffer;

        TcpMessageAgent(Socket &&sock, uint32 input_size, uint32 output_size);

        uint32 read(void* dest, uint32 size, bool fixed);

        template <std::size_t N>
        uint32 read(const Base::BufferArray<N> &dest, bool fixed);

        [[nodiscard]] bool can_receive() const { return input.writable_len() > 0; };

        [[nodiscard]] bool can_send() const { return output.readable_len() > 0; };

        int64 direct_send(const void* data, uint32 size);

        template <std::size_t N>
        int64 direct_send(const Base::BufferArray<N> &data);

        uint32 indirect_send(const void* data, uint32 size, bool fixed);

        template <std::size_t N>
        uint32 indirect_send(const Base::BufferArray<N> &data, bool fixed);

        bool send_message();

        bool receive_message();

        void reset_socket(Socket &&sock);

        void close();

        [[nodiscard]] bool socket_valid() const { return socket.valid(); };

        [[nodiscard]] bool has_hang_up() const { return socket_hang_up; };

        [[nodiscard]] uint32 unread_size() const { return input.readable_len(); };

        [[nodiscard]] uint32 remaining_input_size() const { return input.writable_len(); };

        [[nodiscard]] uint32 unsent_size() const { return output.readable_len(); };

        [[nodiscard]] uint32 remaining_output_size() const { return output.writable_len(); };

        Socket socket;

    private:
        bool socket_hang_up = false;

    public:
        Buffer input, output;

    };

    template <std::size_t N>
    uint32 TcpMessageAgent::read(const Base::BufferArray<N> &dest, bool fixed) {
        if (fixed) {
            uint32 total_size = 0;
            for (auto [ptr, size] : dest) total_size += size;
            if (input.readable_len() < total_size)
                return 0;
        }
        return input.read(dest);
    }

    template <std::size_t N>
    int64 TcpMessageAgent::direct_send(const Base::BufferArray<N> &data) {
        if (can_send())
            return indirect_send(data, false);
        auto result = ops::writev(socket.fd(), data.data(), data.size());
        if (result == 0) {
            socket_hang_up = true;
            return -1;
        }
        if (result < 0) {
            result = 0;
            if (errno != EWOULDBLOCK)
                return -1;
        }
        auto new_data = data + result;
        result += output.write(new_data);
        return result;
    }

    template <std::size_t N>
    uint32 TcpMessageAgent::indirect_send(const Base::BufferArray<N> &data, bool fixed) {
        if (fixed) {
            uint32 total_size = 0;
            for (auto [ptr, size] : data) total_size += size;
            if (output.writable_len() < total_size)
                return 0;
        }
        return output.write(data);
    }

}

#endif

#endif //NET_TCPMESSAGEAGENT_HPP
