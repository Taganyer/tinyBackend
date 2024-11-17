//
// Created by taganyer on 24-3-5.
//

#include "../RingBuffer.hpp"
#include <cstring>
#include <cassert>

void Base::RingBuffer::resize(uint32 size) {
    if (size < _readable) size = _readable;
    auto* t = new char[size];
    if (_read + _readable <= end()) {
        std::memcpy(t, _read, _readable);
    } else {
        auto len = end() - _read, another = _readable - len;
        std::memcpy(t, _read, len);
        std::memcpy(t + len, _buffer, another);
    }
    delete[] _buffer;
    _buffer = t;
    _read = _buffer;
    _write = _buffer + _readable;
    _size = size;
}

void Base::RingBuffer::reposition() {
    if (_read == _buffer) return;
    if (_read + _readable <= end()) {
        std::memmove(_buffer, _read, _readable);
    } else {
        auto len = end() - _read, another = _readable - len;
        if (len < another) {
            auto* t = new char[len];
            std::memcpy(t, _read, len);
            std::memcpy(_buffer + len, _buffer, another);
            std::memcpy(_buffer, t, len);
            delete[] t;
        } else {
            auto* t = new char[another];
            std::memcpy(t, _buffer, another);
            std::memcpy(_buffer, _read, len);
            std::memcpy(_buffer + len, t, another);
            delete[] t;
        }
    }
    _read = _buffer;
    _write = _read + _readable;
}

uint32 Base::RingBuffer::read(void* dest, uint32 size) {
    if (size > _readable) size = _readable;
    if (auto len = end() - _read; len >= size) {
        std::memcpy(dest, _read, size);
    } else {
        auto p = (char *) dest + len;
        std::memcpy(dest, _read, len);
        std::memcpy(p, _buffer, size - len);
    }
    read_advance(size);
    return size;
}

uint32 Base::RingBuffer::try_read(void* dest, uint32 size, uint32 offset) const {
    if (offset >= readable_len()) return 0;
    if (offset + size > readable_len())
        size = readable_len() - offset;
    const char* read_record = _read + offset;
    if (read_record >= end()) read_record -= _size;

    if (auto len = end() - read_record; len >= size) {
        std::memcpy(dest, read_record, size);
    } else {
        auto p = (char *) dest + len;
        std::memcpy(dest, read_record, len);
        std::memcpy(p, _buffer, size - len);
    }
    return size;
}

uint32 Base::RingBuffer::write(const void* target, uint32 size) {
    uint32 writable = writable_len();
    if (size > writable) size = writable;
    if (_write + size <= end()) {
        std::memcpy(_write, target, size);
    } else {
        auto len = end() - _write;
        auto p = (const char *) target + len;
        std::memcpy(_write, target, len);
        std::memcpy(_buffer, p, size - len);
    }
    write_advance(size);
    return size;
}

uint32 Base::RingBuffer::try_write(const void* target, uint32 size, uint32 offset) {
    if (offset >= writable_len()) return 0;
    if (offset + size > writable_len()) size = writable_len() - offset;
    if (offset >= continuously_writable()) {
        std::memcpy(_buffer + offset - continuously_writable(), target, size);
    } else {
        auto len = end() - _write - offset;
        if (len > size) len = size;
        std::memcpy(_write + offset, target, len);
        auto p = (const char *) target + len;
        std::memcpy(_buffer, p, size - len);
    }
    return size;
}

uint32 Base::RingBuffer::change_written(uint32 offset, const void* data, uint32 size) {
    if (offset > readable_len()) return 0;
    if (offset + size > readable_len())
        size = readable_len() - offset;
    char* read_record = _read + offset;
    if (read_record >= end()) read_record -= _size;
    if (auto len = end() - read_record; len >= size) {
        std::memcpy(read_record, data, size);
    } else {
        auto p = (char *) data + len;
        std::memcpy(read_record, data, len);
        std::memcpy(_buffer, p, size - len);
    }
    return size;
}

Base::BufferArray<2> Base::RingBuffer::read_array(uint32 size, uint32 offset) const {
    if (offset >= readable_len()) return { iovec { _read, 0 }, iovec { _buffer, 0 } };
    if (offset + size > readable_len()) size = readable_len() - offset;
    size += offset;
    BufferArray<2> array;
    array[0].iov_base = _read;
    array[0].iov_len = continuously_readable() > size ? size : continuously_readable();
    array[1].iov_base = _buffer;
    array[1].iov_len = size - array[0].iov_len;
    array += offset;
    return array;
}

Base::BufferArray<2> Base::RingBuffer::write_array(uint32 size, uint32 offset) const {
    if (offset >= writable_len()) return { iovec { _read, 0 }, iovec { _buffer, 0 } };
    if (size > writable_len()) size = writable_len() - offset;
    size += offset;
    BufferArray<2> array;
    array[0].iov_base = _write;
    array[0].iov_len = continuously_writable() > size ? size : continuously_writable();
    array[1].iov_base = _buffer;
    array[1].iov_len = size - array[0].iov_len;
    array += offset;
    return array;
}

Base::BufferArray<2> Base::RingBuffer::readable_array() const {
    assert(readable_len() >= continuously_readable());
    BufferArray<2> array;
    array[0].iov_base = _read;
    array[0].iov_len = continuously_readable();
    array[1].iov_base = _buffer;
    array[1].iov_len = readable_len() - array[0].iov_len;
    return array;
}

Base::BufferArray<2> Base::RingBuffer::writable_array() const {
    assert(writable_len() >= continuously_writable());
    BufferArray<2> array;
    array[0].iov_base = _write;
    array[0].iov_len = continuously_writable();
    array[1].iov_base = _buffer;
    array[1].iov_len = writable_len() - array[0].iov_len;
    return array;
}

void Base::RingBuffer::read_advance(uint32 step) {
    assert(readable_len() >= step);
    _read += step;
    if (_read >= end())
        _read -= _size;
    _readable -= step;
}

void Base::RingBuffer::read_back(uint32 step) {
    assert(writable_len() >= step);
    _read -= step;
    if (_read < _buffer)
        _read += _size;
    _readable += step;
}

void Base::RingBuffer::write_advance(uint32 step) {
    assert(writable_len() >= step);
    _write += step;
    if (_write >= end())
        _write -= _size;
    _readable += step;
}

void Base::RingBuffer::write_back(uint32 step) {
    assert(step <= readable_len());
    _write -= step;
    if (_write < _buffer)
        _write += _size;
    _readable -= step;
}
