//
// Created by taganyer on 23-12-27.
//

#ifndef BASE_LOG_CONFIG_HPP
#define BASE_LOG_CONFIG_HPP

#include <fstream>
#include "../Time/Timer.hpp"
#include "../Time/TimeStamp.hpp"


namespace Base::Detail {

    constexpr uint64 BUF_SIZE = 2 << 20;
    constexpr uint64 FILE_RESTRICT = 2 << 30;

    constexpr int64 FLUSH_TIME = 2 * SEC_;

    enum {
        INFO,
        DEBUG,
        WARN,
        ERROR,
        FATAL
    };

    static inline void write_info(char *ptr, int rank) {
        constexpr char store[5][7]{
                "INFO :", "DEBUG:", "WARN :", "ERROR:", "FATAL:"
        };
        std::memcpy(ptr, store[rank], 6);
    }

    class Log_buffer {
    public:
        bool append(int rank, const Time &time, const char *data, uint64 size) {
            if (BUF_SIZE - index <= size + TimeStamp::Time_us_format_len + 7) return false;
            format(buffer + index, time);
            buffer[index += TimeStamp::Time_us_format_len] = ' ';
            ++index;
            write_info(buffer + index, rank);
            index += 6;
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

    };

    class IO_error : public std::exception {
    public:
        IO_error() = default;

        IO_error(std::string message) : message(std::move(message)) {};

        [[nodiscard]] const char *what() const noexcept override {
            return message.c_str();
        }

    private:
        std::string message = "IO_error.\n";
    };

}


#endif //BASE_LOG_CONFIG_HPP
