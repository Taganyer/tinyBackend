//
// Created by taganyer on 4/6/24.
//

#include "BufferPool.hpp"
#include "../Log/Log.hpp"

using namespace Base;


BufferPool::BufferPool(uint64 block_size, uint64 block_number) :
        _buf(new char[block_size * block_number]), _block_size(block_size) {
    if (!_buf) {
        G_FATAL << "BufferPool creat failed: " << errno;
        assert(_buf);
    } else {
        G_TRACE << "BufferPool created, size: " << block_size * block_number;
    }
    _buffers.reserve(block_number);
    char *ptr = _buf;
    for (; block_number; ptr += block_size, --block_number)
        _buffers.push_back(ptr);
}

BufferPool::~BufferPool() {
    Lock l(_mutex);
    if (_running.size()) {
        G_ERROR << "BufferPool force free " << _running.size() << " buffers";
        _buffers.erase(_buffers.begin(), _buffers.end());
    }
    delete[] _buf;
}

BufferPool::Buffer BufferPool::get() {
    Lock l(_mutex);
    _condition.wait(l, [this] {
        return !_buffers.empty();
    });
    void *ptr = _buffers.back();
    _buffers.pop_back();
    return _running.insert(_running.end(), ptr, _block_size);
}

void BufferPool::free(BufferPool::Buffer buffer) {
    Lock l(_mutex);
    _buffers.push_back(buffer->buffer);
    _running.erase(buffer);
    _condition.notify_one();
}
