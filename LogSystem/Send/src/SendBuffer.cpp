//
// Created by taganyer on 3/28/24.
//

#include "../SendBuffer.hpp"
#include "Base/Time/TimeStamp.hpp"

using namespace Base;

using namespace LogSystem;

SendBuffer::SendBuffer(SendBuffer &&other) noexcept:
        buffer(other.buffer), index(other.index), buffer_size(other.buffer_size) {
    other.buffer = nullptr;
    other.index = other.buffer_size = 0;
}

uint64 SendBuffer::append(const void *data, uint64 size) {
    if (buffer_size - index < size)
        size = buffer_size - index;
    std::memcpy(buffer + index, data, size);
    index += size;
    return size;
}

uint64 SendBuffer::append(LogRank rank, const Time &time, const void *data, uint64 size) {
    if (buffer_size - index < size + TimeStamp::Time_us_format_len + 8)
        return 0;
    format(buffer + index, time);
    index += TimeStamp::Time_us_format_len;
    buffer[index++] = ' ';
    index += rank_toString(buffer + index, rank);
    std::memcpy(buffer + index, data, size);
    buffer[index += size] = '\n';
    ++index;
    return size;
}

void SendBuffer::flush() {
    index = 0;
}
