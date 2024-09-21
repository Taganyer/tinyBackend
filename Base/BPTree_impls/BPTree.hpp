//
// Created by taganyer on 24-8-31.
//

#ifndef BASE_BPTREE_HPP
#define BASE_BPTREE_HPP

#include <cassert>
#include <utility>

#ifdef BASE_BPTREE_HPP

/*
 * Impl 要求存在: Key Value Result ResultSet DataBlock IndexBlock Iter IterKey
 *      需要调用的函数: begin_block() 返回指向 B+树的根节点 Iter.
 *                   set_begin_block(Iter) 将 Iter 指向的节点设置为根节点。
 *                   create_index_block() / create_data_block() 创建一个 IndexBlock / DataBlock 节点。
 *                   erase_block(Iter) 删除 Iter 指向的节点（当 Iter 所依赖的节点对象被销毁时，将会传入被销毁对象的 self_iter()）。
 *
 * Key: 能够通过 Iter 解应用、要求存在默认构造函数、复制构造函数、> < == != 比较方法。
 *
 * Value: 能够通过 Iter 解应用。
 *
 * Result: 返回查找的结果，要求存在默认构造函数（无效时），能够通过 Iter 构造。
 *
 * ResultSet: 返回查找的结果集，要求能够默认构造。
 *      需要调用的函数: add(Iter)
 *
 * DataBlock: B+树的叶节点，要求存在右值构造函数，能够通过 Iter.data_block(Impl &) 构造。
 *      需要调用的函数: begin() end() 返回 Iter 对象用来迭代该节点所拥有的键。
 *                   self_iter() 返回 Iter 对象，要求在 DataBlock 销毁后依旧能够表示本节点的位置（不要求其他功能）。
 *                   lower_bound(Key) 返回 iter 对象，指向第一个大于或等于 Key 的位置。
 *                   can_insert(Value) 返回 true 表示能够插入值。
 *                   insert(Iter-location, Key Value) 在 location 之前的位置插入键值。
 *                   update(Iter Value) 更新 Iter 位置的 Value，返回 true 表示更新成功。
 *                   erase(Iter) 删除 Iter 位置的键值，返回 true 表示不需要调整该节点键值数量。
 *                   erase(Iter begin, Iter end) 删除 [begin, end) 范围的键值。
 *                   split(DataBlock& other, Key, Value) 当一个节点无法插入键值时使用，期望在插入键值的同时，
 *                      将原节点内的键值均分给 other一半，并且两个节点的键维持有序（other 中所有键均大于本节点）。
 *                   average(DataBlock& other) 与 other 均分键值（other 中所有键均大于本节点）。
 *                   merge(DataBlock& other) 将 other 的所有键值合并到本节点中。
 *                   merge_keep_average(DataBlock&) 返回值大于0,两个节点会进行合并，等于0维持原状，小于0进行均分操作。
 *                   to_prev() to_next() 将本对象变为上一个 / 下一个节点。
 *                   have_prev() have_next() 判断是否有上一个 / 下一个节点。
 *
 * indexBlock: B+树的索引节点，要求存在右值构造函数，能够通过 Iter.index_block(Impl &) 构造。
*      需要调用的函数: begin() end() 返回 Iter 对象用来迭代该节点所拥有的键。
 *                   self_iter() 返回 Iter 对象，要求在 IndexBlock 销毁后依旧能够表示本节点的位置（不要求其他功能）。
 *                   lower_bound(Key) 返回 iter 对象，指向第一个大于或等于 Key 的位置。
 *                   can_insert() 返回 true 表示能够插入键。
 *                   insert(Iter-location, Key Value) 在 location 之前的位置插入键值。
 *                   update(Iter Key) 更新 Iter 位置的 Key。
 *                   erase(Iter) 删除 Iter 位置的键值，返回 true 表示不需要调整该节点键数量。
 *                   erase(Iter begin, Iter end) 删除 [begin, end) 范围的键。
 *                   split(DataBlock& other, Iter) 当一个节点无法插入键时使用，期望在插入键的同时，
 *                      将原节点内的键均分给 other一半，并且两个节点的键维持有序（other 中所有键均大于本节点）。
 *                   average(DataBlock& other) 与 other 均分键值（other 中所有键均大于本节点）。
 *                   merge(DataBlock& other) 将 other 的所有键合并到本节点中。
 *                   merge_keep_average(DataBlock&) 返回值大于0,两个节点会进行合并，等于0维持原状，小于0进行均分操作。
 *
 * Iter: 迭代器，要求存在默认构造函数（表无效），复制函数。
 *      需要调用的函数: valid() true 表示该迭代器有效。
 *                   key() 返回该位置的 Key 对象（多用于比较），当所依托的节点对象被销毁时不会被调用。
 *                   ++ -- 用于调整位置。
 *                   == != 用于比较。
 *                   is_data_block(Impl&) 用于判断所指向的节点是否为 DataBlock。
 *                   data_block(Impl&) 返回指向的 DataBlock 对象。
 *                   index_block(Impl&) 返回指向的 IndexBlock 对象。
 *
 * IterKey: 迭代器与值的集合，用来解决节点失效后保存其位置和需要的标识键。要求存在默认构造函数（无效时使用）。
 *
 */

namespace Base {

    template <typename Impl>
    class BPTree {
    public:
        using Key = typename Impl::Key;

        using Value = typename Impl::Value;

        using Result = typename Impl::Result;

        using ResultSet = typename Impl::ResultSet;

        enum State {
            Success,
            Fail
        };

        template <typename...Args>
        explicit BPTree(Args &&...args) : _impl(std::forward<Args>(args)...) {};

        /// 查找对应值。
        Result find(const Key &key);

        /// 查找 [begin, end) 范围的值，limit 为限制大小。
        ResultSet find(const Key &begin, const Key &end, uint64 limit = UINT64_MAX);

        /// 查找第一个大于等于 key 的值。
        Result lower_bound(const Key &key);

        /// 查找第一个大于 key 的值。
        Result upper_bound(const Key &key);

        /// 查找所有大于等于 key 的值，limit 为限制大小。
        ResultSet above_or_equal(const Key &key, uint64 limit = UINT64_MAX);

        /// 从头部查找所有小于 key 的值，limit 为限制大小。
        ResultSet below(const Key &key, uint64 limit = UINT64_MAX);

        /// 从头部开始查找，limit 为限制大小
        ResultSet search_from_begin(uint64 limit = UINT64_MAX);

        /// 从尾部开始查找，limit 为限制大小。
        ResultSet search_from_end(uint64 limit = UINT64_MAX);

        /// 更新对应键的值，键不存在或值长度过大会导致更新失败。
        bool update(const Key &key, const Value &value);

        /// 插入对应键值，键已存在或 value 长度过大会导致插入失败。
        bool insert(const Key &key, const Value &value);

        /// 删除对应键值，键不存在删除失败。
        bool erase(const Key &key);

        /// 删除 [begin, end) 范围内的键值，范围内不存在元素会导致删除失败。
        bool erase(const Key &begin, const Key &end);

    private:
        using DataBlock = typename Impl::DataBlock;

        using IndexBlock = typename Impl::IndexBlock;

        using Iter = typename Impl::Iter;

        using IterKey = typename Impl::IterKey;

        Iter find_data(const Key &key);

        IterKey insert_impl(const Key &key, const Value &value,
                            Iter block_iter, State &state);

        IterKey index_block_insert(const Key &key, const Value &value,
                                   Iter block_iter, State &state);

        IterKey data_block_insert(const Key &key, const Value &value,
                                  Iter block_iter, State &state);

        IterKey erase_impl(const Key &key, Iter iter_before,
                           Iter iter, State &state);

        IterKey index_block_erase(const Key &key, Iter iter_before,
                                  Iter iter, State &state);

        IterKey data_block_erase(const Key &key, Iter iter_before,
                                 Iter iter, State &state);

        using Pair = std::pair<IterKey, IterKey>;

        Pair erase_impl(const Key &begin, const Key &end,
                        Iter left, Iter right, State &state);

        Pair index_block_erase(const Key &begin, const Key &end,
                               Iter left, Iter right, State &state);

        Pair data_block_erase(const Key &begin, const Key &end,
                              Iter left, Iter right, State &state);

        void erase_all_block(Iter begin, Iter end);

    private:
        Impl _impl;

    };

}

namespace Base {

    template <typename Impl>
    typename BPTree<Impl>::Result
    BPTree<Impl>::find(const Key &key) {
        Iter iter = find_data(key);
        if (!iter.valid())
            return Result {};
        DataBlock data_block = iter.data_block(_impl);
        auto data_iter = data_block.lower_bound(key);
        if (data_iter == data_block.end() || data_iter.key() > key)
            return Result {};
        return Result { data_iter };
    }

    template <typename Impl>
    typename BPTree<Impl>::ResultSet
    BPTree<Impl>::find(const Key &begin, const Key &end, uint64 limit) {
        ResultSet results;
        if (begin > end || begin == end) return results;
        Iter iter = find_data(begin);
        if (!iter.valid()) return results;
        DataBlock data_block = iter.data_block(_impl);
        auto data_iter = data_block.lower_bound(begin);
        while (limit > 0) {
            if (data_iter == data_block.end() && data_block.have_next()) {
                data_block.to_next();
                data_iter = data_block.begin();
            }
            if (data_iter == data_block.end()) break;
            if (data_iter.key() < end) {
                results.add(data_iter);
                ++data_iter;
                --limit;
            } else break;
        }
        return results;
    }

    template <typename Impl>
    typename BPTree<Impl>::Result
    BPTree<Impl>::lower_bound(const Key &key) {
        Iter iter = find_data(key);
        if (!iter.valid()) return Result {};
        DataBlock data_block = iter.data_block(_impl);
        auto data_iter = data_block.lower_bound(key);
        if (data_iter == data_block.end()) {
            if (!data_block.have_next()) return Result {};
            data_block.to_next();
            data_iter = data_block.begin();
        }
        return Result { data_iter };
    }

    template <typename Impl>
    typename BPTree<Impl>::Result
    BPTree<Impl>::upper_bound(const Key &key) {
        Iter iter = find_data(key);
        if (!iter.valid()) return Result {};
        DataBlock data_block = iter.data_block(_impl);
        auto data_iter = data_block.lower_bound(key);
        while (true) {
            if (data_iter == data_block.end() && data_block.have_next()) {
                data_block.to_next();
                data_iter = data_block.begin();
            }
            if (data_iter == data_block.end()) return Result {};
            if (data_iter.key() > key) break;
            ++data_iter;
        }
        return Result { data_iter };
    }

    template <typename Impl>
    typename BPTree<Impl>::ResultSet
    BPTree<Impl>::above_or_equal(const Key &key, uint64 limit) {
        ResultSet results;
        Iter iter = find_data(key);
        if (!iter.valid()) return results;
        DataBlock data_block = iter.data_block(_impl);
        auto data_iter = data_block.lower_bound(key);
        while (limit && (data_iter != data_block.end()
            || data_block.have_next())) {
            if (data_iter == data_block.end()) {
                data_block.to_next();
                data_iter = data_block.begin();
                assert(data_block.end() != data_iter);
            }
            results.add(data_iter);
            --limit;
            ++data_iter;
        }
        return results;
    }

    template <typename Impl>
    typename BPTree<Impl>::ResultSet
    BPTree<Impl>::below(const Key &key, uint64 limit) {
        ResultSet results;
        Iter iter = _impl.begin_block();
        if (!iter.valid()) return results;
        if (iter.is_data_block(_impl)) {
            DataBlock data_block = iter.data_block(_impl);
            auto end = data_block.lower_bound(key);
            for (auto begin = data_block.begin(); limit && begin != end; ++begin) {
                results.add(begin);
                --limit;
            }
            return results;
        }

        IndexBlock index_block = iter.index_block(_impl);
        for (iter = index_block.begin();
             !iter.is_data_block(_impl);
             iter = index_block.begin()) {
            index_block = iter.index_block(_impl);
        }

        DataBlock data_block = iter.data_block(_impl);
        auto data_iter = data_block.begin();
        while (limit && ((data_iter != data_block.end() && data_iter.key() < key)
            || (data_iter == data_block.end() && data_block.have_next()))) {
            if (data_iter == data_block.end()) {
                data_block.to_next();
                data_iter = data_block.begin();
                assert(data_block.end() != data_iter);
            }
            results.add(data_iter);
            --limit;
            ++data_iter;
        }
        return results;
    }

    template <typename Impl>
    typename BPTree<Impl>::ResultSet
    BPTree<Impl>::search_from_begin(uint64 limit) {
        ResultSet results;
        Iter iter = _impl.begin_block();
        if (!iter.valid()) return results;
        if (iter.is_data_block(_impl)) {
            DataBlock data_block = iter.data_block(_impl);
            for (auto begin = data_block.begin(); limit && begin != data_block.end(); ++begin) {
                results.add(begin);
                --limit;
            }
            return results;
        }

        IndexBlock index_block = iter.index_block(_impl);
        for (iter = index_block.begin();
             !iter.is_data_block(_impl);
             iter = index_block.begin()) {
            index_block = iter.index_block(_impl);
        }

        DataBlock data_block = iter.data_block(_impl);
        auto data_iter = data_block.begin();
        while (limit && (data_iter != data_block.end() || data_block.have_next())) {
            if (data_iter == data_block.end()) {
                data_block.to_next();
                data_iter = data_block.begin();
                assert(data_block.end() != data_iter);
            }
            results.add(data_iter);
            ++data_iter;
            --limit;
        }
        return results;
    }

    template <typename Impl>
    typename BPTree<Impl>::ResultSet
    BPTree<Impl>::search_from_end(uint64 limit) {
        ResultSet results;
        Iter iter = _impl.begin_block();
        if (!iter.valid()) return results;
        if (iter.is_data_block(_impl)) {
            DataBlock data_block = iter.data_block(_impl);
            for (auto end = data_block.end(); limit && end != data_block.begin();) {
                results.add(--end);
                --limit;
            }
            return results;
        }

        IndexBlock index_block = iter.index_block(_impl);
        for (iter = --index_block.end();
             !iter.is_data_block(_impl);
             iter = --index_block.end()) {
            index_block = iter.index_block(_impl);
        }

        DataBlock data_block = iter.data_block(_impl);
        auto data_iter = data_block.end();
        while (limit && (data_iter != data_block.begin() || data_block.have_prev())) {
            if (data_iter == data_block.begin()) {
                data_block.to_prev();
                data_iter = data_block.end();
                assert(data_block.begin() != data_iter);
            }
            results.add(--data_iter);
            --limit;
        }
        return results;
    }

    template <typename Impl>
    bool BPTree<Impl>::update(const Key &key, const Value &value) {
        if (!DataBlock::data_size_check(value))
            return false;
        Iter iter = find_data(key);
        if (!iter.valid()) return false;
        DataBlock data_block = iter.data_block(_impl);
        auto data_iter = data_block.lower_bound(key);
        if (data_iter == data_block.end() || data_iter.key() != key) return false;
        return data_block.update(data_iter, value);
    }

    template <typename Impl>
    bool BPTree<Impl>::insert(const Key &key, const Value &value) {
        if (!DataBlock::data_size_check(value))
            return false;
        Iter iter = _impl.begin_block();
        State state { Fail };
        if (!iter.valid()) {
            DataBlock data_block = _impl.create_data_block();
            assert(data_block.can_insert(value));
            data_block.insert(data_block.end(), key, value);
            _impl.set_begin_block(data_block.self_iter());
            state = Success;
        } else {
            IterKey iter_key = insert_impl(key, value, iter, state);
            if (iter_key.valid()) {
                IndexBlock index_block = _impl.create_index_block();
                assert(index_block.can_insert());
                if (iter.is_data_block(_impl)) {
                    DataBlock before = iter.data_block(_impl);
                    index_block.insert(index_block.end(), before.begin().key(), before.self_iter());
                } else {
                    IndexBlock before = iter.index_block(_impl);
                    index_block.insert(index_block.end(), before.begin().key(), before.self_iter());
                }
                assert(index_block.can_insert());
                index_block.insert(index_block.end(), iter_key.key(), iter_key.iter());
                _impl.set_begin_block(index_block.self_iter());
            }
        }
        return state == Success;
    }

    template <typename Impl>
    bool BPTree<Impl>::erase(const Key &key) {
        Iter iter = _impl.begin_block();
        if (!iter.valid()) return false;
        State state = { Fail };
        IterKey iter_key = erase_impl(key, Iter(), iter, state);
        iter = iter_key.iter();
        if (!iter.valid()) {
            _impl.set_begin_block(Iter());
        } else if (!iter.is_data_block(_impl) && state == Success) {
            IndexBlock index_block = iter.index_block(_impl);
            auto t = index_block.begin();
            if (++t == index_block.end()) {
                _impl.set_begin_block(index_block.begin());
                _impl.erase_block(index_block.self_iter());
            }
        }
        return state == Success;
    }

    template <typename Impl>
    bool BPTree<Impl>::erase(const Key &begin, const Key &end) {
        Iter iter = _impl.begin_block();
        if (!iter.valid()) return false;
        State state = { Fail };
        auto [iter_key, _] = erase_impl(begin, end, iter, iter, state);
        if (!iter_key.valid() || iter != iter_key.iter()) {
            _impl.set_begin_block(iter_key.iter());
        }
        return state == Success;
    }

    template <typename Impl>
    typename BPTree<Impl>::Iter
    BPTree<Impl>::find_data(const Key &key) {
        Iter iter = _impl.begin_block();
        if (!iter.valid()) return {};
        if (iter.is_data_block(_impl)) return iter;
        IndexBlock index_block = iter.index_block(_impl);
        while (true) {
            iter = index_block.lower_bound(key);
            if (iter == index_block.end() ||
                (iter != index_block.begin() && iter.key() > key))
                --iter;
            if (iter.is_data_block(_impl)) break;
            index_block = iter.index_block(_impl);
        }
        if (iter == index_block.end() ||
            (iter != index_block.begin() && iter.key() > key))
            --iter;
        DataBlock data_block = iter.data_block(_impl);
        return data_block.self_iter();
    }

    template <typename Impl>
    typename BPTree<Impl>::IterKey
    BPTree<Impl>::insert_impl(const Key &key, const Value &value,
                              Iter block_iter, State &state) {
        if (block_iter.is_data_block(_impl)) {
            return data_block_insert(key, value, block_iter, state);
        } else {
            return index_block_insert(key, value, block_iter, state);
        }
    }

    template <typename Impl>
    typename BPTree<Impl>::IterKey
    BPTree<Impl>::index_block_insert(const Key &key, const Value &value,
                                     Iter block_iter, State &state) {
        IndexBlock index_block = block_iter.index_block(_impl);
        bool need_update = key < index_block.begin().key();
        auto iter = index_block.lower_bound(key);
        if (iter != index_block.end() && iter.key() == key) {
            state = Fail;
            return IterKey {};
        }
        if (iter != index_block.begin()) --iter;
        assert(iter != index_block.end());
        IterKey key_iter = insert_impl(key, value, iter, state);
        if (need_update && state == Success) index_block.update(iter, key);
        if (key_iter.valid()) {
            if (index_block.can_insert()) {
                index_block.insert(++iter, key_iter.key(), key_iter.iter());
            } else {
                IndexBlock new_index_block = _impl.create_index_block();
                index_block.split(new_index_block, key_iter.key(), key_iter.iter());
                return IterKey { new_index_block.self_iter(), new_index_block.begin().key() };
            }
        }
        return IterKey {};
    }

    template <typename Impl>
    typename BPTree<Impl>::IterKey
    BPTree<Impl>::data_block_insert(const Key &key, const Value &value,
                                    Iter block_iter, State &state) {
        DataBlock data_block = block_iter.data_block(_impl);
        auto iter = data_block.lower_bound(key);
        if (iter != data_block.end() && iter.key() == key) {
            state = Fail;
            return {};
        }
        if (data_block.can_insert(value)) {
            data_block.insert(iter, key, value);
            state = Success;
            return {};
        } else {
            DataBlock new_data_block = _impl.create_data_block();
            data_block.split(new_data_block, key, value);
            state = Success;
            return IterKey { new_data_block.self_iter(), new_data_block.begin().key() };
        }
    }

    template <typename Impl>
    typename BPTree<Impl>::IterKey
    BPTree<Impl>::erase_impl(const Key &key, Iter iter_before, Iter iter, State &state) {
        if (iter.is_data_block(_impl)) {
            return data_block_erase(key, iter_before, iter, state);
        } else {
            return index_block_erase(key, iter_before, iter, state);
        }
    }

    template <typename Impl>
    typename BPTree<Impl>::IterKey
    BPTree<Impl>::index_block_erase(const Key &key, Iter iter_before, Iter iter, State &state) {
        IndexBlock index_block = iter.index_block(_impl);
        auto iter_val = index_block.lower_bound(key);
        if (iter_val == index_block.begin() && iter_val.key() > key)
            return IterKey { index_block.self_iter(), index_block.begin().key() };
        if (iter_val == index_block.end() || iter_val.key() > key) --iter_val;

        Iter before;
        if (iter_val != index_block.begin()) {
            before = iter_val;
            --before;
        }
        IterKey iter_key = erase_impl(key, before, iter_val, state);
        if (state != State::Success)
            return IterKey { index_block.self_iter(), index_block.begin().key() };

        if (iter_key.valid()) {
            if (iter_val.key() != iter_key.key()) {
                index_block.update(iter_val, iter_key.key());
            }
        } else {
            if (index_block.erase(iter_val) || !iter_before.valid()) {
                if (index_block.begin() == index_block.end()) {
                    _impl.erase_block(index_block.self_iter());
                    return IterKey {};
                }
                return IterKey { index_block.self_iter(), index_block.begin().key() };
            }

            IndexBlock before_index_block = iter_before.index_block(_impl);
            int choose = before_index_block.merge_keep_average(index_block);
            if (choose > 0) {
                before_index_block.merge(index_block);
                _impl.erase_block(index_block.self_iter());
                return IterKey {};
            }
            if (choose < 0)
                before_index_block.average(index_block);
        }
        return IterKey { index_block.self_iter(), index_block.begin().key() };
    }

    template <typename Impl>
    typename BPTree<Impl>::IterKey
    BPTree<Impl>::data_block_erase(const Key &key, Iter iter_before, Iter iter, State &state) {
        DataBlock data_block = iter.data_block(_impl);
        Iter iter_val = data_block.lower_bound(key);
        if (iter_val == data_block.end() || iter_val.key() != key)
            return IterKey { data_block.self_iter(), data_block.begin().key() };

        state = State::Success;
        if (data_block.erase(iter_val) || !iter_before.valid()) {
            if (data_block.begin() == data_block.end()) {
                _impl.erase_block(data_block.self_iter());
                return IterKey {};
            }
            return IterKey { data_block.self_iter(), data_block.begin().key() };
        }

        DataBlock before_data_block = iter_before.data_block(_impl);
        int choose = before_data_block.merge_keep_average(data_block);
        if (choose > 0) {
            before_data_block.merge(data_block);
            _impl.erase_block(data_block.self_iter());
            return IterKey {};
        }
        if (choose < 0)
            before_data_block.average(data_block);
        return IterKey { data_block.self_iter(), data_block.begin().key() };
    }

    template <typename Impl>
    typename BPTree<Impl>::Pair
    BPTree<Impl>::erase_impl(const Key &begin, const Key &end,
                             Iter left, Iter right, State &state) {
        if (left.is_data_block(_impl)) {
            return data_block_erase(begin, end, left, right, state);
        } else {
            return index_block_erase(begin, end, left, right, state);
        }
    }

    template <typename Impl>
    typename BPTree<Impl>::Pair
    BPTree<Impl>::index_block_erase(const Key &begin, const Key &end,
                                    Iter left, Iter right, State &state) {
        if (left == right) {
            IndexBlock index_block = left.index_block(_impl);
            Iter li = index_block.lower_bound(begin),
                 ri = index_block.lower_bound(end);
            if (li == index_block.end() || li != index_block.begin() && li.key() != begin)
                --li;
            if (li != ri && (ri == index_block.end() || ri.key() != end))
                --ri;

            auto [ik_l, ik_r] = erase_impl(begin, end, li, ri, state);
            if (li != ri) {
                if (ik_l.valid()) {
                    if (li.key() != ik_l.key())
                        index_block.update(li, ik_l.key());
                    if (ik_r.valid()) {
                        if (ri.key() != ik_r.key())
                            index_block.update(ri, ik_r.key());
                    } else {
                        index_block.erase(ri);
                    }
                } else {
                    if (ik_r.valid()) {
                        if (ri.key() != ik_r.key())
                            index_block.update(ri, ik_r.key());
                    } else {
                        index_block.erase(ri);
                    }
                    index_block.erase(li);
                }

                Iter eb = index_block.lower_bound(begin),
                     ee = index_block.lower_bound(end);
                erase_all_block(eb, ee);
                index_block.erase(eb, ee);
            } else {
                if (ik_l.valid()) {
                    if (li.key() != ik_l.key())
                        index_block.update(li, ik_l.key());
                } else {
                    index_block.erase(li);
                }
            }

            if (index_block.begin() == index_block.end()) {
                _impl.erase_block(index_block.self_iter());
                return Pair { IterKey {}, IterKey {} };
            }
            return Pair { IterKey { index_block.self_iter(), index_block.begin().key() },
                IterKey { index_block.self_iter(), index_block.begin().key() } };
        } else {
            IndexBlock lb = left.index_block(_impl), rb = right.index_block(_impl);
            Iter li = lb.lower_bound(begin),
                 ri = rb.lower_bound(end);
            if (li == lb.end() || li != lb.begin() && li.key() != begin)
                --li;
            if (ri == rb.end() || ri != rb.begin() && ri.key() != end)
                --ri;

            auto [ik_l, ik_r] = erase_impl(begin, end, li, ri, state);
            if (ik_l.valid()) {
                if (li.key() != ik_l.key())
                    lb.update(li, ik_l.key());
            } else {
                lb.erase(li);
            }
            if (ik_r.valid()) {
                if (ri.key() != ik_r.key())
                    rb.update(ri, ik_r.key());
            } else {
                rb.erase(ri);
            }

            Iter eb = lb.lower_bound(begin),
                 ee = rb.lower_bound(end);
            erase_all_block(eb, lb.end());
            lb.erase(eb, lb.end());
            erase_all_block(rb.begin(), ee);
            rb.erase(rb.begin(), ee);

            int choose = lb.merge_keep_average(rb);
            if (choose > 0) {
                if (lb.begin() != lb.end()) {
                    lb.merge(rb);
                    _impl.erase_block(rb.self_iter());
                    return Pair { IterKey { lb.self_iter(), lb.begin().key() },
                        IterKey {} };
                }
                _impl.erase_block(lb.self_iter());
                if (rb.begin() == rb.end()) {
                    _impl.erase_block(rb.self_iter());
                    return Pair { IterKey {}, IterKey {} };
                }
                return Pair { IterKey {},
                    IterKey { rb.self_iter(), rb.begin().key() } };
            }
            if (choose < 0)
                lb.average(rb);
            return Pair { IterKey { lb.self_iter(), lb.begin().key() },
                IterKey { rb.self_iter(), rb.begin().key() } };
        }
    }

    template <typename Impl>
    typename BPTree<Impl>::Pair
    BPTree<Impl>::data_block_erase(const Key &begin, const Key &end,
                                   Iter left, Iter right, State &state) {
        state = State::Success;
        if (left == right) {
            DataBlock data_block = left.data_block(_impl);
            Iter b = data_block.lower_bound(begin),
                 e = data_block.lower_bound(end);
            if (b == e) state = State::Fail;
            else data_block.erase(b, e);
            if (data_block.begin() == data_block.end()) {
                _impl.erase_block(data_block.self_iter());
                return Pair { IterKey {}, IterKey {} };
            }
            return Pair { IterKey { data_block.self_iter(), data_block.begin().key() },
                IterKey { data_block.self_iter(), data_block.begin().key() } };
        } else {
            DataBlock lb = left.data_block(_impl), rb = right.data_block(_impl);
            lb.erase(lb.lower_bound(begin), lb.end());
            rb.erase(rb.begin(), rb.lower_bound(end));

            int choose = lb.merge_keep_average(rb);
            if (choose > 0) {
                if (lb.begin() != lb.end()) {
                    lb.merge(rb);
                    _impl.erase_block(rb.self_iter());
                    return Pair { IterKey { lb.self_iter(), lb.begin().key() },
                        IterKey {} };
                }

                _impl.erase_block(lb.self_iter());
                if (rb.begin() == rb.end()) {
                    _impl.erase_block(rb.self_iter());
                    return Pair { IterKey {}, IterKey {} };
                }
                return Pair { IterKey {},
                    IterKey { rb.self_iter(), rb.begin().key() } };
            }
            if (choose < 0)
                lb.average(rb);
            return Pair { IterKey { lb.self_iter(), lb.begin().key() },
                IterKey { rb.self_iter(), rb.begin().key() } };
        }
    }

    template <typename Impl>
    void BPTree<Impl>::erase_all_block(Iter begin, Iter end) {
        while (begin != end) {
            if (!begin.is_data_block(_impl)) {
                IndexBlock index_block = begin.index_block(_impl);
                erase_all_block(index_block.begin(), index_block.end());
            }
            _impl.erase_block(begin);
            ++begin;
        }
    }

}

#endif

#endif //BASE_BPTREE_HPP
