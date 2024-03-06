//
// Created by taganyer on 23-12-27.
//

#ifndef BASE_LOG_HPP
#define BASE_LOG_HPP

#include "Log_buffer.hpp"
#include "../Thread.hpp"
#include "../Condition.hpp"
#include "../Detail/oFile.hpp"

namespace Base {

    constexpr uint64 FILE_RESTRICT = 2 << 30;

    constexpr int64 FLUSH_TIME = 2 * SEC_;

    enum LogRank {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };


    class LogStream;

    class Log : NoCopy {
    public:

        Log(std::string dictionary_path, LogRank rank);

        void push(int rank, const char *data, uint64 size);

        ~Log();

        LogStream stream(int rank);

        void set_rank(LogRank rank) { outputRank = rank; };

        [[nodiscard]] LogRank get_rank() const { return outputRank; };

    private:

        struct Queue {
            Detail::Log_buffer buffer;
            Queue *next = nullptr;

            Queue() = default;
        };

        Queue *empty_queue = nullptr;
        Queue *full_queue = nullptr;
        Queue *current_queue = nullptr;

        Mutex IO_lock;
        Condition condition;
        bool stop = false;

        std::string path;

        oFile out;

        size_t file_current_size = 0;

        LogRank outputRank;

        inline void get_new_buffer();

        inline void put_full_buffer();

        inline void put_to_empty_buffer(Queue *target);

        inline void clear_empty_buffer();

        inline void open_new_file();

        void invoke(Queue *target);

    };


    class LogStream : NoCopy {
    public:

        LogStream(Log &log, int rank) : _log(&log), _rank(rank) {};

        ~LogStream() {
            _log->push(_rank, _message, _index);
        };

        LogStream &operator<<(const std::string &val) {
            if (_log->get_rank() > _rank) return *this;
            int temp = val.size() > 256 - _index ? 256 - _index : val.size();
            memcpy(_message + _index, val.data(), temp);
            _index += temp;
            return *this;
        };

        LogStream &operator<<(const std::string_view &val) {
            if (_log->get_rank() > _rank) return *this;
            auto temp = val.size() > 256 - _index ? 256 - _index : val.size();
            memcpy(_message + _index, val.data(), temp);
            _index += temp;
            return *this;
        };

#define StreamOperator(type, f) LogStream &operator<<(type val) { \
                if (_log->get_rank() > _rank) return *this;       \
                _index += snprintf(_message + _index, 256 - _index, f, val); \
                return *this;                                     \
        };

        StreamOperator(char, "%c")

        StreamOperator(const char *, "%s")

        StreamOperator(int, "%d")

        StreamOperator(long, "%ld")

        StreamOperator(long long, "%lld")

        StreamOperator(unsigned, "%u")

        StreamOperator(unsigned long, "%lu")

        StreamOperator(unsigned long long, "%llu")

        StreamOperator(double, "%lf")

#undef StreamOperator

    private:

        Log *_log;

        int _rank;

        int _index = 0;

        char _message[256];

    };

}

/// 注意这些宏定义可能会造成意外的 else 悬挂。

#define TRACE(val) if (val.get_rank() <= Base::LogRank::TRACE) \
                        (val.stream(Base::LogRank::TRACE))

#define DEBUG(val) if (val.get_rank() <= Base::LogRank::DEBUG) \
                        (val.stream(Base::LogRank::DEBUG))

#define INFO(val) if (val.get_rank() <= Base::LogRank::INFO) \
                        (val.stream(Base::LogRank::INFO))

#define WARN(val) if (val.get_rank() <= Base::LogRank::WARN) \
                        (val.stream(Base::LogRank::WARN))

#define ERROR(val) if (val.get_rank() <= Base::LogRank::ERROR) \
                        (val.stream(Base::LogRank::ERROR))

#define FATAL(val) if (val.get_rank() <= Base::LogRank::FATAL) \
                        (val.stream(Base::LogRank::FATAL))


#define GLOBAL_LOG

#ifdef GLOBAL_LOG

constexpr char GLOBAL_LOG_PATH[] = "/home/taganyer/Codes/Clion_Project/test/logs";

static_assert(sizeof(GLOBAL_LOG_PATH) > 1, "GLOBAL_LOG_PATH cannot be empty");

extern Base::Log Global_Logger;

#define G_TRACE if (Global_Logger.get_rank() <= Base::LogRank::TRACE) \
                    (Global_Logger.stream(Base::LogRank::TRACE))

#define G_DEBUG if (Global_Logger.get_rank() <= Base::LogRank::DEBUG) \
                    (Global_Logger.stream(Base::LogRank::DEBUG))

#define G_INFO if (Global_Logger.get_rank() <= Base::LogRank::INFO) \
                    (Global_Logger.stream(Base::LogRank::INFO))

#define G_WARN if (Global_Logger.get_rank() <= Base::LogRank::WARN) \
                    (Global_Logger.stream(Base::LogRank::WARN))

#define G_ERROR if (Global_Logger.get_rank() <= Base::LogRank::ERROR) \
                    (Global_Logger.stream(Base::LogRank::ERROR))

#define G_FATAL if (Global_Logger.get_rank() <= Base::LogRank::FATAL) \
                    (Global_Logger.stream(Base::LogRank::FATAL))


#endif


#endif //BASE_LOG_HPP
