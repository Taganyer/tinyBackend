//
// Created by taganyer on 24-9-5.
//

#include "../BlockFile.hpp"
#include <bits/move.h>

using namespace LogSystem;

using namespace Base;

static inline int64 calculate_pos(uint64 index) {
    return index * BlockFile::BlockSize;
}

BlockFile::BlockFile(const char* path, bool append, bool binary) :
    _file(path, append, binary) {
    if (_file.is_open()) {
        _file.seek_end(0);
        _offset = _file.pos();
        _total_blocks = _offset / BlockSize;
    }
}

BlockFile::BlockFile(BlockFile &&other) noexcept :
    _file(std::move(other._file)) {
    other._total_blocks = 0;
    other._offset = 0;
}

bool BlockFile::open(const char* path, bool append, bool binary) {
    _offset = 0;
    _total_blocks = 0;
    if (!_file.open(path, append, binary)) return false;
    _file.seek_end(0);
    _offset = _file.pos();
    _total_blocks = _offset / BlockSize;
    return true;
}

bool BlockFile::close() {
    _offset = 0;
    _total_blocks = 0;
    return _file.close();
}

uint64 BlockFile::read(void* dest, uint64 index, uint64 count) {
    if (index + count > _total_blocks || locating(_total_blocks)) return 0;
    auto _dest = (char *) dest;
    for (uint64 rs = BlockSize; count > 0 && rs == BlockSize; --count) {
        rs = _file.read(BlockSize, _dest);
        _offset += rs;
        _dest += rs;
    }
    return _dest - (char *)dest;
}

uint64 BlockFile::write_to_back(const void* data, uint64 size) {
    if (!locating(_total_blocks)) return 0;
    auto _data = (const char *) data;
    for (uint64 s, written; size > 0; size -= s) {
        s = size > BlockSize ? BlockSize : size;
        written = padding_write(_data, s);
        ++_total_blocks;
        if (written != BlockSize) {
            if (write_error_handle())
                written = 0;
            return written + (_data - (char *)data);
        }
    }
    return size;
}

uint64 BlockFile::update(const void* data, uint64 size, uint64 index) {
    if (index >= _total_blocks || !locating(index)) return 0;
    if (size > BlockSize) size = BlockSize;
    return padding_write(data, size);
}

bool BlockFile::erase_back_blocks(uint64 count) {
    if (count > _total_blocks) return false;
    uint64 after_size = (_total_blocks - count) * BlockSize;
    if (_offset > after_size)
        locating(_total_blocks - count);
    if (!_file.resize_file(after_size)) {
        locating(0);
        return false;
    }
    _total_blocks -= count;
    return true;
}

bool BlockFile::locating(uint64 index) {
    if (index > _total_blocks) return false;
    int64 offset = calculate_pos(index) - _offset;
    if (_file.seek_cur(offset)) {
        _offset += offset;
        return true;
    }
    return false;
}

uint64 BlockFile::padding_write(const void* data, uint64 size) {
    uint64 written = _file.write(data, size);
    if (written == size && size != BlockSize) {
        char _pad[BlockSize] {};
        written += _file.write(_pad, BlockSize - size);
    }
    _offset += written;
    return written;
}

bool BlockFile::write_error_handle() {
    if (_total_blocks != 0)
        --_total_blocks;
    bool success = _file.resize_file(_total_blocks * BlockSize);
    if (success && _file.seek_beg(_total_blocks * BlockSize))
        _offset = _total_blocks * BlockSize;
    return success;
}
