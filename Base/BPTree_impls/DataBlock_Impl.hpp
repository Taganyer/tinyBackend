//
// Created by taganyer on 24-9-9.
//

#ifndef BASE_DATABLOCK_IMPL_HPP
#define BASE_DATABLOCK_IMPL_HPP

#ifdef BASE_DATABLOCK_IMPL_HPP

#include "BlockIter.hpp"

namespace Base {

    template <typename Key, typename Value>
    class DataBlock_Impl : NoCopy {
        using Iter = BlockIter<Key, Value>;
        using KeyChecker = FixedSizeChecker<Key>;
        using ValueChecker = FixedSizeChecker<Value>;
        using DataBlockHelper = Interpreter::DataBlockHelper;

        static_assert(KeyChecker::is_fixed);
        static constexpr uint32 KeySize = size_helper<Key>::size;

    public:
        explicit DataBlock_Impl(Impl_Scheduler& impl);

        explicit DataBlock_Impl(Impl_Scheduler& impl, uint32 index);

        explicit DataBlock_Impl(Impl_Scheduler& impl, uint32 index, void *buf);

        DataBlock_Impl(DataBlock_Impl&& other) noexcept;

        ~DataBlock_Impl();

        void insert(const Iter& pos, const Key& key, const Value& value);

        bool update(const Iter& pos, const Value& value);

        bool erase(const Iter& iter);

        void erase(const Iter& begin, const Iter& end);

        void merge(const DataBlock_Impl& other);

        void split(DataBlock_Impl& new_block, const Key& key, const Value& value);

        void average(DataBlock_Impl& other);

        int merge_keep_average(const DataBlock_Impl& other) const;

        Iter lower_bound(const Key& key) const;

        void to_prev();

        void to_next();

        [[nodiscard]] uint32 index() const { return _index; };

        [[nodiscard]] Iter begin() const;

        [[nodiscard]] Iter end() const;

        [[nodiscard]] Iter self_iter() const;

        [[nodiscard]] bool can_insert(const Value& value) const;

        [[nodiscard]] bool have_prev() const;

        [[nodiscard]] bool have_next() const;

        static bool data_size_check(const Value& value);

        static Key* get_key(void *buf, uint32 offset);

        static Value* get_value(void *buf, uint32 offset);

        static const Key* get_key(const void *buf, uint32 offset);

        static const Value* get_value(const void *buf, uint32 offset);

        static uint32 get_offset(const void *buf, uint32 from, int n);

        static void erase_block(Impl_Scheduler& impl, uint32 index);

    private:
        uint32 _index = 0;

        bool _need_update = false, _need_put_back = true;

        Interpreter::DataBlockHelper _helper;

        Impl_Scheduler *_impl = nullptr;

        static void check_is_DataBlock(const void *buffer) {
            if (unlikely(!Interpreter::is_data_block(buffer)))
                throw BPTreeRuntimeError("Attempting to transform non-DataBlock to DataBlock.");
        };

        static void move_other_block_data(DataBlock_Impl& dest, uint32 dest_pos,
                                          DataBlock_Impl& src, uint32 src_pos, uint32 size);

    };

    template <typename Key, typename Value>
    DataBlock_Impl<Key, Value>::DataBlock_Impl(Impl_Scheduler& impl) : _impl(&impl) {
        auto [i, p] = impl.get_block();
        _index = i;
        _helper.buffer = p;
        _helper.set_to_data_block();
        *_helper.prev_index() = 0;
        *_helper.next_index() = 0;
        _need_update = true;
    }

    template <typename Key, typename Value>
    DataBlock_Impl<Key, Value>::DataBlock_Impl(Impl_Scheduler& impl, uint32 index) :
        _index(index), _impl(&impl) {
        _helper.buffer = impl.get_block(index, true);
        check_is_DataBlock(_helper.buffer);
    }

    template <typename Key, typename Value>
    DataBlock_Impl<Key, Value>::DataBlock_Impl(Impl_Scheduler& impl, uint32 index, void *buf) :
        _index(index), _impl(&impl) {
        _helper.buffer = buf;
        _helper.set_to_data_block();
        _need_update = true;
    }

    template <typename Key, typename Value>
    DataBlock_Impl<Key, Value>::DataBlock_Impl(DataBlock_Impl&& other) noexcept :
        _index(other._index), _need_update(other._need_update),
        _helper(other._helper), _impl(other._impl) {
        other._need_put_back = false;
    }

    template <typename Key, typename Value>
    DataBlock_Impl<Key, Value>::~DataBlock_Impl() {
        if (!_need_put_back) return;
        _impl->put_block(_index, _need_update);
    }

    template <typename Key, typename Value>
    void DataBlock_Impl<Key, Value>::insert(const Iter& pos, const Key& key, const Value& value) {
        _need_update = true;
        uint32 value_size = ValueChecker::get_size(value);
        auto dest = Interpreter::move_ptr(_helper.buffer, pos._index_or_offset);
        uint32 behind_size = *_helper.get_size() + DataBlockHelper::BeginPos - pos._index_or_offset;
        std::memmove(Interpreter::move_ptr(dest, KeySize + value_size), dest, behind_size);
        KeyChecker::write_to(dest, key);
        ValueChecker::write_to(Interpreter::move_ptr(dest, KeySize), value);
        *_helper.get_size() += KeySize + value_size;
    }

    template <typename Key, typename Value>
    bool DataBlock_Impl<Key, Value>::update(const Iter& pos, const Value& value) {
        Value *val = get_value(_helper.buffer, pos._index_or_offset);
        int32 old_size = ValueChecker::get_size(*val), new_size = ValueChecker::get_size(value);
        if (new_size != old_size) {
            if (new_size - old_size > Interpreter::BLOCK_SIZE - *_helper.get_size() - DataBlockHelper::BeginPos)
                return false;
            auto dest = Interpreter::move_ptr(val, new_size);
            uint32 behind_size = *_helper.get_size() + DataBlockHelper::BeginPos
                - pos._index_or_offset - KeySize - old_size;
            std::memmove(dest, Interpreter::move_ptr(val, old_size), behind_size);
            *_helper.get_size() += new_size - old_size;
        }
        _need_update = true;
        ValueChecker::write_to(val, value);
        return true;
    }

    template <typename Key, typename Value>
    bool DataBlock_Impl<Key, Value>::erase(const Iter& iter) {
        _need_update = true;
        uint32 pair_size = KeySize + ValueChecker::get_size(iter.value());
        auto dest = Interpreter::move_ptr(_helper.buffer, iter._index_or_offset);
        uint32 behind_size = *_helper.get_size() + DataBlockHelper::BeginPos
            - iter._index_or_offset - pair_size;
        std::memmove(dest, Interpreter::move_ptr(dest, pair_size), behind_size);
        *_helper.get_size() -= pair_size;
        return *_helper.get_size() > (Interpreter::BLOCK_SIZE - DataBlockHelper::BeginPos) / 2;
    }

    template <typename Key, typename Value>
    void DataBlock_Impl<Key, Value>::erase(const Iter& begin, const Iter& end) {
        _need_update = true;
        auto dest = Interpreter::move_ptr(_helper.buffer, begin._index_or_offset),
             src = Interpreter::move_ptr(_helper.buffer, end._index_or_offset);
        uint32 behind_size = *_helper.get_size() + DataBlockHelper::BeginPos - end._index_or_offset;
        std::memmove(dest, src, behind_size);
        *_helper.get_size() -= end._index_or_offset - begin._index_or_offset;
    }

    template <typename Key, typename Value>
    void DataBlock_Impl<Key, Value>::merge(const DataBlock_Impl& other) {
        _need_update = true;
        auto dest = Interpreter::move_ptr(_helper.buffer, *_helper.get_size()
                                          + DataBlockHelper::BeginPos);
        auto src = Interpreter::move_ptr(other._helper.buffer, DataBlockHelper::BeginPos);
        std::memcpy(dest, src, *other._helper.get_size());
        *_helper.get_size() += *other._helper.get_size();
        *other._helper.get_size() = 0;
    }

    template <typename Key, typename Value>
    void DataBlock_Impl<Key, Value>::split(DataBlock_Impl& new_block, const Key& key, const Value& value) {
        _need_update = true;
        new_block._need_update = true;
        if constexpr (ValueChecker::is_fixed) {
            constexpr uint32 pair_size = KeySize + size_helper<Value>::size;
            uint32 this_size = *_helper.get_size();
            uint32 target_size = (this_size + pair_size) / pair_size / 2;
            uint32 target_pos = (lower_bound(key)._index_or_offset - DataBlockHelper::BeginPos) / pair_size;
            if (target_pos < target_size) {
                uint32 move_size = this_size - (target_size - 1) * pair_size;
                move_other_block_data(new_block, DataBlockHelper::BeginPos,
                                      *this, this_size + DataBlockHelper::BeginPos - move_size, move_size);
                Iter pos { false, true, target_pos * pair_size + DataBlockHelper::BeginPos,
                           _helper.buffer };
                insert(pos, key, value);
            } else {
                uint32 move_size = this_size - target_size * pair_size;
                move_other_block_data(new_block, DataBlockHelper::BeginPos,
                                      *this, this_size + DataBlockHelper::BeginPos - move_size, move_size);
                Iter pos { false, true, (target_pos - target_size) * pair_size + DataBlockHelper::BeginPos,
                           new_block._helper.buffer };
                new_block.insert(pos, key, value);
            }
        } else {
            uint32 kv_size = KeySize + ValueChecker::get_size(value);
            uint32 except_size = (*_helper.get_size() + kv_size) / 2;
            uint32 target_pos = DataBlockHelper::BeginPos, keep_size = 0, insert_pos = 0;
            for (uint32 pair_size = 0; keep_size < except_size; keep_size += pair_size, target_pos += pair_size) {
                pair_size = KeySize + ValueChecker::get_size(target_pos);
                if (insert_pos == 0 && *get_key(_helper.buffer, target_pos) > key) {
                    insert_pos = target_pos;
                    keep_size += kv_size;
                    if (keep_size >= except_size || keep_size + pair_size > Interpreter::BLOCK_SIZE) break;
                }
            }
            if (insert_pos != 0) keep_size -= kv_size;
            move_other_block_data(new_block, DataBlockHelper::BeginPos,
                                  *this, target_pos, *_helper.get_size() - keep_size);
            if (insert_pos != 0) insert(Iter { false, true, insert_pos, _helper.buffer }, key, value);
            else {
                Iter pos = new_block.lower_bound(key);
                new_block.insert(pos, key, value);
            }
        }
        *new_block._helper.prev_index() = _index;
        *new_block._helper.next_index() = *_helper.next_index();
        if (*_helper.next_index() > 0) {
            DataBlockHelper next { _impl->get_block(*_helper.next_index(), true) };
            *next.prev_index() = new_block._index;
            _impl->put_block(*_helper.next_index(), true);
        }
        *_helper.next_index() = new_block._index;
    }

    template <typename Key, typename Value>
    void DataBlock_Impl<Key, Value>::average(DataBlock_Impl& other) {
        if (*_helper.get_size() > *other._helper.get_size()) {
            if constexpr (ValueChecker::is_fixed) {
                constexpr uint32 pair_size = KeySize + size_helper<Value>::size;
                uint32 half_kv_pair = (*_helper.get_size() + *other._helper.get_size()) / pair_size / 2;
                uint32 move_size = *_helper.get_size() - half_kv_pair * pair_size;
                move_other_block_data(other, DataBlockHelper::BeginPos,
                                      *this, half_kv_pair * pair_size + DataBlockHelper::BeginPos, move_size);
            } else {
                uint32 target_size = (*_helper.get_size() + *other._helper.get_size()) / 2;
                uint32 rest_size = 0, pos = DataBlockHelper::BeginPos;
                for (uint32 kv_size = 0; rest_size < target_size; rest_size += kv_size, pos += kv_size) {
                    kv_size = KeySize + ValueChecker::get_size(*get_value(_helper.buffer, pos));
                }
                uint32 move_size = *_helper.get_size() - rest_size;
                if (*other._helper.get_size() + DataBlockHelper::BeginPos + move_size > Interpreter::BLOCK_SIZE)
                    return;
                move_other_block_data(other, DataBlockHelper::BeginPos,
                                      *this, pos, move_size);
            }
        } else {
            if constexpr (ValueChecker::is_fixed) {
                constexpr uint32 pair_size = KeySize + size_helper<Value>::size;
                uint32 half_kv_pair = (*_helper.get_size() + *other._helper.get_size()) / pair_size / 2;
                uint32 move_size = *other._helper.get_size() - half_kv_pair * pair_size;
                move_other_block_data(*this, DataBlockHelper::BeginPos + *_helper.get_size(),
                                      other, DataBlockHelper::BeginPos, move_size);
            } else {
                uint32 target_size = (*other._helper.get_size() - *_helper.get_size()) / 2;
                uint32 move_size = 0, pos = DataBlockHelper::BeginPos;
                for (uint32 kv_size = 0; move_size < target_size; move_size += kv_size, pos += kv_size) {
                    kv_size = KeySize + ValueChecker::get_size(*get_value(_helper.buffer, pos));
                }
                if (*_helper.get_size() + DataBlockHelper::BeginPos + move_size > Interpreter::BLOCK_SIZE)
                    return;
                move_other_block_data(*this, DataBlockHelper::BeginPos + *_helper.get_size(),
                                      other, DataBlockHelper::BeginPos, move_size);
            }
        }
    }

    template <typename Key, typename Value>
    int DataBlock_Impl<Key, Value>::merge_keep_average(const DataBlock_Impl& other) const {
        constexpr uint32 MaxBlockSize = Interpreter::BLOCK_SIZE - DataBlockHelper::BeginPos;
        uint32 this_size = *_helper.get_size(), other_size = *other._helper.get_size();
        if (this_size + other_size <= MaxBlockSize) return 1;
        uint32 difference = this_size > other_size ? this_size - other_size : other_size - this_size;
        if (difference < MaxBlockSize / 2) return 0;
        return -1;
    }

    template <typename Key, typename Value>
    typename DataBlock_Impl<Key, Value>::Iter
    DataBlock_Impl<Key, Value>::lower_bound(const Key& key) const {
        uint32 begin = DataBlockHelper::BeginPos, end = begin + *_helper.get_size();
        if constexpr (ValueChecker::is_fixed) {
            constexpr uint32 len = KeySize + size_helper<Value>::size;
            for (auto size = (end - begin) / len; size > 0; size = (end - begin) / len) {
                auto mid = begin + (size / 2) * len;
                if (*get_key(_helper.buffer, mid) < key) {
                    begin = mid + len;
                    continue;
                }
                end = mid;
                if (*get_key(_helper.buffer, mid) == key)
                    break;
            }
            return Iter(false, true, end, _helper.buffer);
        } else {
            while (begin < end && *get_key(_helper.buffer, begin) < key) {
                begin += KeySize + ValueChecker::get_size(*get_value(_helper.buffer, begin));
            }
            return Iter(false, true, begin, _helper.buffer);
        }
    }

    template <typename Key, typename Value>
    void DataBlock_Impl<Key, Value>::to_prev() {
        uint32 old_index = _index;
        _index = *_helper.prev_index();
        if (_need_put_back)
            _impl->put_block(old_index, _need_update);
        _need_put_back = true;
        _need_update = false;
        _helper.buffer = _impl->get_block(_index, true);
        check_is_DataBlock(_helper.buffer);
    }

    template <typename Key, typename Value>
    void DataBlock_Impl<Key, Value>::to_next() {
        uint32 old_index = _index;
        _index = *_helper.next_index();
        if (_need_put_back)
            _impl->put_block(old_index, _need_update);
        _need_put_back = true;
        _need_update = false;
        _helper.buffer = _impl->get_block(_index, true);
        check_is_DataBlock(_helper.buffer);
    }

    template <typename Key, typename Value>
    typename DataBlock_Impl<Key, Value>::Iter DataBlock_Impl<Key, Value>::begin() const {
        return Iter(false, true, DataBlockHelper::BeginPos, _helper.buffer);
    }

    template <typename Key, typename Value>
    typename DataBlock_Impl<Key, Value>::Iter DataBlock_Impl<Key, Value>::end() const {
        return Iter(false, true, DataBlockHelper::BeginPos + *_helper.get_size(), _helper.buffer);
    }

    template <typename Key, typename Value>
    typename DataBlock_Impl<Key, Value>::Iter DataBlock_Impl<Key, Value>::self_iter() const {
        return Iter(true, true, _index, nullptr);
    }

    template <typename Key, typename Value>
    bool DataBlock_Impl<Key, Value>::can_insert(const Value& value) const {
        uint32 size = KeySize + ValueChecker::get_size(value);
        return *_helper.get_size() + size + DataBlockHelper::BeginPos <= Interpreter::BLOCK_SIZE;
    }

    template <typename Key, typename Value>
    bool DataBlock_Impl<Key, Value>::have_prev() const {
        return *_helper.prev_index() != 0;
    }

    template <typename Key, typename Value>
    bool DataBlock_Impl<Key, Value>::have_next() const {
        return *_helper.next_index() != 0;
    }

    template <typename Key, typename Value>
    bool DataBlock_Impl<Key, Value>::data_size_check(const Value& value) {
        if constexpr (ValueChecker::is_fixed) {
            constexpr uint32 size = size_helper<Value>::size > Interpreter::PTR_SIZE
                                        ? size_helper<Value>::size : Interpreter::PTR_SIZE
                                        + KeySize;
            static_assert(size <= (Interpreter::BLOCK_SIZE - DataBlockHelper::BeginPos) / 2);
            return true;
        } else {
            uint32 val_size = ValueChecker::get_size(value);
            uint32 size = val_size > Interpreter::PTR_SIZE
                              ? val_size : Interpreter::PTR_SIZE
                              + KeySize;
            return size <= (Interpreter::BLOCK_SIZE - DataBlockHelper::BeginPos) / 2;
        }
    }

    template <typename Key, typename Value>
    Key* DataBlock_Impl<Key, Value>::get_key(void *buf, uint32 offset) {
        check_is_DataBlock(buf);
        return (Key *) Interpreter::move_ptr(buf, offset);
    }

    template <typename Key, typename Value>
    Value* DataBlock_Impl<Key, Value>::get_value(void *buf, uint32 offset) {
        check_is_DataBlock(buf);
        offset += KeySize;
        return (Value *) Interpreter::move_ptr(buf, offset);
    }

    template <typename Key, typename Value>
    const Key* DataBlock_Impl<Key, Value>::get_key(const void *buf, uint32 offset) {
        check_is_DataBlock(buf);
        return (const Key *) Interpreter::move_ptr(buf, offset);
    }

    template <typename Key, typename Value>
    const Value* DataBlock_Impl<Key, Value>::get_value(const void *buf, uint32 offset) {
        check_is_DataBlock(buf);
        offset += KeySize;
        return (const Value *) Interpreter::move_ptr(buf, offset);
    }

    template <typename Key, typename Value>
    uint32 DataBlock_Impl<Key, Value>::get_offset(const void *buf, uint32 from, int n) {
        if constexpr (ValueChecker::is_fixed) {
            constexpr uint32 size = KeySize + size_helper<Value>::size;
            return from + size * n;
        } else {
            if (n >= 0) {
                while (n > 0) {
                    from += KeySize + ValueChecker::get_size(*get_value(buf, from));
                    --n;
                }
                return from;
            }
            n = -n;
            auto arr = new uint32[n];
            uint32 pos = 0, index = DataBlockHelper::BeginPos;
            while (index < from) {
                arr[pos] = index;
                index += KeySize + ValueChecker::get_size(*get_value(buf, index));
                if (++pos == n) pos = 0;
            }
            index = arr[pos];
            delete[] arr;
            return index;
        }
    }

    template <typename Key, typename Value>
    void DataBlock_Impl<Key, Value>::erase_block(Impl_Scheduler& impl, uint32 index) {
        DataBlock_Impl data(impl, index);
        uint32 prev_index = *data._helper.prev_index(), next_index = *data._helper.next_index();
        if (prev_index != 0) {
            DataBlock_Impl prev(impl, prev_index);
            *prev._helper.next_index() = next_index;
            prev._need_update = true;
        }
        if (next_index != 0) {
            DataBlock_Impl next(impl, next_index);
            *next._helper.prev_index() = prev_index;
            next._need_update = true;
        }
    }

    template <typename Key, typename Value>
    void DataBlock_Impl<Key, Value>::move_other_block_data(DataBlock_Impl& dest, uint32 dest_pos,
                                                           DataBlock_Impl& src, uint32 src_pos, uint32 size) {
        uint32 dest_behind = *dest._helper.get_size() + DataBlockHelper::BeginPos - dest_pos;
        std::memmove(Interpreter::move_ptr(dest._helper.buffer, dest_pos + size),
                     Interpreter::move_ptr(dest._helper.buffer, dest_pos), dest_behind);
        std::memcpy(Interpreter::move_ptr(dest._helper.buffer, dest_pos),
                    Interpreter::move_ptr(src._helper.buffer, src_pos), size);
        *dest._helper.get_size() += size;
        dest._need_update = true;
        uint32 src_behind = *src._helper.get_size() + DataBlockHelper::BeginPos - src_pos - size;
        std::memmove(Interpreter::move_ptr(src._helper.buffer, src_pos),
                     Interpreter::move_ptr(src._helper.buffer, src_pos + size), src_behind);
        *src._helper.get_size() -= size;
        src._need_update = true;
    }

}

#endif

#endif //BASE_DATABLOCK_IMPL_HPP
