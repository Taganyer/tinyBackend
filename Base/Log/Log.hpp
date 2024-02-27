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

        Log(std::string dictionary_path);

        void push(int rank, const char *data, uint64 size);

        ~Log();

        LogStream stream(int rank);

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

        inline void get_new_buffer();

        inline void put_full_buffer();

        inline void put_to_empty_buffer(Queue *target);

        inline void clear_empty_buffer();

        inline void open_new_file();

        void invoke(Queue *target);

    };


    class LogStream {
    public:

        LogStream(Log &log, int rank) : _log(log), _rank(rank) {};

        ~LogStream() {
            _log.push(_rank, _info.c_str(), _info.size());
        };

        template<typename Type>
        LogStream &operator<<(const Type &val) {
            _info += std::to_string(val);
            return *this;
        };

#define StreamOperator(type) LogStream &operator<<(type val) { \
                _info += val;                                  \
                return *this;                                  \
        };

        StreamOperator(char)

        StreamOperator(char *)

        StreamOperator(const char *)

        StreamOperator(const std::string &)

        StreamOperator(std::string_view)

#undef StreamOperator

    private:

        Log &_log;

        int _rank;

        string _info;

    };

#define TRACE(val) (val.stream(LogRank::TRACE))

#define DEBUG(val) (val.stream(LogRank::DEBUG))

#define INFO(val) (val.stream(LogRank::INFO))

#define WARN(val) (val.stream(LogRank::WARN))

#define ERROR(val) (val.stream(LogRank::ERROR))

#define FATAL(val) (val.stream(LogRank::FATAL))



//#define GLOBAL_LOG


#ifdef GLOBAL_LOG

    constexpr char GLOBAL_LOG_PATH[] = "";

    static_assert(sizeof(GLOBAL_LOG_PATH) > 1, "GLOBAL_LOG_PATH cannot be empty");

    extern  Log Global_Logger;

#define G_TRACE (Global_Logger.stream(LogRank::TRACE))

#define G_DEBUG (Global_Logger.stream(LogRank::DEBUG))

#define G_INFO (Global_Logger.stream(LogRank::INFO))

#define G_WARN (Global_Logger.stream(LogRank::WARN))

#define G_ERROR (Global_Logger.stream(LogRank::ERROR))

#define G_FATAL (Global_Logger.stream(LogRank::FATAL))

#endif

}

#endif //BASE_LOG_HPP
