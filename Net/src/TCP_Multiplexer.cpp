//
// Created by taganyer on 25-3-3.
//

#include "../TCP_Multiplexer.hpp"

using namespace Base;

using namespace Net;


TCP_Multiplexer::TCP_Multiplexer(Socket &&socket, AcceptCallback accept_callback,
                                 uint32 input_size, uint32 output_size, uint16 max_channels_size) :
    TcpMessageAgent(std::move(socket), input_size, output_size),
    _channels(max_channels_size, nullptr), _increase_id(max_channels_size, 0),
    _accept_callback(std::move(accept_callback)) {}

TCP_Multiplexer::~TCP_Multiplexer() { TCP_Multiplexer::close(); };

void TCP_Multiplexer::close() {
    Lock l(_mutex);
    if (agent_valid()) {
        if (!_send_queue.empty()) {
            auto head = _send_queue.front();
            _send_queue = std::queue<Header>();
            if (head.op == Null) _send_queue.push(head);
        }
        Header header { Shutdown, 0, 0, FD() };
        _send_queue.push(header);
    }
    while (!_send_queue.empty()) {
        /// 这里可能会出现问题，信息可能会写入不完全。
        if (send_message_unsafe() < 0) break;
    }
    handle_Shutdown();
    TcpMessageAgent::close();
}

int64 TCP_Multiplexer::send_message() {
    Lock l(_mutex);
    return send_message_unsafe();
}

int64 TCP_Multiplexer::receive_message() {
    Lock l(_mutex);
    int64 size = TcpMessageAgent::receive_message();
    uint32 len = _current_header.op == Null ? sizeof(Header) : 0;
    while (_input.readable_len() >= len) {
        if (_current_header.op == Null) {
            uint32 read = _input.read(&_current_header, sizeof(Header));
            assert(read == sizeof(Header));
        }
        switch (_current_header.op) {
        case Sent:
            handle_Sent();
            break;
        case Received:
            handle_Received();
            break;
        case Connect:
            handle_Connect();
            break;
        case Accept:
            handle_Accept();
            break;
        case Reject:
            handle_Reject();
            break;
        case Close:
            handle_Close();
            break;
        case Shutdown:
            handle_Shutdown();
            break;
        default:
            assert(false);
        }
        if (_current_header.extra_size == 0) _current_header = Header();
    }
    return size;
}

void TCP_Multiplexer::add_channel(std::string verify_message, Channel channel, Event monitor_event,
                                  uint32 input_buffer_size, uint32 output_buffer_size,
                                  CreateCallback create_callback, RejectCallback reject_callback) {
    Lock l(_mutex);
    int index = find_empty();
    if (index == -1) {
        if (reject_callback) reject_callback(std::move(verify_message), channel);
        return;
    }

    FD fd(_increase_id[index], index, _socket.fd());
    _channels[index] = new ChannelData(fd, std::move(channel), monitor_event);
    _channels[index]->monitor_event.extra_data = new UnconnectedData(std::move(verify_message),
                                                                     input_buffer_size, output_buffer_size,
                                                                     std::move(create_callback),
                                                                     std::move(reject_callback));
    Header header { Connect, 0, input_buffer_size, fd };
    _send_queue.push(header);
}

void TCP_Multiplexer::update_channel(Event event) {
    Lock l(_mutex);
    int index = event.fd & 0xffff;
    if (!_channels[index]) return;
    auto &channel_data = *_channels[index];
    if (_close_queue.find(channel_data.self) != _close_queue.end()) return;
    channel_data.monitor_event.event = event.event;
}

void TCP_Multiplexer::weak_up_channel(int fd, const WeakUpFun &fun) {
    Lock l(_mutex);
    int index = fd & 0xffff;
    if (!_channels[index]) return;
    auto &channel_data = *_channels[index];
    if (_close_queue.find(channel_data.self) != _close_queue.end()) return;
    fun(channel_data, channel_data.channel);
}

void TCP_Multiplexer::invoke_event() {
    Lock l(_mutex);
    for (auto ptr : _channels) {
        if (!ptr) continue;
        if (ptr->monitor_event.canWrite() && ptr->output_buffer.writable_len() > 0)
            ptr->socket_event.set_write();
        ptr->channel.invoke_event(*ptr);
    }
}

int64 TCP_Multiplexer::ChannelData::receive_message() {
    return 0;
}

int64 TCP_Multiplexer::ChannelData::send_message() {
    auto ptr = (TCP_Multiplexer *) monitor_event.extra_data;
    uint32 writable = ptr->_output.writable_len();
    uint32 size = std::min(other_window, output_buffer.readable_len());
    if (writable < sizeof(Header) + size / 2) return 0;
    size = std::min(size, (uint32) (writable - sizeof(Header)));
    Header header { Sent, size, 0, other };
    uint32 written = ptr->_output.write(&header, sizeof(header));
    assert(written == sizeof(header));
    written = ptr->_output.write(output_buffer.read_array(size));
    assert(written == size);
    other_window -= size;
    output_buffer.read_advance(size);
    return size;
}

void TCP_Multiplexer::ChannelData::close() {
    if (other == FD()) return;
    auto ptr = (TCP_Multiplexer *) monitor_event.extra_data;
    input_buffer.read_advance(input_buffer.readable_len());
    input_buffer.resize(0);
    uint32 need_sent_size = 0;
    if (passive_closer) {
        ptr->remove(self);
    } else {
        active_closer = true;
        ptr->_close_queue.emplace(self);
        need_sent_size = output_buffer.readable_len();
    }
    Header header { Close, need_sent_size, 0, other };
    ptr->_send_queue.push(header);
    other = FD();
}

int64 TCP_Multiplexer::send_message_unsafe() {
    while (!_send_queue.empty() && _output.writable_len() >= sizeof(Header)) {
        auto &header = _send_queue.front();
        if (header.op == Null) {
            auto &buffer = _channels[header.fd.index]->output_buffer;
            uint32 written = _output.write(buffer.read_array(header.extra_size));
            buffer.read_advance(written);
            header.extra_size -= written;
        } else {
            if (header.op == Sent || header.op == Connect || header.op == Close)
                header.op = Null;
            uint32 written = _output.write(&header, sizeof(header));
            assert(written == sizeof(header));
        }
        if (header.extra_size == 0) {
            _send_queue.pop();
        }
    }
    return TcpMessageAgent::send_message();
}

void TCP_Multiplexer::handle_Sent() {
    ChannelData &channel_data = *_channels[_current_header.fd.index];
    assert(channel_data.self == _current_header.fd);
    if (channel_data.input_buffer.buffer_size() != 0) {
        uint32 read = _input.read(channel_data.input_buffer.write_array(_current_header.extra_size));
        channel_data.input_buffer.write_advance(read);
        _current_header.extra_size -= read;
        _current_header.windows_size += read;
        channel_data.other_window += read;
        if (_current_header.extra_size == 0) {
            Header header { Received, 0, _current_header.windows_size, channel_data.other };
            _send_queue.push(header);
        }
        if (channel_data.monitor_event.canRead()) channel_data.socket_event.set_read();
    } else {
        /// 抛弃消息
        auto read = std::min(_input.readable_len(), _current_header.extra_size);
        _input.read_advance(read);
        _current_header.extra_size -= read;
        _current_header.windows_size += read;
        if (_current_header.extra_size == 0) {
            Header header { Received, 0, _current_header.windows_size, channel_data.other };
            _send_queue.push(header);
        }
    }
}

void TCP_Multiplexer::handle_Received() const {
    ChannelData &channel_data = *_channels[_current_header.fd.index];
    assert(channel_data.self == _current_header.fd);
    channel_data.other_window += _current_header.windows_size;
}

void TCP_Multiplexer::handle_Connect() {
    if (_input.readable_len() >= _current_header.extra_size) {
        std::string verify_message(_current_header.extra_size, '\0');
        _current_header.extra_size -= _input.read(verify_message.data(), _current_header.extra_size);
        assert(_current_header.extra_size == 0);

        int index = find_empty();
        bool can_accept = false;
        uint32 input_buffer_size = 1024, output_buffer_size = 1024;
        Channel channel = _accept_callback(std::move(verify_message),
                                           index == -1, can_accept,
                                           input_buffer_size, output_buffer_size);
        if (index == -1) can_accept = false;
        if (can_accept) {
            FD fd(_increase_id[index], index, _socket.fd());
            _channels[index] = new ChannelData(fd, _current_header.fd, _current_header.windows_size,
                                               std::move(channel), input_buffer_size, output_buffer_size);
            _channels[index]->monitor_event.extra_data = this;
            Header header { Accept, 0, input_buffer_size, _current_header.fd };
            _send_queue.push(header);
        } else {
            Header header { Reject, 0, 0, _current_header.fd };
            _send_queue.push(header);
        }
    } else {
        assert(_input.buffer_size() == _input.readable_len());
    }
}

void TCP_Multiplexer::handle_Accept() {
    ChannelData &channel_data = *_channels[_current_header.fd.index];
    assert(channel_data.self == _current_header.fd);
    auto data = (UnconnectedData *) channel_data.monitor_event.extra_data;
    channel_data.other_window = _current_header.windows_size;
    channel_data.input_buffer.resize(data->input_buffer_size);
    channel_data.output_buffer.resize(data->output_buffer_size);
    data->create_callback(channel_data, channel_data.channel);
    delete data;
    channel_data.monitor_event.extra_data = this;
}

void TCP_Multiplexer::handle_Reject() {
    ChannelData &channel_data = *_channels[_current_header.fd.index];
    assert(channel_data.self == _current_header.fd);
    auto data = (UnconnectedData *) channel_data.monitor_event.extra_data;
    data->reject_callback(std::move(data->verify_message), channel_data.channel);
    delete data;
    remove(_current_header.fd);
}

void TCP_Multiplexer::handle_Close() {
    ChannelData &channel_data = *_channels[_current_header.fd.index];
    assert(channel_data.self == _current_header.fd);
    channel_data.passive_closer = true;
    if (_current_header.extra_size != 0) {
        if (channel_data.input_buffer.writable_len() < _current_header.extra_size) {
            /// 此处可能会消耗大量内存。
            channel_data.input_buffer.resize(_current_header.extra_size
                + channel_data.input_buffer.readable_len());
        }
        uint32 read = _input.read(channel_data.input_buffer.write_array(_current_header.extra_size));
        _current_header.extra_size -= read;
    }
    if (_current_header.extra_size != 0) return;
    if (!channel_data.active_closer) {
        channel_data.socket_event.set_HangUp();
        channel_data.channel.invoke_event(channel_data);
    } else {
        remove(channel_data.self);
    }
}

void TCP_Multiplexer::handle_Shutdown() {
    for (auto ptr : _channels) {
        if (!ptr) continue;
        ChannelData &channel_data = *ptr;
        if (_close_queue.find(channel_data.self) != _close_queue.end()) {
            remove(channel_data.self);
            continue;
        }
        channel_data.socket_event.set_error();
        channel_data.other = FD();
        channel_data.error = { error_types::UnexpectedShutdown, channel_data.fd() };
        channel_data.channel.invoke_event(channel_data);
        /// 当用户没有关闭时，强行关闭。
        if (channel_data.agent_valid()) {
            channel_data.socket_event.set_HangUp();
            channel_data.channel.invoke_event(channel_data);
        }
        remove(channel_data.self);
    }
    _close_queue.clear();
    _output.read_advance(_output.readable_len());
}

void TCP_Multiplexer::remove(FD fd) {
    ChannelData &channel_data = *_channels[fd.index];
    assert(channel_data.self == fd);
    if (channel_data.active_closer) {
        auto success = _close_queue.erase(fd);
        assert(success);
    }
    delete _channels[fd.index];
    _channels[fd.index] = nullptr;
}

int TCP_Multiplexer::find_empty() {
    for (int i = 0; i < _channels.size(); ++i) {
        if (!_channels[i]) {
            if (_increase_id[i] == MAX_USHORT) _increase_id[i] = 0;
            else ++_increase_id[i];
            return i;
        }
    }
    return -1;
}
