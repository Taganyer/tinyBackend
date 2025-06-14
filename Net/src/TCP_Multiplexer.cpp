//
// Created by taganyer on 25-3-3.
//

#include "../TCP_Multiplexer.hpp"

using namespace Base;

using namespace Net;

struct UnconnectedData {
    UnconnectedData(std::string verify_message,
                    uint32 input_buffer_size,
                    uint32 output_buffer_size,
                    TCP_Multiplexer::CreateCallback create_callback,
                    TCP_Multiplexer::RejectCallback reject_callback) :
        verify_message(std::move(verify_message)),
        input_buffer_size(input_buffer_size),
        output_buffer_size(output_buffer_size),
        create_callback(std::move(create_callback)),
        reject_callback(std::move(reject_callback)) {};

    std::string verify_message;
    uint32 input_buffer_size, output_buffer_size;
    TCP_Multiplexer::CreateCallback create_callback;
    TCP_Multiplexer::RejectCallback reject_callback;
};

struct TCP_Multiplexer::ChannelData : MessageAgent {
    explicit ChannelData(FD fd, Channel&& ch, Event event) :
        self(fd), channel(std::move(ch)), monitor_event(event) {};

    explicit ChannelData(FD fd, FD other, uint32 window, Channel&& ch,
                         uint32 input_size, uint32 output_size) :
        self(fd), other(other), other_window(window), channel(std::move(ch)),
        input_buffer(input_size), output_buffer(output_size) {};

    int64 receive_message() override;

    int64 send_message() override;

    void close() override;

    [[nodiscard]] const InputBuffer& input() const override {
        assert_thread_safe();
        return input_buffer;
    };

    [[nodiscard]] const OutputBuffer& output() const override {
        assert_thread_safe();
        return output_buffer;
    };

    /// 这里的 fd 无法注册到 Reactor 中。
    [[nodiscard]] int fd() const override { return static_cast<int>((uint32)(self.id << 16) + self.index); };

    [[nodiscard]] bool agent_valid() const override { return other != FD(); };

    [[nodiscard]] uint32 can_receive() const override { return input_buffer.writable_len(); };

    [[nodiscard]] uint32 can_send() const override { return output_buffer.readable_len(); };

    TCP_Multiplexer* get_multiplexer() const {
        return (TCP_Multiplexer *) monitor_event.extra_data;
    };

    UnconnectedData* get_unconnected_data() const {
        return (UnconnectedData *) monitor_event.extra_data;
    }

    FD self, other;
    uint32 other_window = 0;
    bool active_closer = false, passive_closer = false;
    Channel channel;
    RingBuffer input_buffer { 0 }, output_buffer { 0 };
    Event monitor_event;
};

int64 TCP_Multiplexer::ChannelData::receive_message() {
    assert_thread_safe();
    return 0;
}

int64 TCP_Multiplexer::ChannelData::send_message() {
    assert_thread_safe();
    TCP_Multiplexer *ptr = get_multiplexer();
    uint32 writable = ptr->_output.writable_len();
    uint32 size = std::min(other_window, output_buffer.readable_len());
    if (writable < sizeof(Header) + size / 2) return 0;
    size = std::min(size, (uint32)(writable - sizeof(Header)));
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
    assert_thread_safe();
    if (other == FD()) return;
    TCP_Multiplexer *ptr = get_multiplexer();
    input_buffer.read_advance(input_buffer.readable_len());
    input_buffer.resize(0);
    uint32 need_sent_size = 0;
    if (passive_closer) {
        ptr->remove(self);
    } else {
        active_closer = true;
        need_sent_size = output_buffer.readable_len();
    }
    Header header { Close, need_sent_size, 0, other };
    ptr->_send_queue.push(header);
    other = FD();
}

TCP_Multiplexer::TCP_Multiplexer(Socket&& socket, AcceptCallback accept_callback,
                                 uint32 input_size, uint32 output_size, uint16 max_channels_size) :
    TcpMessageAgent(std::move(socket), input_size, output_size),
    _channels(max_channels_size, nullptr), _increase_id(max_channels_size, 1 << 15),
    _accept_callback(std::move(accept_callback)) {
    assert(_accept_callback);
    assert(input_size > 0 && output_size > 0);
    assert(max_channels_size > 0);
}

TCP_Multiplexer::~TCP_Multiplexer() { TCP_Multiplexer::close(); }

void TCP_Multiplexer::close() {
    Lock l(_mutex);
    set_running_thread();
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
        if (send_message() < 0) break;
    }
    handle_Shutdown();
    TcpMessageAgent::close();
}

int64 TCP_Multiplexer::send_message() {
    Lock l(_mutex);
    set_running_thread();
    while (!_send_queue.empty() && _output.writable_len() >= sizeof(Header)) {
        auto& header = _send_queue.front();
        if (header.op == Null) {
            auto& buffer = _channels[header.fd.index]->output_buffer;
            uint32 written = _output.write(buffer.read_array(header.extra_size));
            buffer.read_advance(written);
            header.extra_size -= written;
        } else if (header.op == Connect) {
            auto& channel_data = *_channels[header.fd.index];
            auto ptr = channel_data.get_unconnected_data();
            if (_output.writable_len() < sizeof(header) + ptr->verify_message.size())
                break;
            uint32 written = _output.write(&header, sizeof(header));
            assert(written == sizeof(header));
            written = _output.write(ptr->verify_message.data(), ptr->verify_message.size());
            assert(written == ptr->verify_message.size());
            header.extra_size = 0;
        } else {
            if (header.op == Sent || header.op == Close)
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

int64 TCP_Multiplexer::receive_message() {
    Lock l(_mutex);
    set_running_thread();
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
    set_running_thread();
    assert(verify_message.size() + sizeof(Header) <= _output.buffer_size());
    int index = find_empty();
    if (index == -1) {
        if (reject_callback) reject_callback(std::move(verify_message), channel);
        return;
    }

    FD fd(_increase_id[index], index, _socket.fd());
    auto data = new UnconnectedData(std::move(verify_message),
                                    input_buffer_size, output_buffer_size,
                                    std::move(create_callback),
                                    std::move(reject_callback));
    _channels[index] = new ChannelData(fd, std::move(channel), monitor_event);
    _channels[index]->monitor_event.extra_data = data;
    Header header { Connect, (uint32) data->verify_message.size(), input_buffer_size, fd };
    _send_queue.push(header);
}

void TCP_Multiplexer::update_channel(Event event) {
    Lock l(_mutex);
    set_running_thread();
    int index = event.fd & 0xffff;
    if (!_channels[index]) return;
    auto& channel_data = *_channels[index];
    if (channel_data.active_closer || channel_data.passive_closer) return;
    channel_data.monitor_event.event = event.event;
}

void TCP_Multiplexer::weak_up_channel(int fd, const WeakUpFun& fun) {
    Lock l(_mutex);
    set_running_thread();
    int index = fd & 0xffff;
    if (!_channels[index]) return;
    auto& channel_data = *_channels[index];
    if (channel_data.active_closer || channel_data.passive_closer) return;
    fun(channel_data, channel_data.channel);
}

void TCP_Multiplexer::invoke_event() {
    Lock l(_mutex);
    set_running_thread();
    for (auto ptr : _channels) {
        if (!ptr) continue;
        if (ptr->monitor_event.canWrite() && ptr->output_buffer.writable_len() > 0)
            ptr->socket_event.set_write();
        ptr->set_running_thread();
        ptr->channel.invoke_event(*ptr);
    }
}

void TCP_Multiplexer::handle_Sent() {
    ChannelData& channel_data = *_channels[_current_header.fd.index];
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
    ChannelData& channel_data = *_channels[_current_header.fd.index];
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
    ChannelData& channel_data = *_channels[_current_header.fd.index];
    assert(channel_data.self == _current_header.fd);
    auto data = channel_data.get_unconnected_data();
    channel_data.other_window = _current_header.windows_size;
    channel_data.input_buffer.resize(data->input_buffer_size);
    channel_data.output_buffer.resize(data->output_buffer_size);
    data->create_callback(channel_data, channel_data.channel);
    delete data;
    channel_data.monitor_event.extra_data = this;
}

void TCP_Multiplexer::handle_Reject() {
    ChannelData& channel_data = *_channels[_current_header.fd.index];
    assert(channel_data.self == _current_header.fd);
    auto data = channel_data.get_unconnected_data();
    data->reject_callback(std::move(data->verify_message), channel_data.channel);
    delete data;
    remove(_current_header.fd);
}

void TCP_Multiplexer::handle_Close() {
    ChannelData& channel_data = *_channels[_current_header.fd.index];
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
        ChannelData& channel_data = *ptr;
        if (channel_data.active_closer || channel_data.passive_closer) {
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
    _output.read_advance(_output.readable_len());
}

void TCP_Multiplexer::remove(FD fd) {
    ChannelData& channel_data = *_channels[fd.index];
    assert(channel_data.self == fd);
    delete _channels[fd.index];
    _channels[fd.index] = nullptr;
}

int TCP_Multiplexer::find_empty() {
    for (int i = 0; i < _channels.size(); ++i) {
        if (!_channels[i]) {
            if (_increase_id[i] == MAX_USHORT) _increase_id[i] = 1 << 15;
            else ++_increase_id[i];
            return i;
        }
    }
    return -1;
}
