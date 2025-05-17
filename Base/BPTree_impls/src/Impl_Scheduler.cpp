//
// Created by taganyer on 24-9-9.
//

#include "../Impl_Scheduler.hpp"

using namespace Base;

Impl_Scheduler::Impl_Scheduler(ScheduledThread& scheduled_thread,
                               const char *filename, uint64 memory_size) :
    _thread(&scheduled_thread), _cache(filename, memory_size) {}

Impl_Scheduler::Impl_Scheduler(ScheduledThread& scheduled_thread,
                               BlockFile&& file, uint64 memory_size) :
    _thread(&scheduled_thread), _cache(std::move(file), memory_size) {}

void Impl_Scheduler::invoke(void *arg) {
    Lock l(_mutex);
    _cache.update_all_with_order(std::less<>());
    _cache._helper._file.flush_to_disk();
}

void Impl_Scheduler::force_invoke() {
    invoke(nullptr);
}

std::pair<uint32, void *> Impl_Scheduler::get_block() {
    Lock l(_mutex);
    _cache._helper.need_read_from_file = false;
    Buffer *ptr = _cache.get(_cache._helper._total_blocks);
    return { _cache._helper._total_blocks++, ptr->data() };
}

void* Impl_Scheduler::get_block(uint32 index, bool read_data) {
    assert(index < _cache._helper._total_blocks);
    Lock l(_mutex);
    _cache._helper.need_read_from_file = read_data;
    Buffer *ptr = _cache.get(index);
    return ptr->data();
}

void Impl_Scheduler::put_block(uint32 index, bool need_update) {
    assert(index < _cache._helper._total_blocks);
    Lock l(_mutex);
    _cache.put(index, need_update);
}

Impl_Scheduler::LRU_Helper::LRU_Helper(const char *filename, uint64 memory_size) :
    _memory_pool(memory_size), _file(filename, true, true, Interpreter::BLOCK_SIZE) {
    if (!_file.is_open())
        throw BPTreeFileError("Failed to open file " + std::string(filename));
    _total_blocks = _file.total_blocks();
}

Impl_Scheduler::LRU_Helper::LRU_Helper(BlockFile&& file, uint64 memory_size) :
    _memory_pool(memory_size), _file(std::move(file)) {
    if (!_file.is_open())
        throw BPTreeFileError("Failed to open file: " + _file.get_path());
    if (_file.block_size() != Interpreter::BLOCK_SIZE)
        throw BPTreeFileError("Block size mismatch in " + _file.get_path());
    _total_blocks = _file.total_blocks();
}

bool Impl_Scheduler::LRU_Helper::can_create(Key) const {
    return _memory_pool.max_block() >= Interpreter::BLOCK_SIZE;
}

Impl_Scheduler::LRU_Helper::Value
Impl_Scheduler::LRU_Helper::create(Key index) {
    Value buffer = _memory_pool.get(Interpreter::BLOCK_SIZE);
    if (need_read_from_file) {
        auto read_size = _file.read(buffer.data(), index);
        if (unlikely(read_size != Interpreter::BLOCK_SIZE)) {
            throw BPTreeRuntimeError("Failed to read block " + std::to_string(index)
                + " from file " + _file.get_path());
        }
    }
    return buffer;
}

void Impl_Scheduler::LRU_Helper::update(Key index, Value& buffer) {
    if (index >= _file.total_blocks()) {
        bool resize_success = _file.resize_file_total_blocks(_total_blocks);
        if (unlikely(!resize_success)) {
            throw BPTreeRuntimeError("Failed to resize file " + _file.get_path()
                + " to " + std::to_string(_total_blocks * Interpreter::BLOCK_SIZE));
        }
    }
    assert(index <= _file.total_blocks());
    auto update_size = _file.update(buffer.data(), Interpreter::BLOCK_SIZE, index);
    if (unlikely(update_size != Interpreter::BLOCK_SIZE)) {
        throw BPTreeRuntimeError("Failed to update block " + std::to_string(index)
            + " from file " + _file.get_path());
    }
}
