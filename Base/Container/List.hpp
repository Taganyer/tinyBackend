//
// Created by taganyer on 3/30/24.
//

#ifndef BASE_LIST_HPP
#define BASE_LIST_HPP

#include <utility>
#include "Base/Detail/config.hpp"

namespace Base {

    template <typename Data>
    class List {
    public:
        List() = default;

        List(const List &other);

        List(List &&other) noexcept;

        ~List();

        class Iter;

        class Val;

        Iter begin() { return { _head }; };

        Iter tail() { return { _tail }; };

        Iter end() { return { nullptr }; };

        template <typename...Args>
        Iter insert(Iter dest, Args &&...args);

        Iter insert(Iter dest, Val &&val);

        void move_to(Iter dest, Iter target);

        void erase(Iter target);

        void erase(Iter begin, Iter end);

        Val release(Iter target);

        [[nodiscard]] uint64 size() const { return _size; };

    private:
        struct Node;

        Node* _head = nullptr;
        Node* _tail = nullptr;

        uint64 _size = 0;

    };


    template <typename Data>
    struct List<Data>::Node {

        template <typename...Args>
        Node(Args...args) : _data(std::forward<Args>(args)...) {};

        Data _data;

        Node* _prev = nullptr;
        Node* _next = nullptr;

    };

    template <typename Data>
    class List<Data>::Iter {
    public:
        Iter() = default;

        Iter(Node* node) : _ptr(node) {};

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

        Iter& operator--() {
            _ptr = _ptr->_prev;
            return *this;
        };

        Iter operator--(int) {
            Iter temp(*this);
            _ptr = _ptr->_prev;
            return temp;
        };

        bool operator==(const Iter &other) const { return _ptr == other._ptr; };

        bool operator!=(const Iter &other) const { return _ptr != other._ptr; };

    private:
        Node* _ptr = nullptr;

        friend class List;

    };

    template <typename Data>
    class List<Data>::Val {
    public:
        Val(const Val &) = delete;

        Val(Val &&other) noexcept: _ptr(other._ptr) { other._ptr = nullptr; };

        ~Val() { delete _ptr; };

        explicit Val(Node* node) : _ptr(node) {};

        Data& operator*() { return _ptr->_data; };

        const Data& operator*() const { return _ptr->_data; };

        Data* operator->() { return &_ptr->_data; };

        const Data& operator->() const { return _ptr->_data; };

    private:
        Node* _ptr = nullptr;

        friend class List;

    };


    template <typename Data>
    List<Data>::List(const List &other) : _size(other.size()) {
        Iter iter = other.begin(), end = other.end();
        Node* temp,* last = nullptr;
        while (iter != end) {
            temp = new Node(*iter);
            if (!last) {
                _head = temp;
            } else {
                temp->_prev = last;
                last->_next = temp;
            }
            last = temp;
            ++iter;
        }
        _tail = last;
    }

    template <typename Data>
    List<Data>::List(List &&other) noexcept :
        _head(other._head), _tail(other._tail), _size(other._size) {
        other._head = other._tail = nullptr;
        other._size = 0;
    }

    template <typename Data>
    List<Data>::~List() {
        while (_head) {
            Node* temp = _head;
            _head = _head->_next;
            delete temp;
        }
    }

    template <typename Data>
    template <typename...Args>
    typename List<Data>::Iter
    List<Data>::insert(Iter dest, Args &&...args) {
        auto ptr = new Node(std::forward<Args>(args)...);
        if (dest._ptr) {
            ptr->_prev = dest._ptr->_prev;
            ptr->_next = dest._ptr;
            dest._ptr->_prev = ptr;
            if (dest == _head) _head = ptr;
            else ptr->_prev->_next = ptr;
        } else {
            ptr->_prev = _tail;
            if (_tail) _tail->_next = ptr;
            else _head = ptr;
            _tail = ptr;
        }
        ++_size;
        return List::Iter(ptr);
    }

    template <typename Data>
    typename List<Data>::Iter List<Data>::insert(Iter dest, Val&& val) {
        auto ptr = val._ptr;
        val._ptr = nullptr;
        ptr->_prev = ptr->_next = nullptr;
        if (dest._ptr) {
            ptr->_prev = dest._ptr->_prev;
            ptr->_next = dest._ptr;
            dest._ptr->_prev = ptr;
            if (dest == _head) _head = ptr;
            else ptr->_prev->_next = ptr;
        } else {
            ptr->_prev = _tail;
            if (_tail) _tail->_next = ptr;
            else _head = ptr;
            _tail = ptr;
        }
        ++_size;
        return List::Iter(ptr);
    }

    template <typename Data>
    void List<Data>::move_to(Iter dest, Iter target) {
        auto ptr = target._ptr;
        if (!ptr || ptr->_next == dest._ptr) return;
        if (ptr->_next) {
            ptr->_next->_prev = ptr->_prev;
            if (ptr->_prev) ptr->_prev->_next = ptr->_next;
            else _head = ptr->_next;
        } else {
            _tail = ptr->_prev;
            if (_tail) _tail->_next = nullptr;
            else _head = nullptr;
        }
        if (dest._ptr) {
            ptr->_prev = dest._ptr->_prev;
            ptr->_next = dest._ptr;
            dest._ptr->_prev = ptr;
            if (dest == _head) _head = ptr;
            else ptr->_prev->_next = ptr;
        } else {
            ptr->_prev = _tail;
            ptr->_next = nullptr;
            if (_tail) _tail->_next = ptr;
            else _head = ptr;
            _tail = ptr;
        }
    }

    template <typename Data>
    void List<Data>::erase(Iter target) {
        auto ptr = target._ptr;
        if (!ptr) return;
        if (ptr->_next) {
            ptr->_next->_prev = ptr->_prev;
            if (ptr->_prev) ptr->_prev->_next = ptr->_next;
            else _head = ptr->_next;
        } else {
            _tail = ptr->_prev;
            if (_tail) _tail->_next = nullptr;
            else _head = nullptr;
        }
        delete ptr;
        --_size;
    }

    template <typename Data>
    void List<Data>::erase(Iter begin, Iter end) {
        if (!begin._ptr) return;
        if (begin._ptr->_prev) begin._ptr->_prev->_next = end._ptr;
        else _head = end._ptr;
        if (end._ptr) end._ptr->_prev = begin._ptr->_prev;
        else _tail = begin._ptr->_prev;
        while (begin != end) {
            delete (begin++)._ptr;
            --_size;
        }
    }

    template <typename Data>
    typename List<Data>::Val List<Data>::release(Iter target) {
        auto ptr = target._ptr;
        if (!ptr) return List::Val(nullptr);
        if (ptr->_next) {
            ptr->_next->_prev = ptr->_prev;
            if (ptr->_prev) ptr->_prev->_next = ptr->_next;
            else _head = ptr->_next;
        } else {
            _tail = ptr->_prev;
            if (_tail) _tail->_next = nullptr;
            else _head = nullptr;
        }
        --_size;
        return List::Val(ptr);
    }

}

#endif //BASE_LIST_HPP
