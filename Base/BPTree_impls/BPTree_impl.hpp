//
// Created by taganyer on 24-9-7.
//

#ifndef BASE_BPTREE_IMPL_HPP
#define BASE_BPTREE_IMPL_HPP

#ifdef BASE_BPTREE_IMPL_HPP

#include "ResultSet_Impl.hpp"
#include "DataBlock_Impl.hpp"
#include "IndexBlock_Impl.hpp"

namespace Base {

    template <typename Impl>
    class BPTree;

    template <typename K, typename V>
    class BPTree_impl {
    public:
        BPTree_impl(ScheduledThread& scheduled_thread, const char *filename, uint64 memory_size);

        BPTree_impl(ScheduledThread& scheduled_thread, BlockFile&& _file, uint64 memory_size);

        ~BPTree_impl();

        void flush() const { _scheduler->force_invoke(); };

        static BlockFile open_file(const char *filename) {
            return BlockFile(filename, true, true, Interpreter::BLOCK_SIZE);
        };

        struct HeaderMessage {
            uint32 total_blocks_size = 0;
            uint32 valid_block_size = 0;
            uint32 deleted_block_size = 0;
            uint32 block_begin = 0;
            uint32 record_begin = 0;

            std::string extra_data;

            HeaderMessage() = default;

            HeaderMessage(const HeaderMessage&) = default;

            explicit HeaderMessage(BlockFile& file) {
                if (total_blocks_size == 0) return;
                if (file.block_size() != Interpreter::BLOCK_SIZE)
                    throw BPTreeFileError("Invalid BlockFile because block size isn't same.");
                if (!file.is_open()) return;
                total_blocks_size = file.total_blocks();
                Interpreter::HeadBlockHelper head { new char[Interpreter::BLOCK_SIZE] };
                if (file.read(head.buffer, 0) == Interpreter::BLOCK_SIZE) {
                    if (!Interpreter::is_head_block(head.buffer))
                        throw BPTreeFileError("Invalid BlockFile because file no header.");
                    valid_block_size = *head.valid_blocks_size();
                    deleted_block_size = *head.deleted_blocks_size();
                    block_begin = *head.begin_block_index();
                    record_begin = *head.begin_record_block_index();
                    if (head.have_extra_data()) {
                        extra_data.resize(*head.extra_data_size());
                        std::memcpy(extra_data.data(), head.extra_data(), *head.extra_data_size());
                    }
                }
                delete[] (char *) head.buffer;
            }
        };

    private:
        friend class BPTree<BPTree_impl>;

        friend class BlockIter<K, V>;

        using Key = K;

        using Value = V;

        using Result = Result_Impl<Key, Value>;

        using ResultSet = ResultSet_Impl<Key, Value>;

        using Iter = BlockIter<Key, Value>;

        class IterKey;

        using DataBlock = DataBlock_Impl<Key, Value>;

        using IndexBlock = IndexBlock_Impl<Key, Value>;

        Iter begin_block();

        void set_begin_block(const Iter& iter);

        void erase_block(Iter iter);

        DataBlock create_data_block();

        IndexBlock create_index_block();

    private:
        std::shared_ptr<Impl_Scheduler> _scheduler;

        using HeadBlock = Interpreter::HeadBlockHelper;

        using RecordBlock = Interpreter::RecordBlockHelper;

        HeadBlock _header;

        RecordBlock _deleted_record;

        uint32 _deleted_record_index = 0;

        bool _head_need_update = false, _deleted_record_need_update = false;

        void init();

        uint32 get_new_block_index();

    };

    template <typename K, typename V>
    BPTree_impl<K, V>::BPTree_impl(ScheduledThread& scheduled_thread,
                                   const char *filename, uint64 memory_size) :
        _scheduler(std::make_shared<Impl_Scheduler>(scheduled_thread, filename, memory_size)) {
        init();
    }

    template <typename K, typename V>
    BPTree_impl<K, V>::BPTree_impl(ScheduledThread& scheduled_thread,
                                   BlockFile&& _file, uint64 memory_size) :
        _scheduler(std::make_shared<Impl_Scheduler>(scheduled_thread, std::move(_file), memory_size)) {
        init();
    }

    template <typename K, typename V> BPTree_impl<K, V>::~BPTree_impl() {
        _scheduler->put_block(0, _head_need_update);
        if (_deleted_record_index != 0) {
            _scheduler->put_block(_deleted_record_index, _deleted_record_need_update);
        }
        _scheduler->invoke(nullptr);
        _scheduler->_thread->remove_scheduler(_scheduler);
    }

    template <typename K, typename V>
    typename BPTree_impl<K, V>::Iter BPTree_impl<K, V>::begin_block() {
        uint32 index = *_header.begin_block_index();
        if (index == 0) return Iter();
        void *buf = _scheduler->get_block(index, true);
        bool is_data_block = Interpreter::is_data_block(buf);
        _scheduler->put_block(index, false);
        return Iter { true, is_data_block, index, nullptr };
    }

    template <typename K, typename V>
    void BPTree_impl<K, V>::set_begin_block(const Iter& iter) {
        _head_need_update = true;
        if (!iter.valid()) *_header.begin_block_index() = 0;
        else *_header.begin_block_index() = iter.index();
    }

    template <typename K, typename V>
    void BPTree_impl<K, V>::erase_block(Iter iter) {
        assert(iter.valid());
        if (iter.is_data_block(*this))
            DataBlock::erase_block(*_scheduler, iter.index());
        --*_header.valid_blocks_size();

        if (_deleted_record.buffer) {
            if (!_deleted_record.can_add_deleted_block()) {
                uint32 new_record = _deleted_record.get_new_record_block_index();
                if (_deleted_record_need_update)
                    _scheduler->put_block(_deleted_record_index, true);
                _deleted_record.buffer = _scheduler->get_block(new_record, false);
                _deleted_record.set_to_record_deleted_block();
                *_deleted_record.last_index() = _deleted_record_index;
                _deleted_record_index = new_record;
                *_header.begin_record_block_index() = new_record;
            }
            _deleted_record.add_deleted_block(iter.index());
            _deleted_record_need_update = true;
            ++*_header.deleted_blocks_size();
        } else {
            if (_header.can_add_deleted_block()) {
                _header.add_deleted_block(iter.index());
            } else {
                uint32 new_record = _header.get_new_record_block_index();
                _deleted_record.buffer = _scheduler->get_block(new_record, false);
                _deleted_record.set_to_record_deleted_block();
                *_deleted_record.last_index() = 0;
                _deleted_record_index = new_record;
                *_header.begin_record_block_index() = new_record;
                _deleted_record.add_deleted_block(iter.index());
                _deleted_record_need_update = true;
                ++*_header.deleted_blocks_size();
            }
        }
        _head_need_update = true;
    }

    template <typename K, typename V>
    typename BPTree_impl<K, V>::DataBlock BPTree_impl<K, V>::create_data_block() {
        ++*_header.valid_blocks_size();
        _head_need_update = true;
        if (*_header.deleted_blocks_size() == 0)
            return DataBlock(*_scheduler);
        uint32 index = get_new_block_index();
        void *buf = _scheduler->get_block(index, false);
        return DataBlock(*_scheduler, index, buf);
    }

    template <typename K, typename V>
    typename BPTree_impl<K, V>::IndexBlock
    BPTree_impl<K, V>::create_index_block() {
        ++*_header.valid_blocks_size();
        _head_need_update = true;
        if (*_header.deleted_blocks_size() == 0)
            return IndexBlock(*_scheduler);
        uint32 index = get_new_block_index();
        void *buf = _scheduler->get_block(index, false);
        return IndexBlock(*_scheduler, index, buf);
    }

    template <typename K, typename V>
    void BPTree_impl<K, V>::init() {
        if (_scheduler->total_blocks() > 0) {
            _header.buffer = _scheduler->get_block(0, true);
            if (!Interpreter::is_head_block(_header.buffer))
                throw BPTreeFileError("Invalid BlockFile because file no header.");
            _deleted_record_index = *_header.begin_record_block_index();
            if (_deleted_record_index != 0) {
                _deleted_record.buffer = _scheduler->get_block(_deleted_record_index, true);
                if (!Interpreter::is_record_deleted_block(_deleted_record.buffer))
                    throw BPTreeFileError("Corrupted BlockFile due to "
                        "failed to read record_deleted_block.");
            }
        } else {
            auto [i, p] = _scheduler->get_block();
            assert(i == 0);
            _header.buffer = p;
            _header.set_to_head_block();
            _scheduler->put_block(0, true);
            _scheduler->invoke(nullptr);
            _scheduler->get_block(0, true);
        }
        _scheduler->_thread->add_scheduler(_scheduler);
    }

    template <typename K, typename V>
    uint32 BPTree_impl<K, V>::get_new_block_index() {
        if (!_deleted_record.buffer)
            return _header.pop_deleted_block_index();
        assert(_deleted_record.can_get_deleted_block());
        uint32 index = _deleted_record.pop_deleted_block_index();
        _deleted_record_need_update = true;
        if (!_deleted_record.can_get_deleted_block()) {
            uint32 last = *_deleted_record.last_index();
            _scheduler->put_block(_deleted_record_index, false);
            _deleted_record_index = last;
            if (last == 0) {
                _deleted_record.buffer = nullptr;
            } else {
                _deleted_record.buffer = _scheduler->get_block(last, true);
                if (!Interpreter::is_record_deleted_block(_deleted_record.buffer))
                    throw BPTreeFileError("Corrupted BlockFile due to "
                        "failed to read record_deleted_block.");
            }
            _deleted_record_need_update = false;
        }
        --*_header.deleted_blocks_size();
        return index;
    }

    template <typename K, typename V>
    class BPTree_impl<K, V>::IterKey {
    public:
        IterKey() = default;

        explicit IterKey(const Iter& iter, const Key& key) : _iter(iter), _key(key) {};

        Key& key() { return _key; };

        Iter& iter() { return _iter; };

        const Key& key() const { return _key; };

        const Iter& iter() const { return _iter; };

        [[nodiscard]] bool valid() const { return _iter.valid(); };

    private:
        Iter _iter;

        Key _key;

    };

}

#endif

#endif //BASE_BPTREE_IMPL_HPP
