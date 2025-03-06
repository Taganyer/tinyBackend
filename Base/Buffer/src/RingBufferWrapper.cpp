//
// Created by taganyer on 25-3-3.
//
#include "../RingBufferWrapper.hpp"
#include <cstring>
#include <cassert>

uint32 Base::RingBufferWrapper::read(void* dest, uint32 size) const {
    if (size > readable_len()) size = readable_len();
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

uint32 Base::RingBufferWrapper::read(uint32 N, const iovec* array) const {
    uint32 read_len = 0;
    for (; N > 0; --N, ++array) {
        if (array->iov_len == 0) continue;
        if (readable_len() == 0) break;
        read_len += read(array->iov_base, array->iov_len);
    }
    return read_len;
}

uint32 Base::RingBufferWrapper::read(const OutputBuffer &buffer, uint32 size) const {
    if (this == &buffer) return 0;
    if (size > readable_len()) size = readable_len();
    size = buffer.write(read_array(size));
    read_advance(size);
    return size;
}

uint32 Base::RingBufferWrapper::try_read(void* dest, uint32 size, uint32 offset) const {
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

uint32 Base::RingBufferWrapper::try_read(uint32 N, const iovec* array, uint32 offset) const {
    uint32 read_size = 0;
    for (; N > 0; --N, ++array) {
        if (array->iov_len == 0) continue;
        if (readable_len() <= offset + read_size) break;
        read_size += try_read(array->iov_base, array->iov_len, offset + read_size);
    }
    return read_size;
}

uint32 Base::RingBufferWrapper::write(const void* target, uint32 size) const {
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

uint32 Base::RingBufferWrapper::write(uint32 N, const iovec* array) const {
    uint32 written = 0;
    for (; N > 0; --N, ++array) {
        if (array->iov_len == 0) continue;
        if (writable_len() == 0) break;
        written += write(array->iov_base, array->iov_len);
    }
    return written;
}

uint32 Base::RingBufferWrapper::write(const InputBuffer &buffer, uint32 size) const {
    if (this == &buffer) return 0;
    if (size > writable_len()) size = writable_len();
    size = buffer.read(write_array(size));
    write_advance(size);
    return size;
}

uint32 Base::RingBufferWrapper::try_write(const void* target, uint32 size, uint32 offset) const {
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

uint32 Base::RingBufferWrapper::try_write(uint32 N, const iovec* array, uint32 offset) const {
    uint32 written = 0;
    for (; N > 0; --N, ++array) {
        if (array->iov_len == 0) continue;
        if (writable_len() <= offset + written) break;
        written += try_write(array->iov_base, array->iov_len, offset + written);
    }
    return written;
}

uint32 Base::RingBufferWrapper::change_written(uint32 offset, const void* data, uint32 size) const {
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

uint32 Base::RingBufferWrapper::change_written(uint32 offset, uint32 N, const iovec* array) const {
    uint32 written = 0;
    for (; N > 0; --N, ++array) {
        if (array->iov_len == 0) continue;
        if (readable_len() <= offset + written) break;
        written += change_written(offset + written, array->iov_base, array->iov_len);
    }
    return written;
}

Base::BufferArray<2> Base::RingBufferWrapper::read_array(uint32 size, uint32 offset) const {
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

Base::BufferArray<2> Base::RingBufferWrapper::write_array(uint32 size, uint32 offset) const {
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

Base::BufferArray<2> Base::RingBufferWrapper::readable_array() const {
    assert(readable_len() >= continuously_readable());
    BufferArray<2> array;
    array[0].iov_base = _read;
    array[0].iov_len = continuously_readable();
    array[1].iov_base = _buffer;
    array[1].iov_len = readable_len() - array[0].iov_len;
    return array;
}

Base::BufferArray<2> Base::RingBufferWrapper::writable_array() const {
    assert(writable_len() >= continuously_writable());
    BufferArray<2> array;
    array[0].iov_base = _write;
    array[0].iov_len = continuously_writable();
    array[1].iov_base = _buffer;
    array[1].iov_len = writable_len() - array[0].iov_len;
    return array;
}

void Base::RingBufferWrapper::read_advance(uint32 step) const {
    assert(readable_len() >= step);
    _read += step;
    if (_read >= end())
        _read -= _size;
    _readable -= step;
}

void Base::RingBufferWrapper::read_back(uint32 step) const {
    assert(writable_len() >= step);
    _read -= step;
    if (_read < _buffer)
        _read += _size;
    _readable += step;
}

void Base::RingBufferWrapper::write_advance(uint32 step) const {
    assert(writable_len() >= step);
    _write += step;
    if (_write >= end())
        _write -= _size;
    _readable += step;
}

void Base::RingBufferWrapper::write_back(uint32 step) const {
    assert(step <= readable_len());
    _write -= step;
    if (_write < _buffer)
        _write += _size;
    _readable -= step;
}

void Base::RingBufferWrapper::reposition() const {
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

char* Base::RingBufferWrapper::resize(char* buf, uint32 size) {
    assert(readable_len() <= size);
    char* old = _buffer;
    uint32 old_readable = readable_len();
    if (buf <= old && buf + size > old || buf > old && old + _size > buf) {
        reposition();
        std::memmove(buf, old, old_readable);
    } else {
        read(buf, old_readable);
    }
    _write = _read = _buffer = buf;
    _readable = 0;
    _size = size;
    write_advance(old_readable);
    return old;
}
