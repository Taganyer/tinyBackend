//
// Created by taganyer on 24-9-19.
//

#ifndef BASE_LRUCACHE_HPP
#define BASE_LRUCACHE_HPP

#ifdef BASE_LRUCACHE_HPP

#include <vector>
#include <cassert>
#include <algorithm>
#include <unordered_map>
#include "Base/Exception.hpp"
#include "Base/Container/List.hpp"
#include "Base/Time/Time_difference.hpp"

namespace Base {

    /*
        class LRUCacheHelper {
            public:
                using Key = Key;

                using Value = Value;

                bool can_create(const Key &key);

                Value create(const Key &key);

                void update(const Key &key, Value &value);

                /// optional
                void release(const Key &key, Value &value);
        };
    */

    template <typename Helper>
    class LRUCache {
    public:
        using Key = typename Helper::Key;

        using Value = typename Helper::Value;

        static constexpr Time_difference threshold_time { 1 * SEC_ };

        Helper _helper;

        template <typename...Args>
        explicit LRUCache(Args &&...args) : _helper(std::forward<Args>(args)...) {};

        ~LRUCache();

        Value* get(const Key &key);

        void put(const Key &key, bool need_update);

        void update_one();

        void update_all();

        template <typename Cmp>
        void update_all_with_order(Cmp cmp);

        void remove(const Key &key);

        [[nodiscard]] uint32 need_update_size() const { return _obsolete.size(); };

    private:
        struct Node;

        using Map = std::unordered_map<Key, Node>;

        using MapIter = typename Map::iterator;

        using Queue = List<MapIter>;

        using QIter = typename Queue::Iter;

        struct Node {
            Value value;

            bool need_update = false;

            bool is_persistent = false;

            uint32 instantiated_size = 0;

            Time_difference instantiated_time;

            QIter iter;

            QIter obsolete_iter;

            template <typename...Args>
            explicit Node(Args &&...args) : value(std::forward<Args>(args)...) {};

            Node(const Node &) = default;

            Node(Node &&other) = default;

        };

        Map _map;

        Queue _persistent;

        Queue _temporary;

        Queue _obsolete;

        void get_change(MapIter iter);

        void put_change(MapIter iter);

        void reserve_space(const Key &key);


        struct FunChecker {
        private:
            template <typename U>
            static auto release_test(int) ->
                decltype(std::declval<U>().release(std::declval<Key>(), std::declval<Value&>()),
                    std::true_type());

            template <typename U>
            static std::false_type release_test(...);

        public:
            static constexpr bool has_release_callable =
                decltype(release_test<Helper>(0))::value;

        };

    };

    template <typename Helper>
    LRUCache<Helper>::~LRUCache() {
        for (auto &[key, node] : _map) {
            if (node.instantiated_size != 0) {
                std::fprintf(stderr, "LRUCache might be cause invalid reference.");
                std::terminate();
            }
            if (node.need_update)
                _helper.update(key, node.value);
            if constexpr (FunChecker::has_release_callable) {
                _helper.release(key, node.value);
            }
        }
    }

    template <typename Helper>
    typename LRUCache<Helper>::Value*
    LRUCache<Helper>::get(const Key &key) {
        if (auto iter = _map.find(key); iter != _map.end()) {
            get_change(iter);
            return &iter->second.value;
        }
        reserve_space(key);
        auto [iter, success] = _map.emplace(key, _helper.create(key));
        assert(success);
        auto &[_, node] = *iter;
        ++node.instantiated_size;
        node.instantiated_time = Unix_to_now();
        return &node.value;
    }

    template <typename Helper>
    void LRUCache<Helper>::put(const Key &key, bool need_update) {
        auto iter = _map.find(key);
        assert(iter != _map.end());
        if (need_update)
            iter->second.need_update = true;
        put_change(iter);
    }

    template <typename Helper>
    void LRUCache<Helper>::update_one() {
        if (_obsolete.size() > 0) {
            QIter iter = _obsolete.begin();
            MapIter map_iter = *iter;
            auto &[key, node] = *map_iter;
            assert(node.need_update);
            assert(node.instantiated_size == 0);
            assert(node.obsolete_iter == iter);
            _helper.update(key, node.value);
            node.need_update = false;
            _obsolete.erase(iter);
            node.obsolete_iter = _obsolete.end();
        }
    }

    template <typename Helper>
    void LRUCache<Helper>::update_all() {
        for (QIter iter = _obsolete.begin(); iter != _obsolete.end(); ++iter) {
            MapIter map_iter = *iter;
            auto &[key, node] = *map_iter;
            assert(node.need_update);
            assert(node.instantiated_size == 0);
            _helper.update(key, node.value);
            node.need_update = false;
            node.obsolete_iter = _obsolete.end();
        }
        _obsolete.erase(_obsolete.begin(), _obsolete.end());
    }

    template <typename Helper> template <typename Cmp>
    void LRUCache<Helper>::update_all_with_order(Cmp cmp) {
        std::vector<MapIter> arr(_obsolete.size());
        QIter iter = _obsolete.begin();
        for (uint32 i = 0; i < _obsolete.size(); ++i, ++iter)
            arr[i] = *iter;
        std::sort(arr.begin(), arr.end(),
                  [&cmp] (MapIter a, MapIter b) {
                      return cmp(a->first, b->first);
                  });
        for (auto map_iter : arr) {
            auto &[key, node] = *map_iter;
            assert(node.need_update);
            assert(node.instantiated_size == 0);
            _helper.update(key, node.value);
            node.need_update = false;
            node.obsolete_iter = _obsolete.end();
        }
        _obsolete.erase(_obsolete.begin(), _obsolete.end());
    }

    template <typename Helper>
    void LRUCache<Helper>::remove(const Key &key) {
        auto iter = _map.find(key);
        if (iter == _map.end())
            throw Exception("LRUCache: key not found.");

        auto &[_, node] = *iter;
        assert(node.instantiated_size == 0);
        if (node.need_update) {
            _helper.update(key, node.value);
            assert(node.obsolete_iter != _obsolete.end());
            _obsolete.erase(node.obsolete_iter);
        }
        if constexpr (FunChecker::has_release_callable) {
            _helper.release(key, node.value);
        }

        if (node.is_persistent) {
            assert(node.iter != _persistent.end());
            _persistent.erase(node.iter);
        } else {
            assert(node.iter != _temporary.end());
            _temporary.erase(node.iter);
        }
        _map.erase(iter);
    }

    template <typename Helper>
    void LRUCache<Helper>::get_change(MapIter iter) {
        auto &[key, node] = *iter;
        if (++node.instantiated_size > 1) return;
        if (node.is_persistent) {
            assert(node.iter != _persistent.end());
            _persistent.erase(node.iter);
            node.iter = _persistent.end();
        } else {
            assert(node.iter != _temporary.end());
            _temporary.erase(node.iter);
            node.iter = _temporary.end();
        }

        if (node.obsolete_iter != _obsolete.end()) {
            assert(node.need_update);
            _obsolete.erase(node.obsolete_iter);
            node.obsolete_iter = _obsolete.end();
        }
    }

    template <typename Helper>
    void LRUCache<Helper>::put_change(MapIter iter) {
        auto &[key, node] = *iter;
        if (--node.instantiated_size > 0) return;
        if (node.is_persistent
            || Unix_to_now() - node.instantiated_time >= threshold_time) {
            node.is_persistent = true;
            node.iter = _persistent.insert(_persistent.end(), iter);
        } else {
            node.iter = _temporary.insert(_temporary.end(), iter);
        }

        if (node.need_update) {
            assert(node.obsolete_iter == _obsolete.end());
            node.obsolete_iter = _obsolete.insert(_obsolete.end(), iter);
        }
    }

    template <typename Helper>
    void LRUCache<Helper>::reserve_space(const Key &key) {
        if (_helper.can_create(key)) return;
        QIter iter = _temporary.begin();
        while (iter != _temporary.end() && !_helper.can_create(key)) {
            MapIter map_iter = *iter;
            auto &[key, node] = *map_iter;
            if (!node.need_update) {
                _temporary.erase(iter++);
                if constexpr (FunChecker::has_release_callable) {
                    _helper.release(key, node.value);
                }
                _map.erase(map_iter);
            } else {
                ++iter;
            }
        }

        if (_helper.can_create(key)) return;
        iter = _temporary.begin();
        while (iter != _temporary.end() && !_helper.can_create(key)) {
            MapIter map_iter = *iter;
            auto &[key, node] = *map_iter;
            assert(node.need_update);
            _helper.update(key, node.value);
            if constexpr (FunChecker::has_release_callable) {
                _helper.release(key, node.value);
            }
            _obsolete.erase(node.obsolete_iter);
            _temporary.erase(iter++);
            _map.erase(map_iter);
        }

        if (_helper.can_create(key)) return;
        iter = _persistent.begin();
        while (iter != _persistent.end() && !_helper.can_create(key)) {
            MapIter map_iter = *iter;
            auto &[key, node] = *map_iter;
            if (node.need_update) {
                _helper.update(key, node.value);
                _obsolete.erase(node.obsolete_iter);
            }
            if constexpr (FunChecker::has_release_callable) {
                _helper.release(key, node.value);
            }
            _persistent.erase(iter++);
            _map.erase(map_iter);
        }

        if (!_helper.can_create(key))
            throw Exception("LRUCache: can't create key because space is not enough.");
    }

}

#endif

#endif //BASE_LRUCACHE_HPP
