//
// Created by taganyer on 24-7-24.
//

#include "../BufferPool.hpp"

using namespace Base;


uint64 BufferPool::round_size(uint64 target) {
    uint64 pre = 0, size = block_size;
    while (size < target && size > pre) {
        pre = size;
        size <<= 1;
    }
    return size < pre ? pre : size;
}

BufferPool::BufferPool(uint64 total_size) :
    _size(round_size(total_size)), _buffer(new char[_size]),
    _mark(std::vector(_size / block_size << 1, true)) {}

BufferPool::~BufferPool() {
    for (auto flag : _mark)
        assert(flag ? "memory leak" : nullptr);
    delete[] _buffer;
}

BufferPool::Buffer BufferPool::get(uint64 size) {
    if (size == 0 || size > _size || (size = round_size(size)) > _size)
        return { nullptr, 0, *this };
    Lock l(_mutex);
    auto begin = get_begin(size), end = begin + _size / size, n = begin;
    while (n != end && !_mark[n]) ++n;
    if (n == end) return { nullptr, 0, *this };
    begin = (n - begin) * size;
    while (_mark[n]) {
        _mark[n] = false;
        if (n == 0) break;
        if (n & 1) n = (n - 1) >> 1;
        else n = (n - 2) >> 1;
    }
    return { _buffer + begin, size, *this };
}

void BufferPool::put(Buffer &buffer) {
    Lock l(_mutex);
    auto n = check(buffer.buf, buffer._size);
    buffer.buf = nullptr;
    buffer._size = 0;
    _mark[n] = true;
    while (n & 1 && _mark[n + 1] || n != 0 && _mark[n - 1]) {
        if (n & 1) n = (n - 1) >> 1;
        else n = (n - 2) >> 1;
        _mark[n] = true;
    }
}

uint64 BufferPool::get_begin(uint64 size) const {
    uint64 n = 0;
    while (_size > size) {
        size <<= 1;
        ++n;
    }
    n = (1 << n) - 1;
    return n;
}

uint64 BufferPool::location(const char* target, uint64 size) const {
    return get_begin(size) + (target - _buffer) / size;
}

uint64 BufferPool::check(const char* target, uint64 size) const {
    assert(size && size % block_size == 0);
    assert(target && target >= _buffer && target + size <= _buffer + _size);
    uint64 n = block_size;
    while (n < size) n <<= 1;
    assert(n == size && !_mark[n = location(target, size)]);
    return n;
}
