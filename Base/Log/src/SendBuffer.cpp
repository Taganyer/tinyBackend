//
// Created by taganyer on 3/28/24.
//

#include "../SendBuffer.hpp"

using namespace Base;

const uint64 SendBuffer::BUF_SIZE = 2 << 20;

SendBuffer::SendBuffer(SendBuffer &&other) noexcept:
        buffer(other.buffer), index(other.index) {
    other.buffer = nullptr;
    other.index = 0;
}

uint64 SendBuffer::append(const void *data, uint64 size) {
    if (BUF_SIZE - index < size)
        size = BUF_SIZE - index;
    std::memcpy(buffer + index, data, size);
    index += size;
    return size;
}

uint64 SendBuffer::append(LogRank rank, const Time &time, const void *data, uint64 size) {
    if (BUF_SIZE - index < size + TimeStamp::Time_us_format_len + 8)
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
