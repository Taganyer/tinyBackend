//
// Created by taganyer on 3/28/24.
//

#ifndef BASE_SENDBUFFER_HPP
#define BASE_SENDBUFFER_HPP

#ifdef BASE_SENDBUFFER_HPP

#include "LogRank.hpp"
#include "Base/Detail/NoCopy.hpp"
#include "Base/Time/TimeStamp.hpp"

namespace Base {

    class Time;

    class SendBuffer : NoCopy {
    public:

        static const uint64 BUF_SIZE;

        SendBuffer() = default;

        SendBuffer(SendBuffer &&other) noexcept;

        ~SendBuffer() { delete[] buffer; };

        uint64 append(const void *data, uint64 size);

        uint64 append(LogRank rank, const Time &time, const void *data, uint64 size);

        void flush();

        [[nodiscard]] uint64 size() const { return index; };

        [[nodiscard]] const char *data() const { return buffer; };

    private:

        char *buffer = new char[BUF_SIZE + 1];

        uint64 index = 0;

    };
}

#endif

#endif //BASE_SENDBUFFER_HPP
