//
// Created by taganyer on 24-8-31.
//

#ifndef BASE_BPTREE_HPP
#define BASE_BPTREE_HPP

#include <cassert>
#include <utility>

#ifdef BASE_BPTREE_HPP

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
            Fail,
            DataInvalid
        };

        template <typename...Args>
        explicit BPTree(Args &&...args) : _impl(std::forward<Args>(args)...) {};

        Result find(const Key &key);

        ResultSet find(const Key &begin, const Key &end);

        Result lower_bound(const Key &key);

        Result upper_bound(const Key &key);

        bool update(const Key &key, const Value &value);

        bool insert(const Key &key, const Value &value);

        bool erase(const Key &key);

        bool erase(const Key &begin, const Key &end);

    private:
        using IndexBlock = typename Impl::IndexBlock;

        using DataBlock = typename Impl::DataBlock;

        using Iter = typename Impl::Iter;

        template <typename Iter>
        Iter lower_bound(Iter begin, Iter end, const Key &key) {
            for (auto size = end - begin; size > 0; size = end - begin) {
                auto mid = begin + (size / 2);
                if (mid.key() == key) return mid;
                if (mid.key() < key) begin = ++mid;
                else end = mid;
            }
            return end;
        };

        Iter find_data(const Key &key);

        Iter insert_impl(const Key &key, const Value &value,
                         Iter block_iter, State &state);

        Iter index_block_insert(const Key &key, const Value &value,
                                Iter block_iter, State &state);

        Iter data_block_insert(const Key &key, const Value &value,
                               Iter block_iter, State &state);

        Iter erase_impl(const Key &key, Iter iter_before,
                        Iter iter, State &state);

        Iter index_block_erase(const Key &key, Iter iter_before,
                               Iter iter, State &state);

        Iter data_block_erase(const Key &key, Iter iter_before,
                              Iter iter, State &state);

        using IterPair = std::pair<Iter, Iter>;

        IterPair erase_impl(const Key &begin, const Key &end,
                            Iter left, Iter right, State &state);

        IterPair index_block_erase(const Key &begin, const Key &end,
                                   Iter left, Iter right, State &state);

        IterPair data_block_erase(const Key &begin, const Key &end,
                                  Iter left, Iter right, State &state);

        void erase_all_block(Iter begin, Iter end);

    private:
        Impl _impl;

    };

}

namespace Base {

    /*
     * Impl: begin_block() set_begin_block(iter)
     *       erase_block(iter) create_index_block() create_data_block()
     *
     * IndexBlock: begin() end() self_iter()
     *             can_insert(iter) insert(iter-location iter-target) update(Iter, Key)
     *             bool-erase(iter) erase(iter iter)
     *             merge(IndexBlock) average(IndexBlock) bool-erase(iter) merge_keep_average(IndexBlock)
     *
     * DataBlock: begin() end() self_iter()
     *            to_last() to_next() have_last() have_next()
     *            can_insert(k, v) insert(iter, k, v) bool-update(iter Value)
     *            bool-erase(iter) erase(iter iter)
     *            merge_keep_average(DataBlock) merge(DataBlock) average(DataBlock)
     *
     * Iter: () key() valid() ++ -- = == != + -
     *       要求对象即使销毁后仍旧能调用 key()，并保留之前的位置。
     *
     * Key: < == >
     *
     * Result: () (iter)
     *
     * ResultSet: () add(data_iter)
     *
     */
    template <typename Impl>
    typename BPTree<Impl>::Result
    BPTree<Impl>::find(const Key &key) {
        Iter iter = find_data(key);
        if (!iter.valid()) return Result {};
        DataBlock data_block = iter.data_block();
        auto data_iter = lower_bound(data_block.begin(), data_block.end(), key);
        if (data_iter == data_block.end() || data_iter.key() > key) return Result {};
        return { data_iter };
    }

    template <typename Impl>
    typename BPTree<Impl>::ResultSet
    BPTree<Impl>::find(const Key &begin, const Key &end) {
        ResultSet results;
        if (begin > end || begin == end) return results;
        Iter iter = find_data(begin);
        if (!iter.valid()) return results;
        DataBlock data_block = iter.data_block();
        auto data_iter = lower_bound(data_block.begin(), data_block.end(), begin);
        while (true) {
            if (data_iter == data_block.end() && data_block.have_next()) {
                data_block.to_next();
                data_iter = data_block.begin();
            }
            if (data_iter == data_block.end()) break;
            if (data_iter.key() < end) {
                results.add(data_iter);
                ++data_iter;
            } else break;
        }
        return results;
    }

    template <typename Impl>
    typename BPTree<Impl>::Result
    BPTree<Impl>::lower_bound(const Key &key) {
        Iter iter = find_data(key);
        if (!iter.valid()) return Result {};
        DataBlock data_block = iter.data_block();
        auto data_iter = lower_bound(data_block.begin(), data_block.end(), key);
        if (data_iter == data_block.end() && data_block.have_next()) {
            data_block.to_next();
            data_iter = data_block.begin();
        } else return Result {};
        return { data_iter };
    }

    template <typename Impl>
    typename BPTree<Impl>::Result
    BPTree<Impl>::upper_bound(const Key &key) {
        Iter iter = find_data(key);
        if (!iter.valid()) return Result {};
        DataBlock data_block = iter.data_block();
        auto data_iter = lower_bound(data_block.begin(), data_block.end(), key);
        while (true) {
            if (data_iter == data_block.end() && data_block.have_next()) {
                data_block.to_next();
                data_iter = data_block.begin();
            }
            if (data_iter == data_block.end()) return Result {};
            if (data_iter.key() > key) break;
            ++data_iter;
        }
        return { data_iter };
    }


    template <typename Impl>
    bool BPTree<Impl>::update(const Key &key, const Value &value) {
        Iter iter = find_data(key);
        if (!iter.valid()) return false;
        DataBlock data_block = iter.data_block();
        auto data_iter = lower_bound(data_block.begin(), data_block.end(), key);
        if (data_iter == data_block.end() || data_iter.key() != key) return false;
        return data_block.update(data_iter, value);
    }

    template <typename Impl>
    bool BPTree<Impl>::insert(const Key &key, const Value &value) {
        Iter iter = _impl.begin_block();
        State state { Fail };
        if (!iter.valid()) {
            DataBlock data_block = _impl.create_data_block();
            assert(data_block.can_insert(key, value));
            data_block.insert(data_block.end(), key, value);
            _impl.set_begin_block(data_block.self_iter());
        } else {
            Iter new_iter = insert_impl(key, value, iter, state);
            if (new_iter.valid()) {
                IndexBlock index_block = _impl.create_index_block();
                assert(index_block.can_insert(iter));
                index_block.insert(index_block.end(), iter);
                assert(index_block.can_insert(new_iter));
                index_block.insert(index_block.end(), new_iter);
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
        iter = erase_impl(key, Iter(), iter, state);
        if (!iter.valid()) {
            _impl.set_begin_block(Iter());
        } else if (!iter.is_data_block() && state == Success) {
            IndexBlock index_block = iter.index_block();
            auto t = index_block.begin();
            if (++t == index_block.end()) {
                _impl.set_begin_block(index_block.begin());
                _impl.erase_block(iter);
            }
        }
        return state == Success;
    }

    template <typename Impl>
    bool BPTree<Impl>::erase(const Key &begin, const Key &end) {
        Iter iter = _impl.begin_block();
        State state = { Fail };
        auto [iter_ret, _] = erase_impl(begin, end, iter, iter, state);
        if (!iter_ret.valid() || iter != iter_ret) {
            _impl.set_begin_block(iter_ret);
        }
        return state == Success;
    }

    template <typename Impl>
    typename BPTree<Impl>::Iter
    BPTree<Impl>::find_data(const Key &key) {
        Iter iter = _impl.begin_block();
        if (!iter.valid() || iter.key() > key) return {};
        if (iter.is_data_block()) return iter;
        IndexBlock index_block = iter.index_block();
        for (iter = lower_bound(index_block.begin(), index_block.end(), key);
             !iter.is_data_block();
             iter = lower_bound(index_block.begin(), index_block.end(), key)) {
            if (iter == index_block.end() || iter.key() > key) --iter;
            index_block = iter.index_block();
        }
        assert(!(iter == index_block.begin() && iter.key() > key));
        if (iter == index_block.end() || iter.key() > key) --iter;
        return iter;
    }

    template <typename Impl>
    typename BPTree<Impl>::Iter
    BPTree<Impl>::insert_impl(const Key &key, const Value &value,
                              Iter block_iter, State &state) {
        if (block_iter.is_data_block()) {
            return data_block_insert(key, value, block_iter, state);
        } else {
            return index_block_insert(key, value, block_iter, state);
        }
    }

    template <typename Impl>
    typename BPTree<Impl>::Iter
    BPTree<Impl>::index_block_insert(const Key &key, const Value &value,
                                     Iter block_iter, State &state) {
        IndexBlock index_block = block_iter.index_block();
        bool need_update = key < block_iter.key();
        auto iter = lower_bound(index_block.begin(), index_block.end(), key);
        if (iter != index_block.begin()) --iter;
        assert(iter == index_block.end());
        if (iter.key() == key) {
            state = Fail;
            return {};
        }
        Iter new_iter = insert_impl(key, value, &index_block, iter, state);
        if (need_update && state == Success) index_block.update(iter, key);
        if (new_iter.valid()) {
            if (index_block.can_insert(new_iter)) {
                index_block.insert(++iter, new_iter);
            } else {
                IndexBlock new_index_block = _impl.create_index_block();
                index_block.average(new_index_block, new_iter);
                return new_index_block.self_iter();
            }
        }
        return {};
    }

    template <typename Impl>
    typename BPTree<Impl>::Iter
    BPTree<Impl>::data_block_insert(const Key &key, const Value &value,
                                    Iter block_iter, State &state) {
        DataBlock data_block = block_iter.data_block();
        auto iter = lower_bound(data_block.begin(), data_block.end(), key);
        if (iter != data_block.end() && iter.key() == key) {
            state = Fail;
            return {};
        }
        if (data_block.can_insert(key, value)) {
            data_block.insert(iter, key, value);
            state = Success;
            return {};
        } else {
            DataBlock new_data_block = _impl.create_data_block();
            assert(new_data_block.can_insert(key, value));
            new_data_block.insert(new_data_block.begin(), key, value);
            data_block.average(new_data_block);
            state = Success;
            return new_data_block.self_iter();
        }
    }

    template <typename Impl>
    typename BPTree<Impl>::Iter
    BPTree<Impl>::erase_impl(const Key &key, Iter iter_before, Iter iter, State &state) {
        if (iter.is_data_block()) {
            return data_block_erase(key, iter_before, iter, state);
        } else {
            return index_block_erase(key, iter_before, iter, state);
        }
    }

    template <typename Impl>
    typename BPTree<Impl>::Iter
    BPTree<Impl>::index_block_erase(const Key &key, Iter iter_before, Iter iter, State &state) {
        IndexBlock index_block = iter.index_block();
        auto iter_val = lower_bound(index_block.begin(), index_block.end(), key);
        if (iter_val == index_block.begin() || iter_val.key() > key) return iter;
        if (iter_val == index_block.end() || iter_val.key() > key) --iter;

        Iter before;
        if (iter_val != index_block.begin()) {
            before = iter_val;
            --before;
        }
        Iter ret_iter = erase_impl(key, before, iter_val, state);
        if (state != State::Success) return iter;

        if (ret_iter.valid()) {
            if (iter_val.key() != ret_iter.key()) {
                index_block.update(iter_val, ret_iter.key());
            }
        } else {
            if (!index_block.erase(iter_val) || !iter_before.valid()) {
                if (index_block.begin() == index_block.end()) {
                    _impl.erase(iter);
                    return {};
                }
                return index_block.begin();
            }

            IndexBlock before_index_block = iter_before.index_block();
            int choose = before_index_block.merge_keep_average(index_block);
            if (choose > 0) {
                before_index_block.merge(index_block);
                _impl.erase_block(iter);
                return {};
            }
            if (choose < 0)
                before_index_block.average(index_block);
        }
        return index_block.begin();
    }

    template <typename Impl>
    typename BPTree<Impl>::Iter
    BPTree<Impl>::data_block_erase(const Key &key, Iter iter_before, Iter iter, State &state) {
        DataBlock data_block = iter.data_block();
        Iter iter_val = lower_bound(data_block.begin(), data_block.end(), key);
        if (iter_val == data_block.end() || iter_val.key() != key) return iter;

        state = State::Success;
        if (!data_block.erase(iter_val) || !iter_before.valid()) {
            if (data_block.begin() == data_block.end()) {
                _impl.erase_block(iter);
                return {};
            }
            return data_block.begin();
        }

        DataBlock before_data_block = iter_before.data_block();
        int choose = before_data_block.merge_keep_average(data_block);
        if (choose > 0) {
            before_data_block.merge(data_block);
            _impl.erase_block(iter);
            return {};
        }
        if (choose < 0)
            before_data_block.average(data_block);
        return data_block.begin();
    }

    template <typename Impl>
    typename BPTree<Impl>::IterPair
    BPTree<Impl>::erase_impl(const Key &begin, const Key &end,
                             Iter left, Iter right, State &state) {
        if (left.is_data_block()) {
            return data_block_erase(begin, end, left, right, state);
        } else {
            return index_block_erase(begin, end, left, right, state);
        }
    }

    template <typename Impl>
    typename BPTree<Impl>::IterPair
    BPTree<Impl>::index_block_erase(const Key &begin, const Key &end,
                                    Iter left, Iter right, State &state) {
        if (left == right) {
            IndexBlock index_block = left.index_block();
            Iter li = lower_bound(index_block.begin(), index_block.end(), begin),
                 ri = lower_bound(index_block.begin(), index_block.end(), end);
            if (li != index_block.begin() && (li == index_block.end() || li.key() != begin)) --li;
            if (li != ri && (ri == index_block.end() || ri.key() != end)) --ri;

            auto [ret_l, ret_r] = erase_impl(begin, end, li, ri, state);
            if (li != ri) {
                if (ret_l.valid()) {
                    if (li.key() != ret_l.key())
                        index_block.update(li, ret_l.key());
                } else {
                    index_block.erase(li);
                }
                if (ret_r.valid()) {
                    if (ri.key() != ret_r.key())
                        index_block.update(ri, ret_r.key());
                } else {
                    index_block.erase(ri);
                }

                Iter eb = lower_bound(index_block.begin(), index_block.end(), begin),
                     ee = lower_bound(index_block.begin(), index_block.end(), end);
                erase_all_block(eb, ee);
                index_block.erase(eb, ee);
            } else {
                if (ret_l.valid()) {
                    if (li.key() != ret_l.key())
                        index_block.update(li, ret_l.key());
                } else {
                    index_block.erase(li);
                }
            }

            if (index_block.begin() == index_block.end()) {
                _impl.erase_block(left);
                return IterPair { Iter {}, Iter {} };
            }
            if (++index_block.begin() == index_block.end()) {
                Iter ret = index_block.begin();
                _impl.erase_block(left);
                return IterPair { ret, ret };
            }
            return Iter { index_block.self_iter, index_block.self_iter };
        } else {
            IndexBlock lb = left.index_block(), rb = right.index_block();
            Iter li = lower_bound(lb.begin(), lb.end(), begin),
                 ri = lower_bound(rb.begin(), rb.end(), end);
            if (li != lb.begin() && (li == lb.end() || li.key() != begin)) --li;
            if (ri != rb.begin() && (ri == rb.end() || ri.key() != end)) --ri;

            auto [ret_l, ret_r] = erase_impl(begin, end, li, ri, state);
            if (ret_l.valid()) {
                if (li.key() != ret_l.key())
                    lb.update(li, ret_l.key());
            } else {
                lb.erase(li);
            }
            if (ret_r.valid()) {
                if (ri.key() != ret_r.key())
                    rb.update(ri, ret_r.key());
            } else {
                rb.erase(ri);
            }

            Iter eb = lower_bound(lb.begin(), lb.end(), begin),
                 ee = lower_bound(rb.begin(), rb.end(), end);
            erase_all_block(eb, lb.end());
            lb.erase(eb, lb.end());
            erase_all_block(rb.begin(), ee);
            rb.erase(rb.begin(), ee);

            int choose = lb.merge_keep_average(rb);
            if (choose > 0) {
                if (lb.begin() != lb.end()) {
                    lb.merge(rb);
                    _impl.erase_block(right);
                    return IterPair { lb.self_iter(), Iter {} };
                }
                _impl.erase_block(left);
                if (rb.begin() == rb.end()) {
                    _impl.erase_block(right);
                    return IterPair { Iter {}, Iter {} };
                }
                return IterPair { Iter {}, rb.self_iter() };
            }
            if (choose < 0)
                lb.average(rb);
            return IterPair { lb.self_iter(), rb.self_iter() };
        }
    }

    template <typename Impl>
    typename BPTree<Impl>::IterPair
    BPTree<Impl>::data_block_erase(const Key &begin, const Key &end,
                                   Iter left, Iter right, State &state) {
        state = State::Success;
        if (left == right) {
            DataBlock data_block = left.data_block();
            Iter b = lower_bound(data_block.begin(), data_block.end(), begin),
                 e = lower_bound(data_block.begin(), data_block.end(), end);
            if (b == e) state = State::Fail;
            else data_block.erase(b, e);
            if (data_block.begin() == data_block.end()) {
                _impl.erase_block(left);
                return IterPair { Iter {}, Iter {} };
            }
            return data_block.begin();
        } else {
            DataBlock lb = left.data_block(), rb = right.data_block();
            lb.erase(lower_bound(lb.begin(), lb.end(), begin), lb.end());
            rb.erase(rb.begin(), upper_bound(right.begin(), right.end(), end));

            int choose = lb.merge_keep_average(rb);
            if (choose > 0) {
                if (lb.begin() != lb.end()) {
                    lb.merge(rb);
                    _impl.erase_block(right);
                    return IterPair { lb.self_iter(), Iter {} };
                }
                _impl.erase_block(left);
                if (rb.begin() == rb.end()) {
                    _impl.erase_block(right);
                    return IterPair { Iter {}, Iter {} };
                }
                return IterPair { Iter {}, rb.self_iter() };
            }
            if (choose < 0)
                lb.average(rb);
            return IterPair { lb.self_iter(), rb.self_iter() };
        }
    }

    template <typename Impl>
    void BPTree<Impl>::erase_all_block(Iter begin, Iter end) {
        while (begin != end) {
            if (!begin.is_data_block()) {
                IndexBlock index_block = begin.index_block();
                erase_all_block(index_block.begin(), index_block.end());
            }
            _impl.erase_block(begin);
            ++begin;
        }
    }

}

#endif

#endif //BASE_BPTREE_HPP
