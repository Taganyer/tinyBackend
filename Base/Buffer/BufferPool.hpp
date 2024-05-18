//
// Created by taganyer on 4/6/24.
//

#ifndef BASE_BUFFERPOOL_HPP
#define BASE_BUFFERPOOL_HPP

#ifdef BASE_BUFFERPOOL_HPP

#include <vector>
#include <functional>
#include "Base/Condition.hpp"
#include "Base/Container/List.hpp"
#include "Base/Detail/NoCopy.hpp"

namespace Base {

    class BufferPool : Base::NoCopy {
    public:

        BufferPool(uint64 block_size, uint64 block_number);

        virtual ~BufferPool();

        [[nodiscard]] uint64 block_size() const { return _block_size; };

    protected:

        class BufferChannel;

        using BufferQueue = std::vector<void *>;

        using RunningQueue = List<BufferChannel>;

        char *const _buf;

        BufferQueue _buffers;

        RunningQueue _running;

        uint64 _block_size;

        Mutex _mutex;

        Condition _condition;

    public:

        using Buffer = RunningQueue::Iter;

        virtual Buffer get();

        virtual void free(Buffer buffer);

    };


    class BufferPool::BufferChannel {
    public:

        using FreeCallback = std::function<void(void *)>;

        BufferChannel(void *buffer, uint64 size) : buffer(buffer), size(size) {};

        ~BufferChannel() {
            if (_callback)
                _callback(buffer);
        };

        void set_callback(FreeCallback callback) {
            _callback = std::move(callback);
        };

        void *const buffer;

        const uint64 size;

    private:

        FreeCallback _callback;

    };

}

#endif

#endif //BASE_BUFFERPOOL_HPP
