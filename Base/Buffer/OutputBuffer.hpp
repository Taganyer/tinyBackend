//
// Created by taganyer on 25-3-3.
//

#ifndef BASE_OUTPUTBUFFER_HPP
#define BASE_OUTPUTBUFFER_HPP

#ifdef BASE_OUTPUTBUFFER_HPP

#include "Base/Detail/config.hpp"
#include "Base/Detail/NoCopy.hpp"
#include "Base/Detail/BufferArray.hpp"

namespace Base {

    class InputBuffer;

    class OutputBuffer : NoCopy {
    public:
        explicit OutputBuffer(char* buf) : _write(buf) {};

        OutputBuffer(OutputBuffer &&other) noexcept : _write(other._write) {
            other._write = nullptr;
        };

        virtual ~OutputBuffer() = default;

        virtual uint32 write(const void* target, uint32 size) const = 0;

        virtual uint32 write(uint32 N, const iovec* array) const = 0;

        virtual uint32 write(const InputBuffer &buffer, uint32 size) const = 0;

        virtual uint32 fix_write(const void* target, uint32 size) const = 0;

        virtual uint32 fix_write(uint32 N, const iovec* array) const = 0;

        virtual uint32 fix_write(const InputBuffer &buffer, uint32 size) const = 0;

        template <std::size_t N>
        uint32 write(const BufferArray<N> &array) const {
            return write(N, array.data());
        };

        template <std::size_t N>
        uint32 fix_write(const BufferArray<N> &array) const {
            return fix_write(N, array.data());
        };

        virtual uint32 try_write(const void* target, uint32 size, uint32 offset) const = 0;

        virtual uint32 try_write(uint32 N, const iovec* array, uint32 offset) const = 0;

        template <std::size_t N>
        uint32 try_write(const BufferArray<N> &array, uint32 offset = 0) const {
            return try_write(N, array.data(), offset);
        };

        virtual void clear_output() const = 0;

        [[nodiscard]] virtual uint32 writable_len() const = 0;

        [[nodiscard]] const char* write_data() const { return _write; };

    protected:
        mutable char* _write = nullptr;

    };

}

#endif

#endif //BASE_OUTPUTBUFFER_HPP
