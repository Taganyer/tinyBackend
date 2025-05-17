//
// Created by taganyer on 4/3/24.
//

#ifndef BASE_SINGLELIST_HPP
#define BASE_SINGLELIST_HPP

#include <utility>
#include "tinyBackend/Base/Detail/config.hpp"

namespace Base {

    template <typename Data>
    class SingleList {
    public:
        SingleList() = default;

        SingleList(const SingleList& other);

        SingleList(SingleList&& other) noexcept;

        ~SingleList();

        class Iter;

        class Val;

        Iter begin() { return { _head }; };

        Iter tail() { return { _tail }; };

        Iter end() { return { nullptr }; };

        Iter before_begin() { return { nullptr }; };

        template <typename... Args>
        Iter insert_after(Iter before_dest, Args&&... args);

        Iter insert_after(Iter before_dest, Val&& val);

        void next_to_after(Iter before_dest, Iter before_target);

        void erase_after(Iter before_target);

        void erase_after(Iter before_begin, Iter end);

        Val release_after(Iter before_target);

        [[nodiscard]] uint64 size() const { return _size; };

    private:
        struct Node;

        Node *_head = nullptr;
        Node *_tail = nullptr;

        uint64 _size = 0;

    };


    template <typename Data>
    struct SingleList<Data>::Node {

        template <typename... Args>
        Node(Args... args) : _data(std::forward<Args>(args)...) {};

        Data _data;

        Node *_next = nullptr;

    };

    template <typename Data>
    class SingleList<Data>::Iter {
    public:
        Iter() = default;

        Iter(Node *node) : _ptr(node) {};

        Data& operator*() { return _ptr->_data; };

        const Data& operator*() const { return _ptr->_data; };

        Data* operator->() { return &_ptr->_data; };

        const Data& operator->() const { return _ptr->_data; };

        Iter& operator++() {
            _ptr = _ptr->_next;
            return *this;
        };

        Iter operator++(int) {
            Iter temp(*this);
            _ptr = _ptr->_next;
            return temp;
        };

        bool operator==(const Iter& other) const { return _ptr == other._ptr; };

        bool operator!=(const Iter& other) const { return _ptr != other._ptr; };

    private:
        SingleList<Data>::Node *_ptr = nullptr;

        friend class SingleList<Data>;

    };

    template <typename Data>
    class SingleList<Data>::Val {
    public:
        Val(const Val&) = delete;

        Val(Val&& other) noexcept: _ptr(other._ptr) { other._ptr = nullptr; };

        ~Val() { delete _ptr; };

        explicit Val(Node *node) : _ptr(node) {};

        Data& operator*() { return _ptr->_data; };

        const Data& operator*() const { return _ptr->_data; };

        Data* operator->() { return &_ptr->_data; };

        const Data& operator->() const { return _ptr->_data; };

    private:
        Node *_ptr = nullptr;

        friend class SingleList;

    };


    template <typename Data>
    SingleList<Data>::SingleList(const SingleList& other) : _size(other._size) {
        Iter iter = other.begin(), end = other.end();
        Node *temp, *last = nullptr;
        while (iter != end) {
            temp = new Node(*iter);
            (!last ? _head : last->_next) = temp;
            last = temp;
            ++iter;
        }
        _tail = last;
    }

    template <typename Data>
    SingleList<Data>::SingleList(SingleList&& other) noexcept :
        _head(other._head), _tail(other._tail), _size(other._size) {
        other._head = other._tail = nullptr;
        other._size = 0;
    }

    template <typename Data>
    SingleList<Data>::~SingleList() {
        while (_head) {
            Node *temp = _head;
            _head = _head->_next;
            delete temp;
        }
    }

    template <typename Data>
    template <typename... Args>
    typename SingleList<Data>::Iter
    SingleList<Data>::insert_after(Iter before_dest, Args&&... args) {
        auto ptr = new Node(std::forward<Args>(args)...);
        if (before_dest._ptr) {
            if (before_dest._ptr == _tail) _tail = ptr;
            ptr->_next = before_dest._ptr->_next;
            before_dest._ptr->_next = ptr;
        } else {
            if (_head) ptr->_next = _head;
            else _tail = ptr;
            _head = ptr;
        }
        ++_size;
        return SingleList::Iter();
    }

    template <typename Data>
    typename SingleList<Data>::Iter
    SingleList<Data>::insert_after(Iter before_dest, Val&& val) {
        auto ptr = val._ptr;
        val._ptr = nullptr;
        ptr->_next = nullptr;
        if (before_dest._ptr) {
            if (before_dest._ptr == _tail) _tail = ptr;
            ptr->_next = before_dest._ptr->_next;
            before_dest._ptr->_next = ptr;
        } else {
            if (_head) ptr->_next = _head;
            else _tail = ptr;
            _head = ptr;
        }
        ++_size;
        return SingleList::Iter();
    }

    template <typename Data>
    void SingleList<Data>::next_to_after(Iter before_dest, Iter before_target) {
        if (before_dest == before_target) return;
        if (before_dest._ptr) {
            if (before_target._ptr) {
                auto ptr = before_target._ptr->_next;
                if (!ptr || ptr == before_dest._ptr) return;
                if (before_dest._ptr == _tail) _tail = ptr;
                before_target._ptr->_next = ptr->_next;
                ptr->_next = before_dest._ptr->_next;
                before_dest._ptr->_next = ptr;
            } else {
                auto ptr = _head;
                if (ptr == before_dest._ptr) return;
                if (before_dest._ptr == _tail) _tail = _head;
                _head = _head->_next;
                ptr->_next = before_dest._ptr->_next;
                before_dest._ptr->_next = ptr;
            }
        } else {
            auto ptr = before_target._ptr->_next;
            if (!ptr) return;
            if (ptr == _tail) _tail = before_target._ptr;
            before_target._ptr->_next = ptr->_next;
            ptr->_next = _head;
            _head = ptr;
        }
    }

    template <typename Data>
    void SingleList<Data>::erase_after(Iter before_target) {
        if (before_target._ptr == _tail) return;
        Node *ptr;
        if (before_target._ptr) {
            ptr = before_target._ptr->_next;
            if (ptr == _tail) _tail = before_target._ptr;
            before_target._ptr->_next = ptr->_next;
        } else {
            ptr = _head;
            if (_head == _tail) _tail = nullptr;
            _head = _head->_next;
        }
        delete ptr;
        --_size;
    }

    template <typename Data>
    void SingleList<Data>::erase_after(Iter before_begin, Iter end) {
        if (before_begin._ptr == _tail) return;
        if (!end._ptr) _tail = before_begin._ptr;
        Node *ptr;
        if (before_begin._ptr) {
            ptr = before_begin._ptr->_next;
            before_begin._ptr->_next = end._ptr;
        } else {
            ptr = _head;
            _head = end._ptr;
        }
        while (ptr != end._ptr) {
            auto temp = ptr;
            ptr = ptr->_next;
            delete temp;
            --_size;
        }
    }

    template <typename Data>
    typename SingleList<Data>::Val
    SingleList<Data>::release_after(Iter before_target) {
        if (before_target._ptr == _tail) return SingleList::Val(nullptr);
        Node *ptr;
        if (before_target._ptr) {
            ptr = before_target._ptr->_next;
            if (ptr == _tail) _tail = before_target._ptr;
            before_target._ptr->_next = ptr->_next;
        } else {
            ptr = _head;
            if (_head == _tail) _tail = nullptr;
            _head = _head->_next;
        }
        --_size;
        return SingleList::Val(ptr);
    }

}

#endif //BASE_SINGLELIST_HPP
