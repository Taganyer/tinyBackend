//
// Created by taganyer on 23-12-27.
//

#ifndef LOGSYSTEM_SYSTEMLOG_HPP
#define LOGSYSTEM_SYSTEMLOG_HPP

#ifdef LOGSYSTEM_SYSTEMLOG_HPP

#include "LogRank.hpp"
#include "Base/Detail/oFile.hpp"
#include "Base/ScheduledThread.hpp"
#include "Base/Buffer/BufferPool.hpp"

namespace LogSystem {

    class LogStream;

    constexpr uint64 FILE_LIMIT = 1 << 29;

    constexpr uint64 LOG_BUFFER_SIZE = 1 << 24;

    class SystemLog : Base::NoCopy {
    public:
        SystemLog(Base::ScheduledThread &thread, Base::BufferPool &buffer_pool,
                  std::string dictionary_path, LogRank rank,
                  uint64 file_limit_size = FILE_LIMIT, uint64 buffer_limit_size = LOG_BUFFER_SIZE);

        ~SystemLog();

        void push(LogRank rank, const void* ptr, uint64 size);

        LogStream stream(LogRank rank);

        void set_rank(LogRank rank) { outputRank = rank; };

        [[nodiscard]] LogRank get_rank() const { return outputRank; };

    private:
        class LogBuffer {
        public:
            explicit LogBuffer(Base::BufferPool::Buffer &&buffer) : _buffer(std::move(buffer)) {};

            uint64 append(LogRank rank, const Base::Time &time, const void* ptr, uint64 size);

            void clear() { _index = 0; };

            [[nodiscard]] const void* data() const { return _buffer.data(); };

            [[nodiscard]] uint64 size() const { return _index; };

            bool valid() const { return _buffer.size() > 0; };

        private:
            Base::BufferPool::Buffer _buffer;

            uint64 _index = 0;
        };

        class LogScheduler : public Base::Scheduler {
        public:
            Base::Mutex IO_lock;

            LogBuffer* _buffer = nullptr;

            Base::ScheduledThread* _thread;

            LogScheduler(Base::ScheduledThread* thread, std::string dictionary_path, uint64 limit_size);

        private:
            uint64 current_size = 0, limit_size;

            Base::oFile _file;

            std::string _path;

            void open_new_file();

            void write_to_file(const LogBuffer* logBuffer);

            void invoke(void* buffer_ptr) override;

            void force_invoke() override;

            friend class Base::ScheduledThread;
        };

        std::shared_ptr<LogScheduler> _scheduler;

        LogRank outputRank;

        Base::BufferPool* _bufferPool;

        uint64 _buffer_size;

        friend class Base::ScheduledThread;

    };


    class LogStream : private Base::NoCopy {
    public:
        LogStream(SystemLog &log, LogRank rank) : _log(&log), _rank(rank) {};

        ~LogStream() {
            _log->push(_rank, _message, _index);
        };

        LogStream& operator<<(const std::string &val) {
            if (_log->get_rank() > _rank) return *this;
            int temp = val.size() > 256 - _index ? 256 - _index : val.size();
            memcpy(_message + _index, val.data(), temp);
            _index += temp;
            return *this;
        };

        LogStream& operator<<(const std::string_view &val) {
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
        SystemLog* _log;

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


/// 解除注释开启全局 BufferPool 对象
#define GLOBAL_BUFFER_POOL
#ifdef GLOBAL_BUFFER_POOL

extern Base::BufferPool Global_BufferPool;
/// 解除注释开启全局 ScheduledThread 对象
#define GLOBAL_SCHEDULED_THREAD

#endif


#ifdef GLOBAL_SCHEDULED_THREAD

extern Base::Time_difference Global_ScheduledThread_FlushTime;
extern Base::ScheduledThread Global_ScheduledThread;
/// 解除注释开启全局日志
#define GLOBAL_LOG

#endif


#ifdef GLOBAL_LOG

#include "../CMake_config.h"

/// 设置全局日志文件夹路径
constexpr char GLOBAL_LOG_PATH[] = PROJECT_GLOBAL_LOG_PATH;

static_assert(sizeof(GLOBAL_LOG_PATH) > 1, "GLOBAL_LOG_PATH cannot be empty");

extern LogSystem::SystemLog Global_Logger;

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
