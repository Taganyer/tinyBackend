//
// Created by taganyer on 24-3-5.
//

#ifndef BASE_RINGBUFFER_HPP
#define BASE_RINGBUFFER_HPP

#include "RingBufferWrapper.hpp"

namespace Base {

    class RingBuffer : public RingBufferWrapper {
    public:
        explicit RingBuffer(uint32 size = 1024) : RingBufferWrapper(allocate(size), size) {};

        RingBuffer(RingBuffer&& other) noexcept : RingBufferWrapper(std::move(other)) {};

        ~RingBuffer() override {
            deallocate(begin());
        };

        void resize(uint32 size) {
            char *old = RingBufferWrapper::resize(allocate(size), size);
            deallocate(old);
        };

    private:
        static char* allocate(uint32 size) {
            char *ret = new char[size];
            return ret;
        };

        static void deallocate(const char *buf) {
            delete[] buf;
        };

    };

}


#endif //BASE_RINGBUFFER_HPP
