//
// Created by taganyer on 24-3-5.
//

#ifndef BASE_RINGBUFFER_HPP
#define BASE_RINGBUFFER_HPP

#include "Base/Detail/config.hpp"
#include "Base/Detail/NoCopy.hpp"
#include "Base/Detail/BufferArray.hpp"

namespace Base {

    class RingBuffer : NoCopy {
    public:
        explicit RingBuffer(uint32 size = 1024) : _buffer(new char[size]), _size(size) {};

        ~RingBuffer() { delete[] _buffer; };

        void resize(uint32 size);

        void reposition();


        uint32 read(void* dest, uint32 size);

        template <std::size_t N>
        uint32 read(const BufferArray<N> &array);

        uint32 try_read(void* dest, uint32 size, uint32 offset = 0) const;

        template <std::size_t N>
        uint32 try_read(const BufferArray<N> &array, uint32 offset = 0) const;


        uint32 write(const void* target, uint32 size);

        template <std::size_t N>
        uint32 write(const BufferArray<N> &array);

        uint32 try_write(const void* target, uint32 size, uint32 offset = 0);

        template <std::size_t N>
        uint32 try_write(const BufferArray<N> &array, uint32 offset = 0);

        uint32 change_written(uint32 offset, const void* data, uint32 size);

        template <std::size_t N>
        uint32 change_written(uint32 offset, const BufferArray<N> &array);


        void read_advance(uint32 step);

        void read_back(uint32 step);

        void write_advance(uint32 step);

        void write_back(uint32 step);

        [[nodiscard]] char* write_data() { return _write; };

        [[nodiscard]] char* read_data() { return _read; };

        [[nodiscard]] const char* write_data() const { return _write; };

        [[nodiscard]] const char* read_data() const { return _read; };

        [[nodiscard]] const char* begin() const { return _buffer; };

        [[nodiscard]] const char* end() const { return _buffer + _size; };

        [[nodiscard]] uint32 buffer_size() const { return _size; };

        [[nodiscard]] uint32 readable_len() const { return _readable; };

        [[nodiscard]] uint32 writable_len() const { return _size - _readable; };

        [[nodiscard]] uint32 continuously_readable() const {
            auto dis = end() - _read;
            return dis >= _readable ? _readable : dis;
        };

        [[nodiscard]] uint32 continuously_writable() const {
            if (_read > _write) return _read - _write;
            if (_read == _write && _readable != 0) return 0;
            return end() - _write;
        };

        [[nodiscard]] BufferArray<2> read_array(uint32 size, uint32 offset = 0) const;

        [[nodiscard]] BufferArray<2> write_array(uint32 size, uint32 offset = 0) const;

        [[nodiscard]] BufferArray<2> readable_array() const;

        [[nodiscard]] BufferArray<2> writable_array() const;

        [[nodiscard]] bool empty() const { return _read == _write && _readable == 0; };

    private:
        char* _buffer = nullptr;

        char* _read = _buffer;

        char* _write = _buffer;

        uint32 _readable = 0, _size = 0;

    };

}

namespace Base {

    template <std::size_t N>
    uint32 RingBuffer::read(const BufferArray<N> &array) {
        uint32 read = 0;
        for (auto [buf, size] : array) {
            if (size == 0) continue;
            if (readable_len() == 0) break;
            read += read(buf, size);
        }
        return read;
    }

    template <std::size_t N>
    uint32 RingBuffer::try_read(const BufferArray<N> &array, uint32 offset) const {
        uint32 read_size = 0;
        for (auto [buf, size] : array) {
            if (size == 0) continue;
            if (readable_len() <= offset + read_size) break;
            read_size += try_read(buf, size, offset + read_size);
        }
        return read_size;
    }

    template <std::size_t N>
    uint32 RingBuffer::write(const BufferArray<N> &array) {
        uint32 written = 0;
        for (auto [buf, size] : array) {
            if (size == 0) continue;
            if (writable_len() == 0) break;
            written += write(buf, size);
        }
        return written;
    }

    template <std::size_t N>
    uint32 RingBuffer::try_write(const BufferArray<N> &array, uint32 offset) {
        uint32 written = 0;
        for (auto [buf, size] : array) {
            if (size == 0) continue;
            if (writable_len() <= offset + written) break;
            written += try_write(buf, size, offset + written);
        }
        return written;
    }

    template <std::size_t N>
    uint32 RingBuffer::change_written(uint32 offset, const BufferArray<N> &array) {
        uint32 written = 0;
        for (auto [buf, size] : array) {
            if (size == 0) continue;
            if (readable_len() <= offset + written) break;
            written += change_written(offset + written, buf, size);
        }
        return written;
    }

}


#endif //BASE_RINGBUFFER_HPP
