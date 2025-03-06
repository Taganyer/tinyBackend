//
// Created by taganyer on 25-3-3.
//

#ifndef BASE_RINGBUFFERWRAPPER_HPP
#define BASE_RINGBUFFERWRAPPER_HPP

#ifdef BASE_RINGBUFFERWRAPPER_HPP

#include "InputBuffer.hpp"
#include "OutputBuffer.hpp"

namespace Base {

    class RingBufferWrapper : public InputBuffer, public OutputBuffer {
    public:
        RingBufferWrapper(char* buf, uint32 size) :
            InputBuffer(buf), OutputBuffer(buf), _buffer(buf), _size(size) {};

        RingBufferWrapper(RingBufferWrapper &&other) noexcept :
            InputBuffer(std::move(other)), OutputBuffer(std::move(other)),
            _buffer(other._buffer), _readable(other._readable), _size(other._size) {
            other._buffer = nullptr;
            other._readable = other._size = 0;
        };

        uint32 read(void* dest, uint32 size) const override;

        uint32 read(uint32 N, const iovec* array) const override;

        uint32 read(const OutputBuffer& buffer, uint32 size) const override;

        uint32 fix_read(void* dest, uint32 size) const override {
            if (readable_len() < size) return 0;
            return read(dest, size);
        };

        uint32 fix_read(uint32 N, const iovec* array) const override {
            uint64 size = 0;
            for (uint32 i = 0; i < N; ++i) size += array[i].iov_len;
            if (readable_len() < size) return 0;
            return read(N, array);
        };

        uint32 fix_read(const OutputBuffer& buffer, uint32 size) const override {
            if (readable_len() < size) return 0;
            return read(buffer, size);
        };

        template <std::size_t N>
        uint32 read(const BufferArray<N> &array) const {
            return read(N, array.data());
        };

        template <std::size_t N>
        uint32 fix_read(const BufferArray<N> &array) const {
            return fix_read(N, array.data());
        };

        uint32 try_read(void* dest, uint32 size, uint32 offset) const override;

        uint32 try_read(uint32 N, const iovec* array, uint32 offset) const override;

        template <std::size_t N>
        uint32 try_read(const BufferArray<N> &array, uint32 offset = 0) const {
            return try_read(N, array.data(), offset);
        };

        uint32 write(const void* target, uint32 size) const override;

        uint32 write(uint32 N, const iovec* array) const override;

        uint32 write(const InputBuffer& buffer, uint32 size) const override;

        uint32 fix_write(const void* target, uint32 size) const override {
            if (writable_len() < size) return 0;
            return write(target, size);
        };

        uint32 fix_write(uint32 N, const iovec* array) const override {
            uint64 size = 0;
            for (uint32 i = 0; i < N; ++i) size += array[i].iov_len;
            if (writable_len() < size) return 0;
            return write(N, array);
        };

        uint32 fix_write(const InputBuffer& buffer, uint32 size) const override {
            if (writable_len() < size) return 0;
            return write(buffer, size);
        };

        template <std::size_t N>
        uint32 write(const BufferArray<N> &array) const {
            return write(N, array.data());
        };

        template <std::size_t N>
        uint32 fix_write(const BufferArray<N> &array) const {
            return fix_write(N, array.data());
        };

        uint32 try_write(const void* target, uint32 size, uint32 offset) const override;

        uint32 try_write(uint32 N, const iovec* array, uint32 offset) const override;

        template <std::size_t N>
        uint32 try_write(const BufferArray<N> &array, uint32 offset = 0) const {
            return try_write(array.data(), N, offset);
        };

        uint32 change_written(uint32 offset, const void* data, uint32 size) const;

        uint32 change_written(uint32 offset, uint32 N, const iovec* array) const;

        template <std::size_t N>
        uint32 change_written(uint32 offset, const BufferArray<N> &array) const {
            return change_written(offset, N, array.data());
        };

        void clear_input() const override {
            read_advance(readable_len());
        };

        void clear_output() const override {
            read_advance(readable_len());
        };

        void read_advance(uint32 step) const;

        void read_back(uint32 step) const;

        void write_advance(uint32 step) const;

        void write_back(uint32 step) const;

        void reposition() const;

        [[nodiscard]] uint32 buffer_size() const { return _size; };

        [[nodiscard]] uint32 readable_len() const override { return _readable; };

        [[nodiscard]] uint32 writable_len() const override { return _size - _readable; };

        [[nodiscard]] const char* begin() const { return _buffer; };

        [[nodiscard]] const char* end() const { return _buffer + _size; };

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

    protected:
        char* resize(char* buf, uint32 size);

    private:
        char* _buffer = nullptr;
        mutable uint32 _readable = 0;
        uint32 _size = 0;
    };

}

#endif

#endif //BASE_RINGBUFFERWRAPPER_HPP
