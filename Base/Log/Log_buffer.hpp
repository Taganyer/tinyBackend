//
// Created by taganyer on 23-12-27.
//

#ifndef BASE_LOG_BUFFER_HPP
#define BASE_LOG_BUFFER_HPP


#include "../Time/Timer.hpp"
#include "../Time/TimeStamp.hpp"


namespace Base::Detail {

    constexpr uint64 BUF_SIZE = 2 << 20;

    static inline void write_rank(char *ptr, int rank) {
        constexpr char store[6][7]{
                "TRACE:", "DEBUG:", "INFO :", "WARN :", "ERROR:", "FATAL:"
        };
        std::memcpy(ptr, store[rank], 6);
    }

    static constexpr uint64 basic_info_len() {
        return TimeStamp::Time_us_format_len + 7;
    }


    class Log_buffer {
    public:
        bool append(int rank, const Time &time, const char *data, uint64 size) {
            if (BUF_SIZE - index <= size + basic_info_len()) return false;
            write_info(rank, time);
            std::memcpy(buffer + index, data, size);
            buffer[index += size] = '\n';
            ++index;
            return true;
        };

        void flush() { buffer[index = 0] = '\0'; }

        [[nodiscard]] const char *get() const {
            buffer[index] = '\0';
            return buffer;
        };

        [[nodiscard]] uint64 size() const { return index; }

        ~Log_buffer() { delete[] buffer; };

    private:

        char *buffer = new char[BUF_SIZE + 1];

        uint64 index = 0;

        inline void write_info(int rank, const Time &time) {
            format(buffer + index, time);
            buffer[index += TimeStamp::Time_us_format_len] = ' ';
            ++index;
            write_rank(buffer + index, rank);
            index += 6;
        }

    };

}


#endif //BASE_LOG_BUFFER_HPP
