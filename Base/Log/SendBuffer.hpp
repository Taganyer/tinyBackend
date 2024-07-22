//
// Created by taganyer on 3/28/24.
//

#ifndef BASE_SENDBUFFER_HPP
#define BASE_SENDBUFFER_HPP

#ifdef BASE_SENDBUFFER_HPP

#include "LogRank.hpp"
#include "Base/Detail/config.hpp"
#include "Base/Detail/NoCopy.hpp"

namespace Base {

    class Time;

    class SendBuffer : private NoCopy {
    public:
        static constexpr uint64 default_buffer_size = 1 << 20;

        explicit SendBuffer(uint64 size = default_buffer_size) :
            buffer(size > 0 ? new char[size] : nullptr), buffer_size(size) {};

        SendBuffer(SendBuffer &&other) noexcept;

        ~SendBuffer() { delete[] buffer; };

        uint64 append(const void* data, uint64 size);

        uint64 append(LogRank rank, const Time &time, const void* data, uint64 size);

        void flush();

        [[nodiscard]] uint64 size() const { return index; };

        [[nodiscard]] uint64 total_size() const { return buffer_size; };

        /// 不会在末尾添加'/0'
        [[nodiscard]] const char* data() const { return buffer; };

    private:
        char* buffer;

        uint64 index = 0, buffer_size;

    };
}

#endif

#endif //BASE_SENDBUFFER_HPP
