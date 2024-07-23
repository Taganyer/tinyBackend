//
// Created by taganyer on 23-12-27.
//

#ifndef LOGSYSTEM_SYSTEMLOG_HPP
#define LOGSYSTEM_SYSTEMLOG_HPP

#ifdef LOGSYSTEM_SYSTEMLOG_HPP

#include "LogRank.hpp"
#include "Send/SendThread.hpp"


namespace LogSystem {

    class LogStream;

    constexpr uint64 FILE_LIMIT = 2 << 30;

    class SystemLog : private Base::NoCopy {
    public:

        SystemLog(SendThread &thread, std::string dictionary_path,
            LogRank rank, uint64 limit_size = FILE_LIMIT);

        ~SystemLog();

        void push(LogRank rank, const void *ptr, uint64 size);

        LogStream stream(LogRank rank);

        void set_rank(LogRank rank) { outputRank = rank; };

        [[nodiscard]] LogRank get_rank() const { return outputRank; };

    private:

        class LogSender : public Sender {
        public:

            Base::Mutex IO_lock;

            SendThread *_thread;

            SendThread::Data data;

            LogSender(SendThread *thread, std::string dictionary_path, uint64 limit_size);

        private:

            uint64 current_size = 0, limit_size;

            Base::oFile _file;

            std::string _path;

            void send(const void *buffer, uint64 size) override;

            void force_flush() override;

            void open_new_file();

            friend class SendThread;
        };

        std::shared_ptr<LogSender> logSender;

        LogRank outputRank;

        friend class SendThread;

    };


    class LogStream : private Base::NoCopy {
    public:

        LogStream(SystemLog &log, LogRank rank) : _log(&log), _rank(rank) {};

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

        SystemLog *_log;

        LogRank _rank;

        int _index = 0;

        char _message[256];

    };

}

/// FIXME 可能会存在 else 悬挂问题，使用时注意
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


/// 解除注释开启全局 SendThread 对象
#define GLOBAL_SENDTHREAD

#ifdef GLOBAL_SENDTHREAD

extern LogSystem::SendThread Global_LogThread;

/// 解除注释开启全局日志
#define GLOBAL_LOG

#ifdef GLOBAL_LOG

#include "../CMake_config.h"

/// 设置全局日志文件夹路径
constexpr char GLOBAL_LOG_PATH[] = PROJECT_GLOBAL_LOG_PATH;

static_assert(sizeof(GLOBAL_LOG_PATH) > 1, "GLOBAL_LOG_PATH cannot be empty");

extern LogSystem::SystemLog Global_Logger;

#endif

#endif

#ifdef GLOBAL_LOG

/// FIXME 可能会存在 else 悬挂问题，使用时注意
#define G_TRACE if (Global_Logger.get_rank() <= LogSystem::LogRank::TRACE) \
                    (Global_Logger.stream(LogSystem::LogRank::TRACE))

#define G_DEBUG if (Global_Logger.get_rank() <= LogSystem::LogRank::DEBUG) \
                    (Global_Logger.stream(LogSystem::LogRank::DEBUG))

#define G_INFO if (Global_Logger.get_rank() <= LogSystem::LogRank::INFO) \
                    (Global_Logger.stream(LogSystem::LogRank::INFO))

#define G_WARN if (Global_Logger.get_rank() <= LogSystem::LogRank::WARN) \
                    (Global_Logger.stream(LogSystem::LogRank::WARN))

#define G_ERROR if (Global_Logger.get_rank() <= LogSystem::LogRank::ERROR) \
                    (Global_Logger.stream(LogSystem::LogRank::ERROR))

#define G_FATAL if (Global_Logger.get_rank() <= LogSystem::LogRank::FATAL) \
                    (Global_Logger.stream(LogSystem::LogRank::FATAL))

#else

class Empty {
public:

#define Empty_fun(type) Empty &operator<<(type) { \
        return *this;                             \
};

    Empty_fun(const std::string &)

    Empty_fun(const std::string_view &)

    Empty_fun(char)

    Empty_fun(const char *)

    Empty_fun(int)

    Empty_fun(long)

    Empty_fun(long long)

    Empty_fun(unsigned)

    Empty_fun(unsigned long)

    Empty_fun(unsigned long long)

    Empty_fun(double)

#undef Empty_fun

};

/// FIXME 可能会存在 else 悬挂问题，使用时注意
#define G_TRACE if (false) (Empty())

#define G_DEBUG if (false) (Empty())

#define G_INFO if (false) (Empty())

#define G_WARN if (false) (Empty())

#define G_ERROR if (false) (Empty())

#define G_FATAL if (false) (Empty())

#endif

#endif

#endif //LOGSYSTEM_SYSTEMLOG_HPP
