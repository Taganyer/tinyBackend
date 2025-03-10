//
// Created by taganyer on 24-10-26.
//

#include "../TcpMessageAgent.hpp"

using namespace Net;

using namespace Base;

TcpMessageAgent::TcpMessageAgent(Socket &&sock, uint32 input_size, uint32 output_size) :
    _socket(std::move(sock)), _input(input_size), _output(output_size) {}

int64 TcpMessageAgent::send_message() {
    assert_thread_safe();
    if (_output.readable_len() == 0)
        return 0;
    auto array = _output.readable_array();
    auto written = ops::writev(fd(), array.data(), array.size());
    if (written < 0) return -1;
    _output.read_advance(written);
    return written;
}

int64 TcpMessageAgent::receive_message() {
    assert_thread_safe();
    if (_input.writable_len() == 0)
        return 0;
    auto array = _input.writable_array();
    auto read = ops::readv(fd(), array.data(), array.size());
    if (read < 0) return -1;
    if (read == 0) socket_event.set_HangUp();
    _input.write_advance(read);
    return read;
}

void TcpMessageAgent::reset_socket(Socket &&sock) {
    close();
    _socket = std::move(sock);
}

void TcpMessageAgent::close() {
    assert_thread_safe();
    _input.read_advance(_input.readable_len());
    _output.read_advance(_output.readable_len());
    _socket.close();
}
