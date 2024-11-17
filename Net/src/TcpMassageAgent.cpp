//
// Created by taganyer on 24-10-26.
//

#include <cerrno>
#include "../TcpMessageAgent.hpp"

using namespace Net;

using namespace Base;

TcpMessageAgent::TcpMessageAgent(Socket &&sock, uint32 input_size, uint32 output_size) :
    socket(std::move(sock)), input(input_size), output(output_size) {}

uint32 TcpMessageAgent::read(void* dest, uint32 size, bool fixed) {
    if (fixed && input.readable_len() < size)
        return 0;
    return input.read(dest, size);
}

int64 TcpMessageAgent::direct_send(const void* data, uint32 size) {
    if (can_send())
        return indirect_send(data, size, false);
    auto result = ops::write(socket.fd(), data, size);
    if (result == 0) {
        socket_hang_up = true;
        return -1;
    }
    if (result < 0) {
        result = 0;
        if (errno != EWOULDBLOCK)
            return -1;
    }
    if (result < size)
        result += output.write((const char *)data + result, size - result);
    return result;
}

uint32 TcpMessageAgent::indirect_send(const void* data, uint32 size, bool fixed) {
    if (fixed && output.writable_len() < size)
        return 0;
    return output.write(data, size);
}

bool TcpMessageAgent::send_message() {
    if (output.readable_len() == 0)
        return true;
    auto array = output.readable_array();
    auto written = ops::writev(socket.fd(), array.data(), array.size());
    if (written < 0)
        return false;
    output.read_advance(written);
    return true;
}

bool TcpMessageAgent::receive_message() {
    if (input.writable_len() == 0)
        return true;
    auto array = input.writable_array();
    auto read = ops::readv(socket.fd(), array.data(), array.size());
    if (read < 0)
        return false;
    if (read == 0) {
        socket_hang_up = true;
        return false;
    }
    input.write_advance(read);
    return true;
}

void TcpMessageAgent::reset_socket(Socket&& sock) {
    socket = std::move(sock);
    socket_hang_up = false;
}

void TcpMessageAgent::close() {
    socket_hang_up = false;
    socket.close();
}
