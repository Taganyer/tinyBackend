//
// Created by taganyer on 24-9-9.
//

#ifndef BASE_INDEXBLOCK_IMPL_HPP
#define BASE_INDEXBLOCK_IMPL_HPP

#ifdef BASE_INDEXBLOCK_IMPL_HPP

#include "BlockIter.hpp"

namespace Base {

    template <typename Key, typename Value>
    class IndexBlock_Impl : NoCopy {
        using Iter = BlockIter<Key, Value>;
        using KeyChecker = FixedSizeChecker<Key>;
        using IndexBlockHelper = Interpreter::IndexBlockHelper;

        static_assert(KeyChecker::is_fixed);

        static constexpr uint32 KeySize = size_helper<Key>::size;

        static constexpr uint32 PairSize = KeySize + Interpreter::PTR_SIZE;

    public:
        explicit IndexBlock_Impl(Impl_Scheduler &impl);

        explicit IndexBlock_Impl(Impl_Scheduler &impl, uint32 index);

        explicit IndexBlock_Impl(Impl_Scheduler &impl, uint32 index, void* buf);

        IndexBlock_Impl(IndexBlock_Impl &&other) noexcept;

        ~IndexBlock_Impl();

        IndexBlock_Impl& operator=(IndexBlock_Impl &&other) noexcept;

        void insert(const Iter &pos, const Key &key, const Iter &target);

        void update(const Iter &pos, const Key &key);

        bool erase(const Iter &iter);

        void erase(const Iter &begin, const Iter &end);

        void merge(const IndexBlock_Impl &other);

        void split(IndexBlock_Impl &new_block, const Key &key, const Iter &iter);

        void average(IndexBlock_Impl &other);

        int merge_keep_average(const IndexBlock_Impl &other) const;

        Iter lower_bound(const Key &key) const;

        [[nodiscard]] uint32 index() const { return _index; };

        [[nodiscard]] Iter begin() const;

        [[nodiscard]] Iter end() const;

        [[nodiscard]] Iter self_iter() const;

        [[nodiscard]] bool can_insert() const;

        static Key* get_key(void* buf, uint32 offset);

        static uint32* get_index(void* buf, uint32 offset);

        static const Key* get_key(const void* buf, uint32 offset);

        static const uint32* get_index(const void* buf, uint32 offset);

        static uint32 get_offset(const void* buf, uint32 from, int n);

    private:
        uint32 _index = 0;

        bool _need_update = false, _need_put_back = true;

        IndexBlockHelper _helper;

        Impl_Scheduler* _impl = nullptr;

        static void check_is_IndexBlock(const void *buffer) {
            if (unlikely(!Interpreter::is_index_block(buffer)))
                throw BPTreeRuntimeError("Attempting to transform non-IndexBlock to IndexBlock.");
        };

        static void move_other_block_data(IndexBlock_Impl &dest, uint32 dest_pos,
                                          IndexBlock_Impl &src, uint32 src_pos, uint32 size);
    };

    template <typename Key, typename Value>
    IndexBlock_Impl<Key, Value>::IndexBlock_Impl(Impl_Scheduler &impl) : _impl(&impl) {
        auto [i, p] = impl.get_block();
        _index = i;
        _helper.buffer = p;
        _helper.set_to_index_block();
        _need_update = true;
    }

    template <typename Key, typename Value>
    IndexBlock_Impl<Key, Value>::IndexBlock_Impl(Impl_Scheduler &impl, uint32 index) :
        _index(index), _impl(&impl) {
        _helper.buffer = impl.get_block(index, true);
        check_is_IndexBlock(_helper.buffer);
    }

    template <typename Key, typename Value>
    IndexBlock_Impl<Key, Value>::IndexBlock_Impl(Impl_Scheduler &impl, uint32 index, void* buf) :
        _index(index), _impl(&impl) {
        _helper.buffer = buf;
        _helper.set_to_index_block();
        _need_update = true;
    }

    template <typename Key, typename Value>
    IndexBlock_Impl<Key, Value>::IndexBlock_Impl(IndexBlock_Impl &&other) noexcept :
        _index(other._index), _need_update(other._need_update),
        _helper(other._helper), _impl(other._impl) {
        other._need_put_back = false;
    }

    template <typename Key, typename Value>
    IndexBlock_Impl<Key, Value>::~IndexBlock_Impl() {
        if (!_need_put_back) return;
        _impl->put_block(_index, _need_update);
    }

    template <typename Key, typename Value>
    IndexBlock_Impl<Key, Value>&
    IndexBlock_Impl<Key, Value>::operator=(IndexBlock_Impl &&other) noexcept {
        if (_need_put_back)
            _impl->put_block(_index, _need_update);
        _index = other._index;
        _need_update = other._need_update;
        _helper = other._helper;
        _impl = other._impl;
        other._need_put_back = false;
        _need_put_back = true;
        return *this;
    }

    template <typename Key, typename Value>
    void IndexBlock_Impl<Key, Value>::insert(const Iter &pos, const Key &key, const Iter &target) {
        _need_update = true;
        auto dest = Interpreter::move_ptr(_helper.buffer, pos._index_or_offset);
        uint32 behind_size = *_helper.get_size() + IndexBlockHelper::BeginPos - pos._index_or_offset;
        std::memmove(Interpreter::move_ptr(dest, PairSize), dest, behind_size);
        KeyChecker::write_to(dest, key);
        Interpreter::write_ptr(Interpreter::move_ptr(dest, KeySize), target.index());
        *_helper.get_size() += KeySize + Interpreter::PTR_SIZE;
    }

    template <typename Key, typename Value>
    void IndexBlock_Impl<Key, Value>::update(const Iter &pos, const Key &key) {
        _need_update = true;
        KeyChecker::write_to(get_key(_helper.buffer, pos._index_or_offset), key);
    }

    template <typename Key, typename Value>
    bool IndexBlock_Impl<Key, Value>::erase(const Iter &iter) {
        _need_update = true;
        auto dest = Interpreter::move_ptr(_helper.buffer, iter._index_or_offset);
        uint32 behind_size = *_helper.get_size() + IndexBlockHelper::BeginPos
            - iter._index_or_offset - PairSize;
        std::memmove(dest, Interpreter::move_ptr(dest, PairSize), behind_size);
        *_helper.get_size() -= PairSize;
        return *_helper.get_size() > (Interpreter::BLOCK_SIZE - IndexBlockHelper::BeginPos) / 2;
    }

    template <typename Key, typename Value>
    void IndexBlock_Impl<Key, Value>::erase(const Iter &begin, const Iter &end) {
        _need_update = true;
        auto dest = Interpreter::move_ptr(_helper.buffer, begin._index_or_offset),
             src = Interpreter::move_ptr(_helper.buffer, end._index_or_offset);
        uint32 behind_size = *_helper.get_size() + IndexBlockHelper::BeginPos - end._index_or_offset;
        std::memmove(dest, src, behind_size);
        *_helper.get_size() -= end._index_or_offset - begin._index_or_offset;
    }

    template <typename Key, typename Value>
    void IndexBlock_Impl<Key, Value>::merge(const IndexBlock_Impl &other) {
        _need_update = true;
        auto dest = Interpreter::move_ptr(_helper.buffer, *_helper.get_size()
                                          + IndexBlockHelper::BeginPos);
        auto src = Interpreter::move_ptr(other._helper.buffer, IndexBlockHelper::BeginPos);
        std::memcpy(dest, src, *other._helper.get_size());
        *_helper.get_size() += *other._helper.get_size();
        *other._helper.get_size() = 0;
    }

    template <typename Key, typename Value>
    void IndexBlock_Impl<Key, Value>::split(IndexBlock_Impl &new_block, const Key &key, const Iter &iter) {
        uint32 this_size = *_helper.get_size();
        uint32 target_size = (this_size + PairSize) / PairSize / 2;
        uint32 target_pos = (lower_bound(key)._index_or_offset - IndexBlockHelper::BeginPos) / PairSize;
        if (target_pos < target_size) {
            uint32 move_size = this_size - (target_size - 1) * PairSize;
            move_other_block_data(new_block, IndexBlockHelper::BeginPos,
                                  *this, this_size + IndexBlockHelper::BeginPos - move_size, move_size);
            Iter pos { false, false, target_pos * PairSize + IndexBlockHelper::BeginPos,
                _helper.buffer };
            insert(pos, key, iter);
        } else {
            uint32 move_size = this_size - target_size * PairSize;
            move_other_block_data(new_block, IndexBlockHelper::BeginPos,
                                  *this, this_size + IndexBlockHelper::BeginPos - move_size, move_size);
            Iter pos { false, false, (target_pos - target_size) * PairSize + IndexBlockHelper::BeginPos,
                new_block._helper.buffer };
            new_block.insert(pos, key, iter);
        }
    }

    template <typename Key, typename Value>
    void IndexBlock_Impl<Key, Value>::average(IndexBlock_Impl &other) {
        if (*_helper.get_size() > *other._helper.get_size()) {
            uint32 half_kv_pair = (*_helper.get_size() + *other._helper.get_size()) / PairSize / 2;
            uint32 move_size = *_helper.get_size() - half_kv_pair * PairSize;
            move_other_block_data(other, IndexBlockHelper::BeginPos,
                                  *this, half_kv_pair * PairSize + IndexBlockHelper::BeginPos, move_size);
        } else {
            uint32 half_kv_pair = (*_helper.get_size() + *other._helper.get_size()) / PairSize / 2;
            uint32 move_size = *other._helper.get_size() - half_kv_pair * PairSize;
            move_other_block_data(*this, IndexBlockHelper::BeginPos + *_helper.get_size(),
                                  other, IndexBlockHelper::BeginPos, move_size);
        }
    }

    template <typename Key, typename Value>
    int IndexBlock_Impl<Key, Value>::merge_keep_average(const IndexBlock_Impl &other) const {
        constexpr uint32 MaxBlockSize = Interpreter::BLOCK_SIZE - IndexBlockHelper::BeginPos;
        uint32 this_size = *_helper.get_size(), other_size = *other._helper.get_size();
        if (this_size + other_size <= MaxBlockSize) return 1;
        uint32 difference = this_size > other_size ? this_size - other_size : other_size - this_size;
        if (difference < MaxBlockSize / 2) return 0;
        return -1;
    }

    template <typename Key, typename Value>
    typename IndexBlock_Impl<Key, Value>::Iter
    IndexBlock_Impl<Key, Value>::lower_bound(const Key &key) const {
        uint32 begin = IndexBlockHelper::BeginPos, end = begin + *_helper.get_size();
        for (auto size = (end - begin) / PairSize; size > 0; size = (end - begin) / PairSize) {
            auto mid = begin + (size / 2) * PairSize;
            if (*get_key(_helper.buffer, mid) < key) {
                begin = mid + PairSize;
                continue;
            }
            end = mid;
            Key k = *get_key(_helper.buffer, mid);
            if (*get_key(_helper.buffer, mid) == key)
                break;
        }
        return Iter(false, false, end, _helper.buffer);
    }

    template <typename Key, typename Value>
    typename IndexBlock_Impl<Key, Value>::Iter IndexBlock_Impl<Key, Value>::begin() const {
        return Iter(false, false, IndexBlockHelper::BeginPos, _helper.buffer);
    }

    template <typename Key, typename Value>
    typename IndexBlock_Impl<Key, Value>::Iter IndexBlock_Impl<Key, Value>::end() const {
        return Iter(false, false, IndexBlockHelper::BeginPos + *_helper.get_size(), _helper.buffer);
    }

    template <typename Key, typename Value>
    typename IndexBlock_Impl<Key, Value>::Iter IndexBlock_Impl<Key, Value>::self_iter() const {
        return Iter(true, false, _index, nullptr);
    }

    template <typename Key, typename Value>
    bool IndexBlock_Impl<Key, Value>::can_insert() const {
        return *_helper.get_size() + PairSize + IndexBlockHelper::BeginPos <= Interpreter::BLOCK_SIZE;
    }

    template <typename Key, typename Value>
    Key* IndexBlock_Impl<Key, Value>::get_key(void* buf, uint32 offset) {
        check_is_IndexBlock(buf);
        return (Key *) Interpreter::move_ptr(buf, offset);
    }

    template <typename Key, typename Value>
    uint32* IndexBlock_Impl<Key, Value>::get_index(void* buf, uint32 offset) {
        check_is_IndexBlock(buf);;
        offset += KeySize;
        return (uint32 *) Interpreter::move_ptr(buf, offset);
    }

    template <typename Key, typename Value>
    const Key* IndexBlock_Impl<Key, Value>::get_key(const void* buf, uint32 offset) {
        check_is_IndexBlock(buf);;
        return (const Key *) Interpreter::move_ptr(buf, offset);
    }

    template <typename Key, typename Value>
    const uint32* IndexBlock_Impl<Key, Value>::get_index(const void* buf, uint32 offset) {
        check_is_IndexBlock(buf);;
        offset += KeySize;
        return (const uint32 *) Interpreter::move_ptr(buf, offset);
    }

    template <typename Key, typename Value>
    uint32 IndexBlock_Impl<Key, Value>::get_offset(const void* buf, uint32 from, int n) {
        check_is_IndexBlock(buf);;
        return from + n * PairSize;
    }

    template <typename Key, typename Value>
    void IndexBlock_Impl<Key, Value>::move_other_block_data(IndexBlock_Impl &dest, uint32 dest_pos,
                                                            IndexBlock_Impl &src, uint32 src_pos, uint32 size) {
        uint32 dest_behind = *dest._helper.get_size() + IndexBlockHelper::BeginPos - dest_pos;
        std::memmove(Interpreter::move_ptr(dest._helper.buffer, dest_pos + size),
                     Interpreter::move_ptr(dest._helper.buffer, dest_pos), dest_behind);
        std::memcpy(Interpreter::move_ptr(dest._helper.buffer, dest_pos),
                    Interpreter::move_ptr(src._helper.buffer, src_pos), size);
        *dest._helper.get_size() += size;
        dest._need_update = true;
        uint32 src_behind = *src._helper.get_size() + IndexBlockHelper::BeginPos - src_pos - size;
        std::memmove(Interpreter::move_ptr(src._helper.buffer, src_pos),
                     Interpreter::move_ptr(src._helper.buffer, src_pos + size), src_behind);
        *src._helper.get_size() -= size;
        src._need_update = true;
    }

}

#endif

#endif //BASE_INDEXBLOCK_IMPL_HPP
