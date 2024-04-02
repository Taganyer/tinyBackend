//
// Created by taganyer on 3/30/24.
//

#ifndef BASE_LIST_HPP
#define BASE_LIST_HPP

#include <utility>
#include "../Detail/config.hpp"

namespace Base {

    template<typename Data>
    class List {
    public:

        List() = default;

        ~List();

        class Iter;

        Iter begin() { return {_head}; };

        Iter tail() { return {_tail}; };

        Iter end() { return {nullptr}; };

        template<typename ...Args>
        Iter insert(Iter dest, Args &&...args);

        void move_to(Iter dest, Iter target);

        void erase(Iter target);

        void erase(Iter begin, Iter end);

        [[nodiscard]] uint64 size() const { return _size; };

    private:

        struct Node;

        Node *_head = nullptr;
        Node *_tail = nullptr;

        uint64 _size = 0;

    };


    template<typename Data>
    struct List<Data>::Node {

        template<typename ...Args>
        Node(Args... args) : _data(std::forward<Args>(args)...) {};

        Data _data;

        Node *_prev = nullptr;
        Node *_next = nullptr;

    };

    template<typename Data>
    class List<Data>::Iter {
    public:

        Iter() = default;

        Iter(Node *node) : _ptr(node) {};

        Data &operator*() { return _ptr->_data; };

        const Data &operator*() const { return _ptr->_data; };

        Data *operator->() { return &_ptr->_data; };

        const Data &operator->() const { return _ptr->_data; };

        Iter &operator++() {
            _ptr = _ptr->_next;
            return *this;
        };

        Iter operator++(int) {
            Iter temp(*this);
            _ptr = _ptr->_next;
            return temp;
        };

        Iter &operator--() {
            _ptr = _ptr->_prev;
            return *this;
        };

        Iter operator--(int) {
            Iter temp(*this);
            _ptr = _ptr->_next;
            return temp;
        };

        bool operator==(const Iter &other) const { return _ptr == other._ptr; };

        bool operator!=(const Iter &other) const { return _ptr != other._ptr; };

    private:

        List<Data>::Node *_ptr = nullptr;

        friend class List<Data>;

    };

    template<typename Data>
    inline List<Data>::~List() {
        while (_head) {
            Node *temp = _head;
            _head = _head->_next;
            delete temp;
        }
    }

    template<typename Data>
    template<typename... Args>
    inline typename List<Data>::Iter
    List<Data>::insert(List::Iter dest, Args &&... args) {
        auto ptr = new Node(std::forward<Args>(args)...);
        if (dest._ptr) {
            ptr->_prev = dest._ptr->_prev;
            ptr->_next = dest._ptr;
            dest._ptr->_prev = ptr;
            if (dest == _head) _head = ptr;
        } else {
            ptr->_prev = _tail;
            if (_tail) _tail->_next = ptr;
            else _head = ptr;
            _tail = ptr;
        }
        ++_size;
        return List::Iter(ptr);
    }

    template<typename Data>
    inline void List<Data>::move_to(List::Iter dest, List::Iter target) {
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
        } else {
            ptr->_prev = _tail;
            if (_tail) _tail->_next = ptr;
            else _head = ptr;
            _tail = ptr;
        }
    }

    template<typename Data>
    inline void List<Data>::erase(List::Iter target) {
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
        --_size;
        delete ptr;
    }

    template<typename Data>
    inline void List<Data>::erase(List::Iter begin, List::Iter end) {
        if (!begin._ptr) return;
        if (begin._ptr->_prev) begin._ptr->_prev->_next = end._ptr;
        else _head = end._ptr;
        if (end._ptr) end._ptr->_prev = begin._ptr->_prev;
        else _tail = begin._ptr->_prev;
        while (begin != end) {
            --_size;
            delete (begin++)._ptr;
        }
    }

}

#endif //BASE_LIST_HPP
