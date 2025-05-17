//
// Created by taganyer on 23-12-27.
//

#ifndef LOGSYSTEM_SYSTEMLOG_HPP
#define LOGSYSTEM_SYSTEMLOG_HPP

#ifdef LOGSYSTEM_SYSTEMLOG_HPP

#include "LogRank.hpp"
#include "GlobalObject.hpp"
#include "tinyBackend/Base/Detail/oFile.hpp"
#include "tinyBackend/Base/ScheduledThread.hpp"
#include "tinyBackend/Base/Buffer/BufferPool.hpp"

namespace LogSystem {

    class LogStream;

    constexpr uint64 FILE_LIMIT = 1 << 28;

    constexpr uint64 LOG_BUFFER_SIZE = 1 << 24;

    class SystemLog : Base::NoCopy {
    public:
        SystemLog(Base::ScheduledThread& thread, Base::BufferPool& buffer_pool,
                  std::string dictionary_path, LogRank rank,
                  uint64 file_limit_size = FILE_LIMIT, uint64 buffer_limit_size = LOG_BUFFER_SIZE);

        ~SystemLog();

        void push(LogRank rank, const void *ptr, uint64 size) const;

        LogStream stream(LogRank rank);

        void flush() const;

        void set_rank(LogRank rank) { outputRank = rank; };

        [[nodiscard]] LogRank get_rank() const { return outputRank; };

    private:
        class LogBuffer {
        public:
            explicit LogBuffer(Base::BufferPool::Buffer&& buffer) : _buffer(std::move(buffer)) {};

            uint64 append(LogRank rank, const Base::Time& time, const void *ptr, uint64 size);

            void clear() { _index = 0; };

            [[nodiscard]] const void* data() const { return _buffer.data(); };

            [[nodiscard]] uint64 size() const { return _index; };

            [[nodiscard]] bool valid() const { return _buffer.size() > 0; };

        private:
            Base::BufferPool::Buffer _buffer;

            uint64 _index = 0;
        };

        class LogScheduler final : public Base::Scheduler {
        public:
            Base::Mutex IO_lock;

            LogBuffer *_buffer = nullptr;

            Base::ScheduledThread *_thread;

            LogScheduler(Base::ScheduledThread *thread, std::string dictionary_path, uint64 limit_size);

        private:
            uint64 current_size = 0, limit_size;

            Base::oFile _file;

            std::string _path;

            void open_new_file();

            void write_to_file(const LogBuffer *logBuffer);

            void invoke(void *buffer_ptr) override;

            void force_invoke() override;

            friend class Base::ScheduledThread;

            friend class SystemLog;
        };

        std::shared_ptr<LogScheduler> _scheduler;

        LogRank outputRank;

        Base::BufferPool *_bufferPool;

        uint64 _buffer_size;

        friend class Base::ScheduledThread;

    };


    class LogStream : Base::NoCopy {
    public:
        static constexpr uint32 BUFFER_SIZE = 256;

        LogStream(SystemLog& log, LogRank rank) : _log(&log), _rank(rank) {};

        ~LogStream() {
            _log->push(_rank, _message, _index);
        };

        LogStream& operator<<(const std::string& val) {
            if (_log->get_rank() > _rank) return *this;
            auto len = val.size() > BUFFER_SIZE - _index ? BUFFER_SIZE - _index : val.size();
            memcpy(_message + _index, val.data(), len);
            _index += len;
            return *this;
        };

        LogStream& operator<<(const std::string_view& val) {
            if (_log->get_rank() > _rank) return *this;
            auto len = val.size() > BUFFER_SIZE - _index ? BUFFER_SIZE - _index : val.size();
            memcpy(_message + _index, val.data(), len);
            _index += len;
            return *this;
        };

#define StreamOperator(type, f) LogStream &operator<<(type val) { \
                if (_log->get_rank() > _rank) return *this;       \
                _index += snprintf(_message + _index, BUFFER_SIZE - _index, f, val); \
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

        uint32 _index = 0;

        char _message[BUFFER_SIZE] {};

    };

}


#ifdef GLOBAL_SCHEDULED_THREAD
#ifdef GLOBAL_BUFFER_POOL

#define GLOBAL_LOG

#endif
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
#define G_TRACE TRACE(Global_Logger)

#define G_DEBUG DEBUG(Global_Logger)

#define G_INFO INFO(Global_Logger)

#define G_WARN WARN(Global_Logger)

#define G_ERROR ERROR(Global_Logger)

#define G_FATAL FATAL(Global_Logger)

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
