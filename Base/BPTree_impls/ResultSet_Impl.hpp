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

        explicit Result_Impl(const BlockIter<Key, Value>& iter) :
            key(iter.key()), value(iter.value()) {};

        Result_Impl(const Key& key, const Value& value) :
            key(key), value(value) {};

        Key key;

        Value value;

    };

    template <typename K, typename V>
    class ResultSet_Impl {
    public:
        using Key = K;
        using Value = V;

        using Array = std::vector<Result_Impl<Key, V>>;

        ResultSet_Impl() = default;

        ResultSet_Impl(const ResultSet_Impl&) = default;

        ResultSet_Impl(ResultSet_Impl&&) = default;

        void add(const BlockIter<Key, Value>& iter) {
            results.emplace_back(iter);
        };

        void add(const Key& key, const Value& value) {
            results.emplace(key, value);
        };

        Array results;

    };

}

#endif

#endif //BASE_RESULTSET_IMPL_HPP
