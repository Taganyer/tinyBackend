//
// Created by taganyer on 24-9-9.
//

#ifndef BASE_BLOCKITER_HPP
#define BASE_BLOCKITER_HPP

#ifdef BASE_BLOCKITER_HPP

#include "Impl_Scheduler.hpp"

namespace Base {

    template <typename Key, typename Value>
    class DataBlock_Impl;

    template <typename Key, typename Value>
    class IndexBlock_Impl;

    template <typename Key, typename Value>
    class BPTree_impl;

    template <typename Key, typename Value>
    class BlockIter {
        using DataBlock = DataBlock_Impl<Key, Value>;
        using IndexBlock = IndexBlock_Impl<Key, Value>;
        using Impl = BPTree_impl<Key, Value>;

        friend class DataBlock_Impl<Key, Value>;
        friend class IndexBlock_Impl<Key, Value>;
        friend class BPTree_impl<Key, Value>;

    public:
        BlockIter() = default;

        BlockIter(const BlockIter&) = default;

        BlockIter(bool is_self_valid, bool is_data, uint32 index_or_offset, void *buffer) :
            _self_valid(is_self_valid), _belong_data_block(is_data),
            _index_or_offset(index_or_offset), _block_ptr(buffer) {};

        DataBlock data_block(Impl& impl) {
            return DataBlock(*impl._scheduler, index());
        };

        IndexBlock index_block(Impl& impl) {
            return IndexBlock(*impl._scheduler, index());
        };

        Key& key() {
            check_not_self_valid();
            if (_belong_data_block) return *DataBlock::get_key(_block_ptr, _index_or_offset);
            return *IndexBlock::get_key(_block_ptr, _index_or_offset);
        };

        const Key& key() const {
            check_not_self_valid();
            if (_belong_data_block) return *DataBlock::get_key(_block_ptr, _index_or_offset);
            return *IndexBlock::get_key(_block_ptr, _index_or_offset);
        };

        Value& value() {
            check_not_self_valid();
            return *DataBlock::get_value(_block_ptr, _index_or_offset);
        };

        const Value& value() const {
            check_not_self_valid();
            return *DataBlock::get_value(_block_ptr, _index_or_offset);
        };

        [[nodiscard]] uint32 index() const {
            if (_self_valid) return _index_or_offset;
            return *IndexBlock::get_index(_block_ptr, _index_or_offset);
        };

        BlockIter& operator++() {
            check_not_self_valid();
            if (_belong_data_block) _index_or_offset = DataBlock::get_offset(_block_ptr, _index_or_offset, 1);
            else _index_or_offset = IndexBlock::get_offset(_block_ptr, _index_or_offset, 1);
            return *this;
        };

        BlockIter& operator--() {
            check_not_self_valid();
            if (_belong_data_block) _index_or_offset = DataBlock::get_offset(_block_ptr, _index_or_offset, -1);
            else _index_or_offset = IndexBlock::get_offset(_block_ptr, _index_or_offset, -1);
            return *this;
        };

        BlockIter& operator=(const BlockIter& other) = default;

        friend bool operator==(const BlockIter& left, const BlockIter& right) {
            return left._belong_data_block == right._belong_data_block
                && left._index_or_offset == right._index_or_offset
                && left._block_ptr == right._block_ptr;
        };

        friend bool operator!=(const BlockIter& left, const BlockIter& right) {
            return !operator==(left, right);
        };

        friend BlockIter operator+(const BlockIter& iter, int n) {
            iter.check_not_self_valid();
            int new_offset = iter._belong_data_block ?
                                 DataBlock::get_offset(iter._block_ptr, iter._index_or_offset, n) :
                                 IndexBlock::get_offset(iter._block_ptr, iter._index_or_offset, n);
            return BlockIter { false, iter._belong_data_block, new_offset, iter._block_ptr };
        };

        friend BlockIter operator-(const BlockIter& iter, int n) {
            iter.check_not_self_valid();
            int new_offset = iter._belong_data_block ?
                                 DataBlock::get_offset(iter._block_ptr, iter._index_or_offset, -n) :
                                 IndexBlock::get_offset(iter._block_ptr, iter._index_or_offset, -n);
            return BlockIter { false, iter._belong_data_block, new_offset, iter._block_ptr };
        };

        [[nodiscard]] bool is_data_block(Impl& impl) {
            if (_self_valid) return _belong_data_block;
            uint32 i = index();
            void *buf = impl._scheduler->get_block(i, true);
            bool is_data_block = Interpreter::is_data_block(buf);
            impl._scheduler->put_block(i, false);
            return is_data_block;
        };

        [[nodiscard]] bool valid() const { return _block_ptr != nullptr || _self_valid; };

    private:
        bool _self_valid = false, _belong_data_block = false;

        uint32 _index_or_offset = 0;

        void *_block_ptr = nullptr;

        void check_not_self_valid() const {
            if (unlikely(_self_valid))
                throw BPTreeRuntimeError("Attempting to access self_valid block.");
        };

    };

}

#endif

#endif //BASE_BLOCKITER_HPP
