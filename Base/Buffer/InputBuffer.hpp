//
// Created by taganyer on 25-3-3.
//

#ifndef BASE_INPUTBUFFER_HPP
#define BASE_INPUTBUFFER_HPP

#ifdef BASE_INPUTBUFFER_HPP

#include "Base/Detail/NoCopy.hpp"
#include "Base/Detail/BufferArray.hpp"

namespace Base {

    class OutputBuffer;

    class InputBuffer : NoCopy {
    public:
        explicit InputBuffer(char* buf) : _read(buf) {};

        InputBuffer(InputBuffer &&other) noexcept : _read(other._read) {
            other._read = nullptr;
        };

        virtual ~InputBuffer() = default;

        virtual uint32 read(void* dest, uint32 size) const = 0;

        virtual uint32 read(uint32 N, const iovec* array) const = 0;

        virtual uint32 read(const OutputBuffer &buffer, uint32 size) const = 0;

        virtual uint32 fix_read(void* dest, uint32 size) const = 0;

        virtual uint32 fix_read(uint32 N, const iovec* array) const = 0;

        virtual uint32 fix_read(const OutputBuffer &buffer, uint32 size) const = 0;

        template <std::size_t N>
        uint32 read(const BufferArray<N> &array) const {
            return read(N, array.data());
        };

        template <std::size_t N>
        uint32 fix_read(const BufferArray<N> &array) const {
            return fix_read(N, array.data());
        };

        virtual uint32 try_read(void* dest, uint32 size, uint32 offset) const = 0;

        virtual uint32 try_read(uint32 N, const iovec* array, uint32 offset) const = 0;

        template <std::size_t N>
        uint32 try_read(const BufferArray<N> &array, uint32 offset = 0) const {
            return try_read(N, array.data(), offset);
        };

        virtual void clear_input() const = 0;

        [[nodiscard]] virtual uint32 readable_len() const = 0;

        [[nodiscard]] const char* read_data() const { return _read; };

    protected:
        mutable char* _read = nullptr;

    };
}

#endif

#endif //BASE_INPUTBUFFER_HPP
