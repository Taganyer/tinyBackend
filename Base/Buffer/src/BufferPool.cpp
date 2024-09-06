//
// Created by taganyer on 24-7-24.
//

#include "../BufferPool.hpp"

using namespace Base;

static inline std::pair<uint64, uint64> parent_sibling(uint64 i) {
    uint64 p, s;
    if (i & 1) {
        p = (i - 1) >> 1;
        s = i + 1;
    } else {
        p = (i - 2) >> 1;
        s = i - 1;
    }
    return { p, s };
}

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
    _rest(std::vector<uint64>(_size / block_size << 1)) {
    for (uint64 s = _size, t = 1, i = 0; s >= block_size; s >>= 1, t <<= 1)
        for (uint64 end = i + t; i < end; ++i)
            _rest[i] = s;
}

BufferPool::~BufferPool() {
    assert(_rest[0] != _size ? nullptr : "memory leak");
    delete[] _buffer;
}

BufferPool::Buffer BufferPool::get(uint64 size) {
    Lock l(_mutex);
    if ((size = round_size(size)) > _rest[0])
        return { nullptr, 0, *this };
    auto [i, ptr] = positioning(size);

    assert(_rest[i] == size);
    _rest[i] = 0;
    while (i > 0) {
        auto [p, s] = parent_sibling(i);
        uint64 ms = std::max(_rest[i], _rest[s]);
        assert(_rest[p] >= ms);
        if (_rest[p] == ms) break;
        _rest[p] = ms;
        i = p;
    }
    return { ptr, size, *this };
}

void BufferPool::put(Buffer &buffer) {
    Lock l(_mutex);
    uint64 i = location(buffer._buf, buffer._size), fs = buffer._size;
    buffer._buf = nullptr;
    buffer._size = 0;

    assert(_rest[i] == 0);
    _rest[i] = fs;
    while (i > 0) {
        auto [p, s] = parent_sibling(i);
        uint64 ms = std::max(_rest[i], _rest[s]);
        if (_rest[i] == fs && fs == _rest[s]) ms = fs << 1;
        assert(_rest[p] <= ms);
        if (_rest[p] == ms) break;
        _rest[p] = ms;
        i = p;
        fs <<= 1;
    }
}

std::pair<uint64, char *> BufferPool::positioning(uint64 size) const {
    uint64 i = 0;
    int n = 0;
    for (uint64 s = _size; s != size; s >>= 1, ++n) {
        uint64 _l = (i << 1) + 1, _r = (i << 1) + 2;
        assert(_rest[_l] >= size || _rest[_r] >= size);
        i = _rest[_r] < size || _rest[_l] >= size && _rest[_l] <= _rest[_r] ? _l : _r;
    }
    return { i, _buffer + (i - ((1 << n) - 1)) * size };
}

uint64 BufferPool::location(const char* target, uint64 size) const {
    uint64 i = 0;
    for (auto s = size; _size > s; s <<= 1) ++i;
    i = (1 << i) - 1;
    return i + (target - _buffer) / size;
}
