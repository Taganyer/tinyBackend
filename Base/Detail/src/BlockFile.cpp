//
// Created by taganyer on 24-9-5.
//

#include <bits/move.h>
#include "../BlockFile.hpp"
#include "Base/Exception.hpp"


using namespace Base;

BlockFile::BlockFile(const char* path, bool append,
                     bool binary, uint64 block_size) :
    _file(path, append, binary), _block_size(block_size) {
    if (_file.is_open()) {
        struct stat st {};
        if (!_file.get_stat(&st))
            throw Exception("Could not get stat from: " + std::string(path));
        if (st.st_size % block_size != 0)
            throw Exception("Block file size is not a multiple of block size.");
        _total_blocks = st.st_size / _block_size;
    }
}

BlockFile::BlockFile(BlockFile &&other) noexcept :
    _file(std::move(other._file)), _offset(other._offset),
    _total_blocks(other._total_blocks), _block_size(other._block_size) {
    other._total_blocks = 0;
    other._offset = 0;
    other._total_blocks = 0;
    other._block_size = 0;
}

bool BlockFile::open(const char* path, bool append, bool binary) {
    _offset = 0;
    _total_blocks = 0;
    if (!_file.open(path, append, binary)) return false;
    struct stat st {};
    if (!_file.get_stat(&st)) {
        _file.close();
        return false;
    }
    _total_blocks = st.st_size / _block_size;
    return true;
}

bool BlockFile::close() {
    _offset = 0;
    _total_blocks = 0;
    return _file.close();
}

uint64 BlockFile::read(void* dest, uint64 index, uint64 count) {
    if (index + count > _total_blocks || !locating(index)) return 0;
    auto _dest = (char *) dest;
    for (uint64 rs = _block_size; count > 0 && rs == _block_size; --count) {
        rs = _file.read(_block_size, _dest);
        _offset += rs;
        _dest += rs;
    }
    return _dest - (char *) dest;
}

uint64 BlockFile::write_to_back(const void* data, uint64 size) {
    if (!locating(_total_blocks)) return 0;
    auto _data = (const char *) data;
    for (uint64 s, written; size > 0; size -= s) {
        s = size > _block_size ? _block_size : size;
        written = padding_write(_data, s);
        ++_total_blocks;
        if (unlikely(written != _block_size)) {
            if (write_error_handle())
                written = 0;
            return written + (_data - (char *) data);
        }
        _data += written;
    }
    return _data - (char *) data;
}

uint64 BlockFile::update(const void* data, uint64 size, uint64 index) {
    if (index >= _total_blocks || !locating(index)) return 0;
    if (size > _block_size) size = _block_size;
    return padding_write(data, size);
}

bool BlockFile::erase_back_blocks(uint64 count) {
    if (count > _total_blocks) return false;
    uint64 after_size = (_total_blocks - count) * _block_size;
    if (_offset > after_size)
        locating(_total_blocks - count);
    if (!_file.resize_file(after_size)) {
        locating(0);
        return false;
    }
    _total_blocks -= count;
    return true;
}

bool BlockFile::resize_file_total_blocks(uint64 new_block_size) {
    if (!_file.resize_file(new_block_size * _block_size)) return false;
    _total_blocks = new_block_size;
    return true;
}

bool BlockFile::locating(uint64 index) {
    if (index > _total_blocks) return false;
    int64 offset = index * _block_size - _offset;
    if (_file.seek_cur(offset)) {
        _offset += offset;
        return true;
    }
    return false;
}

uint64 BlockFile::padding_write(const void* data, uint64 size) {
    uint64 written = _file.write(data, size);
    if (written == size && size != _block_size) {
        char* _pad = new char[_block_size];
        written += _file.write(_pad, _block_size - size);
        delete [] _pad;
    }
    _offset += written;
    return written;
}

bool BlockFile::write_error_handle() {
    if (_total_blocks != 0)
        --_total_blocks;
    bool success = _file.resize_file(_total_blocks * _block_size);
    if (success && _file.seek_beg(_total_blocks * _block_size))
        _offset = _total_blocks * _block_size;
    return success;
}
