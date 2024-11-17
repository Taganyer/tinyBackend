//
// Created by taganyer on 24-10-10.
//

#ifndef LOGSYSTEM_IDENTIFICATION_HPP
#define LOGSYSTEM_IDENTIFICATION_HPP

#ifdef LOGSYSTEM_IDENTIFICATION_HPP

#include <cassert>
#include <cstring>
#include "Base/Detail/config.hpp"

namespace LogSystem {

    static constexpr uint32 SERVICE_ID_SIZE = 8;
    static constexpr uint32 NODE_ID_SIZE = 8;

    enum LinkNodeType : uint8 {
        Invalid,
        Head,
        BranchHead,
        Fork,
        Follow,
        Decision,
        RpcFork,
        RpcFollow,
        RpcDecision
    };

    inline const char* NodeTypeName(LinkNodeType type) {
        constexpr const char* name[] = {
            "[Invalid]",
            "[Head]",
            "[BranchHead]",
            "[Fork]",
            "[Follow]",
            "[Decision]"
        };
        return name[type];
    };

    template <uint32 _size>
    struct Mark_Helper {
        static bool equal(const void* left, const void* right) {
            return std::memcmp(left, right, _size) == 0;
        };

        static bool less(const void* left, const void* right) {
            return std::memcmp(left, right, _size) < 0;
        };

        static bool great(const void* left, const void* right) {
            return std::memcmp(left, right, _size) > 0;
        };
    };

    template <uint32 _size, typename Helper = Mark_Helper<_size>>
    class Mark {
    public:
        Mark() = default;

        explicit Mark(const void* data, uint32 size) {
            assert(size <= _size);
            if (data == nullptr) return;
            std::memcpy(_mark, data, size);
        };

        char* data() {
            return _mark;
        }

        [[nodiscard]] const char* data() const {
            return _mark;
        };

        [[nodiscard]] bool valid() const {
            uint32 i = 0;
            for (; i < _size; ++i)
                if (_mark[i] != 0) break;
            return i < _size;
        }

        friend bool operator==(const Mark &left, const Mark &right) {
            return Helper::equal(left._mark, right._mark);
        };

        friend bool operator!=(const Mark &left, const Mark &right) {
            return !Helper::equal(left._mark, right._mark);
        };

        friend bool operator<(const Mark &left, const Mark &right) {
            return Helper::less(left._mark, right._mark);
        };

        friend bool operator>(const Mark &left, const Mark &right) {
            return Helper::great(left._mark, right._mark);
        };

    private:
        char _mark[_size] {};

    };

    using LinkServiceID = Mark<SERVICE_ID_SIZE>;

    using LinkNodeID = Mark<NODE_ID_SIZE>;

    class Identification {
    public:
        Identification() = default;

        Identification(const void* data1, uint32 size1, const void* data2, uint32 size2) :
            _service(data1, size1), _node(data2, size2) {};

        Identification(const LinkServiceID &service, const LinkNodeID &node) :
            _service(service), _node(node) {};

        [[nodiscard]] const LinkServiceID& serviceID() const { return _service; };

        [[nodiscard]] const LinkNodeID& nodeID() const { return _node; };

        friend bool operator==(const Identification &left, const Identification &right) {
            return left._service == right._service && left._node == right._node;
        };

        friend bool operator!=(const Identification &left, const Identification &right) {
            return !(right == left);
        };

        friend bool operator<(const Identification &left, const Identification &right) {
            if (left._service != right._service)
                return left._service < right._service;
            return left._node < right._node;
        };

        friend bool operator>(const Identification &left, const Identification &right) {
            if (left._service != right._service)
                return left._service > right._service;
            return left._node > right._node;
        };

    private:
        LinkServiceID _service;

        LinkNodeID _node;

    };

    class LinkedMark {
    public:
        using ParentID = Identification;

        LinkedMark() = default;

        LinkedMark(const void* data1, uint32 size1, const void* data2, uint32 size2,
                   const void* data3, uint32 size3) :
            _parent(data1, size1, data2, size2), _child_node(data3, size3) {};

        LinkedMark(const LinkServiceID &service, const LinkNodeID &parent_node, const LinkNodeID &child_node) :
            _parent(service, parent_node), _child_node(child_node) {};

        LinkedMark(const ParentID &parent, const LinkNodeID &node) :
            _parent(parent), _child_node(node) {};

        [[nodiscard]] const ParentID& parentID() const { return _parent; };

        [[nodiscard]] const LinkNodeID& childNode() const { return _child_node; };

    private:
        ParentID _parent;

        LinkNodeID _child_node;

    };

}

namespace std {

    template <>
    struct hash<LogSystem::Identification> {
        std::size_t operator()(const LogSystem::Identification &id) const noexcept {
            return _Hash_impl::hash(&id, sizeof(LogSystem::Identification));
        }
    };

}

#endif

#endif //LOGSYSTEM_IDENTIFICATION_HPP
