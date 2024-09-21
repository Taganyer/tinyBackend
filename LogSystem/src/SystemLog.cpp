//
// Created by taganyer on 23-12-28.
//

#include "../SystemLog.hpp"
#include "Base/Exception.hpp"
#include "Base/Time/TimeStamp.hpp"

using namespace Base;

using namespace LogSystem;

SystemLog::SystemLog(ScheduledThread &thread, BufferPool &buffer_pool,
                     std::string dictionary_path, LogRank rank,
                     uint64 file_limit_size, uint64 buffer_limit_size) :
    _scheduler(std::make_shared<LogScheduler>(&thread, std::move(dictionary_path), file_limit_size)),
    outputRank(rank), _bufferPool(&buffer_pool), _buffer_size(buffer_limit_size) {
    thread.add_scheduler(_scheduler);
    _scheduler->_buffer = new LogBuffer(buffer_pool.get(buffer_limit_size));
}

SystemLog::~SystemLog() {
    Lock l(_scheduler->IO_lock);
    if (_scheduler) {
        _scheduler->_thread->remove_scheduler_and_invoke(_scheduler, _scheduler->_buffer);
        _scheduler->_buffer = nullptr;
    }
}

void SystemLog::push(LogRank rank, const void* ptr, uint64 size) {
    if (rank < outputRank || _scheduler->_thread->closed()) return;
    Lock l(_scheduler->IO_lock);
    while (size) {
        uint64 ret = _scheduler->_buffer->append(rank, get_time_now(), ptr, size);
        if (size > ret) {
            _scheduler->_thread->submit_task(*_scheduler, _scheduler->_buffer);
            _scheduler->_buffer = new LogBuffer(_bufferPool->get(LOG_BUFFER_SIZE));
            if (!_scheduler->_buffer->valid()) {
                /// FIXME memory overflow
                break;
            }
            ptr = (const char *) ptr + ret;
        }
        size -= ret;
    }
}

LogStream SystemLog::stream(LogRank rank) {
    return { *this, rank };
}

uint64 SystemLog::LogBuffer::append(LogRank rank, const Time &time,
                                    const void* ptr, uint64 size) {
    if (_buffer.size() - _index < size + TimeStamp::Time_us_format_len + 8)
        return 0;
    char* buffer = _buffer.data();
    format(buffer + _index, time);
    _index += TimeStamp::Time_us_format_len;
    buffer[_index++] = ' ';
    _index += rank_toString(buffer + _index, rank);
    std::memcpy(buffer + _index, ptr, size);
    buffer[_index += size] = '\n';
    ++_index;
    return size;
}

SystemLog::LogScheduler::LogScheduler(ScheduledThread* thread, std::string dictionary_path,
                                      uint64 limit_size) : _thread(thread), limit_size(limit_size),
    _path(std::move(dictionary_path)) {
    if (_path.back() != '/' && _path.back() != '\\') _path.push_back('/');
    open_new_file();
}

void SystemLog::LogScheduler::open_new_file() {
    string path = _path + to_string(get_time_now(), true) + ".log";
    if (unlikely(!_file.open(path.c_str(), false, true)))
        throw Exception("fail to open: " + path);
    current_size = 0;
}

void SystemLog::LogScheduler::write_to_file(const LogBuffer* logBuffer) {
    uint64 size = logBuffer->size();
    auto buffer = (const char *) logBuffer->data();
    while (size) {
        uint64 rest = limit_size - current_size;
        if (size > rest) {
            _file.write(buffer, rest);
            size -= rest;
            buffer = buffer + rest;
            open_new_file();
        } else {
            current_size += _file.write(buffer, size);
            break;
        }
    }
    _file.flush_to_disk();
}

void SystemLog::LogScheduler::invoke(void* buffer_ptr) {
    if (!buffer_ptr) return;
    auto logBuffer = (const LogBuffer *) buffer_ptr;
    write_to_file(logBuffer);
    delete logBuffer;
}

void SystemLog::LogScheduler::force_invoke() {
    Lock l(IO_lock);
    if (!_buffer || _buffer->size() == 0) return;
    write_to_file(_buffer);
    _buffer->clear();
}


#ifdef GLOBAL_BUFFER_POOL

BufferPool Global_BufferPool(1 << 28);

#endif


#ifdef GLOBAL_SCHEDULED_THREAD

Time_difference Global_ScheduledThread_FlushTime(1_s);
ScheduledThread Global_ScheduledThread(Global_ScheduledThread_FlushTime);

#endif


#ifdef GLOBAL_LOG

SystemLog Global_Logger(Global_ScheduledThread, Global_BufferPool, GLOBAL_LOG_PATH, TRACE);

#endif
