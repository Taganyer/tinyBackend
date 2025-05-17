//
// Created by taganyer on 24-2-22.
//

#ifndef BASE_BLOCKQUEUE_HPP
#define BASE_BLOCKQUEUE_HPP

#include "Condition.hpp"

namespace Base {

    template <typename Type>
    class BlockQueue : NoCopy {
    public:
        explicit BlockQueue(uint32 max_size) : _limit(max_size) {};

        BlockQueue(BlockQueue<Type>&& other) = delete;

        ~BlockQueue();

        Type take();

        bool put(Type&& val);

        bool put(const Type& val);

        bool put_to_top(Type&& val);

        bool put_to_top(const Type& val);

        void clear();

        void start();

        void stop();

        void reset_limit(uint32 max_size);

        [[nodiscard]] unsigned limit_size() const { return _limit; };

        [[nodiscard]] unsigned size() const { return _size; };

        [[nodiscard]] bool stopping() const { return !_run; };

    private:
        struct Node {
            Node *next = nullptr;
            Type val;

            explicit Node(Type&& val) : val(std::move(val)) {};

            explicit Node(const Type& val) : val(val) {};
        };

        Node *head = nullptr, *tail = nullptr;

        uint32 _limit, _size = 0;

        volatile bool _run = true;

        Mutex _mutex;

        Condition _take, _put;

        auto takeable() const {
            return [this] { return _run && head; };
        };

        auto joinable() const {
            return [this] { return !_run || _size < _limit; };
        };

        Type* take_node() {
            Node *t = head;
            head = head->next;
            if (!head) tail = nullptr;
            Type val = std::move(t->val);
            delete t;
            --_size;
            return val;
        };

        void to_top(Node *target) {
            if (head) target->next = head;
            else tail = target;
            head = target;
            ++_size;
        };

        void to_tail(Node *target) {
            if (tail) tail->next = target;
            else head = target;
            tail = target;
            ++_size;
        };

    };

}

namespace Base {

    template <typename Type>
    BlockQueue<Type>::~BlockQueue() {
        stop();
        clear();
    }

    template <typename Type>
    Type BlockQueue<Type>::take() {
        Lock l(_mutex);
        _put.notify_one();
        _take.wait(l, takeable());
        return take_node();
    }

    template <typename Type>
    bool BlockQueue<Type>::put(Type&& val) {
        if (!_run) return false;
        Lock l(_mutex);
        _put.wait_for(l, joinable());
        if (!_run) return false;
        to_tail(new Node(std::move(val)));
        _take.notify_one();
        return true;
    }

    template <typename Type>
    bool BlockQueue<Type>::put(const Type& val) {
        if (!_run) return false;
        Lock l(_mutex);
        _put.wait_for(l, joinable());
        if (!_run) return false;
        to_tail(new Node(val));
        _take.notify_one();
        return true;
    }

    template <typename Type>
    bool BlockQueue<Type>::put_to_top(Type&& val) {
        if (!_run) return false;
        Lock l(_mutex);
        _put.wait_for(l, joinable());
        if (!_run) return false;
        to_top(new Node(std::move(val)));
        _take.notify_one();
        return true;
    }

    template <typename Type>
    bool BlockQueue<Type>::put_to_top(const Type& val) {
        if (!_run) return false;
        Lock l(_mutex);
        _put.wait_for(l, joinable());
        if (!_run) return false;
        to_top(new Node(val));
        _take.notify_one();
        return true;
    }

    template <typename Type>
    void BlockQueue<Type>::clear() {
        Lock l(_mutex);
        while (head) {
            Node *t = head;
            head = head->next;
            delete t;
        }
        _size = 0;
        tail = nullptr;
    }

    template <typename Type>
    void BlockQueue<Type>::start() {
        Lock l(_mutex);
        _run = true;
    }

    template <typename Type>
    void BlockQueue<Type>::stop() {
        Lock l(_mutex);
        _run = false;
    }

    template <typename Type>
    void BlockQueue<Type>::reset_limit(uint32 max_size) {
        Lock l(_mutex);
        _limit = max_size;
    }

}

#endif //BASE_BLOCKQUEUE_HPP
