//
// Created by taganyer on 23-12-28.
//

#include "../Log.hpp"
#include "Base/Exception.hpp"

namespace Base {

    Log::Log(SendThread &thread, std::string dictionary_path,
             LogRank rank, uint64 limit_size) :
            logSender(std::make_shared<LogSender>(&thread, std::move(dictionary_path), limit_size)), outputRank(rank) {
        logSender->_thread->add_sender(logSender->shared_from_this(), logSender->data);
    }

    Log::~Log() {
        if (logSender)
            logSender->_thread->remove_sender(logSender->data);
    }

    void Log::push(LogRank rank, const void *ptr, uint64 size) {
        if (rank < outputRank) return;
        Lock l(logSender->IO_lock);
        while (size) {
            uint64 ret = logSender->data.buffer->append(rank, get_time_now(), ptr, size);
            if (size > ret) {
                logSender->_thread->put_buffer(logSender->data);
                ptr = (char *) ptr + ret;
            }
            size -= ret;
        }
    }

    LogStream Log::stream(LogRank rank) {
        return {*this, rank};
    }

    void Log::LogSender::send(const void *buffer, uint64 size) {
        while (size) {
            uint64 rest = limit_size - current_size;
            if (size > rest) {
                _file.write(buffer, rest);
                size -= rest;
                buffer = (const char *) buffer + rest;
                open_new_file();
            } else {
                current_size += _file.write(buffer, size);
                break;
            }
        }
        _file.flush_to_disk();
    }

    void Log::LogSender::force_flush() {
        Lock l(IO_lock);
        _thread->put_buffer(data);
    }

    void Log::LogSender::open_new_file() {
        string path = _path + to_string(get_time_now(), true) + ".log";
        if (unlikely(!_file.open(path.c_str(), false, true)))
            throw Exception("fail to open: " + path);
        current_size = 0;
    }

    Log::LogSender::LogSender(SendThread *thread, std::string dictionary_path, uint64 limit_size) :
            _thread(thread), limit_size(limit_size), _path(std::move(dictionary_path)) {
        if (_path.back() != '/' && _path.back() != '\\') _path.push_back('/');
        open_new_file();
    }
}

#ifdef GLOBAL_SENDTHREAD

Base::SendThread Global_LogThread;

#ifdef GLOBAL_LOG

Base::Log Global_Logger(Global_LogThread, GLOBAL_LOG_PATH, Base::LogRank::TRACE);

#endif

#endif
