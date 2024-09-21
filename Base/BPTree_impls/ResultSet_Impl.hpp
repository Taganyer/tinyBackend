//
// Created by taganyer on 24-9-13.
//

#ifndef BASE_RESULTSET_IMPL_HPP
#define BASE_RESULTSET_IMPL_HPP

#ifdef BASE_RESULTSET_IMPL_HPP

#include "BlockIter.hpp"

namespace Base {

    template <typename K, typename V>
    class Result_Impl {
    public:
        using Key = K;
        using Value = V;

        Result_Impl() = default;

        explicit Result_Impl(const BlockIter<Key, Value> &iter) :
            key(iter.key()), value(iter.value()) {};

        Key key;

        Value value;

    };

    template <typename K, typename V>
    class ResultSet_Impl {
    public:
        using Key = K;
        using Value = V;

        ResultSet_Impl() = default;

        ResultSet_Impl(const ResultSet_Impl &) = default;

        ResultSet_Impl(ResultSet_Impl &&) = default;

        void add(const BlockIter<Key, Value> &iter) {
            results.emplace_back(iter);
        }

        std::vector<Result_Impl<Key, V>> results;

    };

}

#endif

#endif //BASE_RESULTSET_IMPL_HPP
