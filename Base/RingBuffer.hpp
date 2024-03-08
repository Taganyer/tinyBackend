//
// Created by taganyer on 24-3-5.
//

#ifndef BASE_RINGBUFFER_HPP
#define BASE_RINGBUFFER_HPP

#include "Detail/config.hpp"
#include "Detail/NoCopy.hpp"
#include <string_view>

namespace Base {

    class RingBuffer : NoCopy {
    public:

        using View = std::string_view;

        using ViewPair = std::pair<View, View>;

        RingBuffer() = default;

        explicit RingBuffer(uint32 size = 1024) : _buffer(new char[size]), _size(size) {};

        ~RingBuffer() { delete[] _buffer; };

        void resize(uint32 size);

        void reposition();

        uint32 write(const char *target, uint32 size);

        char *write_data() { return _write; };

        void write_advance(uint32 step);

        uint32 read_to(char *dest, uint32 size);

        [[nodiscard]] const char *read_data() const { return _read; };

        void read_advance(uint32 step);

        inline char peek();

        [[nodiscard]] ViewPair get_view() const;

        [[nodiscard]] uint32 buffer_size() const { return _size; };

        [[nodiscard]] uint32 readable_len() const { return _readable; };

        [[nodiscard]] uint32 writable_len() const { return _size - _readable; };

        [[nodiscard]] uint32 continuously_readable() const {
            auto dis = end() - _read;
            return dis >= _readable ? _readable : _readable - dis;
        };

        [[nodiscard]] uint32 continuously_writable() const { return end() - _write; };

        [[nodiscard]] bool empty() const { return _read == _write && _readable == 0; };

    private:

        char *_buffer = nullptr;

        char *_read = _buffer;

        char *_write = _buffer;

        uint32 _readable = 0, _size = 0;

        inline void read_move(uint32 step);

        inline void write_move(uint32 step);

        [[nodiscard]] const char *end() const { return _buffer + _size; };

    };

}

namespace Base {

    char Base::RingBuffer::peek() {
        if (!empty()) {
            char t = *_write;
            write_move(1);
            return t;
        }
        return -1;
    }

    void Base::RingBuffer::read_move(uint32 step) {
        _read += step;
        if (_read >= end())
            _read -= _size;
        _readable -= step;
    }

    void Base::RingBuffer::write_move(uint32 step) {
        _write += step;
        if (_write >= end())
            _write -= _size;
    }

}


#endif //BASE_RINGBUFFER_HPP
