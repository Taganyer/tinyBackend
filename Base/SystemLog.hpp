//
// Created by taganyer on 23-12-27.
//

#ifndef LOGSYSTEM_SYSTEMLOG_HPP
#define LOGSYSTEM_SYSTEMLOG_HPP

#ifdef LOGSYSTEM_SYSTEMLOG_HPP

#include "LogRank.hpp"
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

/// FIXME 可能会存在 else 悬挂问题，使用时注意
#define TRACE(val) if ((val).get_rank() <= LogSystem::LogRank::TRACE) ((val).stream(LogSystem::LogRank::TRACE))

#define DEBUG(val) if ((val).get_rank() <= LogSystem::LogRank::DEBUG) ((val).stream(LogSystem::LogRank::DEBUG))

#define INFO(val) if ((val).get_rank() <= LogSystem::LogRank::INFO) ((val).stream(LogSystem::LogRank::INFO))

#define WARN(val) if ((val).get_rank() <= LogSystem::LogRank::WARN) ((val).stream(LogSystem::LogRank::WARN))

#define ERROR(val) if ((val).get_rank() <= LogSystem::LogRank::ERROR) ((val).stream(LogSystem::LogRank::ERROR))

#define FATAL(val) if ((val).get_rank() <= LogSystem::LogRank::FATAL) ((val).stream(LogSystem::LogRank::FATAL))

#endif

#endif //LOGSYSTEM_SYSTEMLOG_HPP
