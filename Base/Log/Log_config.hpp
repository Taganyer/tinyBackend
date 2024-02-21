//
// Created by taganyer on 23-12-27.
//

#ifndef TEST_LOG_CONFIG_HPP
#define TEST_LOG_CONFIG_HPP


#include <fstream>
#include <condition_variable>
#include "../Time/TimeStamp.hpp"


namespace Base::Detail {

    constexpr size_t BUF_SIZE = 2 << 20;
    constexpr size_t FILE_RESTRICT = 2 << 30;

    using namespace std::chrono_literals;
    constexpr std::chrono::milliseconds FLUSH_TIME = 2s;

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
        bool append(int rank, const Time &time, const char *data, size_t size) {
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

        [[nodiscard]] size_t size() const { return index; }

        ~Log_buffer() { delete[] buffer; };

    private:
        char *buffer = new char[BUF_SIZE + 1];
        size_t index = 0;

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


#endif //TEST_LOG_CONFIG_HPP
