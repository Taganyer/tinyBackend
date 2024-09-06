//
// Created by taganyer on 24-7-24.
//

#ifndef BASE_BUFFERPOOL_HPP
#define BASE_BUFFERPOOL_HPP

#ifdef BASE_BUFFERPOOL_HPP

#include <vector>
#include "Base/Mutex.hpp"

namespace Base {

    class BufferPool : NoCopy {
    public:
        constexpr static uint64 block_size = 1 << 12;

        /// 返回与 block_size 向上对齐的大小。
        static uint64 round_size(uint64 target);

        class Buffer;

        explicit BufferPool(uint64 total_size);

        ~BufferPool();

        Buffer get(uint64 size);

        [[nodiscard]] uint64 total_size() const { return _size; };

        [[nodiscard]] uint64 max_block() const { return _rest[0]; };

    private:
        uint64 _size = 0;

        char* _buffer;

        Mutex _mutex;

        std::vector<uint64> _rest;

        void put(Buffer &buffer);

        [[nodiscard]] std::pair<uint64, char *> positioning(uint64 size) const;

        [[nodiscard]] uint64 location(const char* target, uint64 size) const;

        friend class Buffer;

    public:
        class Buffer : NoCopy {
        public:
            Buffer(Buffer &&other) noexcept : _buf(other._buf), _size(other._size), _pool(other._pool) {
                other._buf = nullptr;
                other._size = 0;
            };

            ~Buffer() { put_back(); };

            char* data() { return _buf; };

            void put_back() { if (_buf) _pool.put(*this); };

            [[nodiscard]] const char* data() const { return _buf; };

            [[nodiscard]] uint64 size() const { return _size; };

            [[nodiscard]] operator bool() const { return _buf; };

        private:
            char* _buf;
            uint64 _size;
            BufferPool &_pool;

            Buffer(char* b, uint64 s, BufferPool &pool) : _buf(b), _size(s), _pool(pool) {};

            friend class BufferPool;

        };

    };

}

#endif

#endif //BASE_BUFFERPOOL_HPP
