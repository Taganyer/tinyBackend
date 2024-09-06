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

uint32 Base::RingBuffer::write(const void* target, uint32 size) {
    uint32 writable = writable_len();
    if (size > writable) size = writable;
    if (_write + writable <= end()) {
        std::memcpy(_write, target, size);
    } else {
        auto len = end() - _write;
        auto p = (const char *) target + len;
        std::memcpy(_write, target, len);
        std::memcpy(_buffer, p, size - len);
    }
    write_move(size);
    return size;
}

uint32 Base::RingBuffer::read_to(void* dest, uint32 size) {
    if (size > _readable) size = _readable;
    if (auto len = end() - _read; len >= size) {
        std::memcpy(dest, _read, size);
    } else {
        auto p = (char *) dest + len;
        std::memcpy(dest, _read, len);
        std::memcpy(p, _buffer, size - len);
    }
    read_move(size);
    return size;
}

Base::RingBuffer::ViewPair Base::RingBuffer::get_view() const {
    auto dis = end() - _read;
    const char* ps = nullptr;
    uint32 sf, ss = 0;
    if (dis >= _readable) {
        sf = _readable;
    } else {
        sf = dis;
        ss = _readable - sf;
        ps = _buffer;
    }
    return { View(_read, sf), View(ps, ss) };
}

void Base::RingBuffer::read_advance(uint32 step) {
    assert(_readable >= step);
    read_move(step);
}

void Base::RingBuffer::write_advance(uint32 step) {
    assert(_size - _readable >= step);
    write_move(step);
}
