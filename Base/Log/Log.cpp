//
// Created by taganyer on 23-12-28.
//

#include "Log.hpp"
#include "../Exception.hpp"

namespace Base {

    Log::Log(std::string dictionary_path) :
            path(std::move(dictionary_path)) {
        if (path.back() != '/') path.push_back('/');
        open_new_file();
        Thread thread([this] {
            Queue *target = nullptr;
            while (!stop) {
                {
                    Lock l(IO_lock);
                    clear_empty_buffer();
                    put_to_empty_buffer(target);
                    if (condition.wait_for(l, FLUSH_TIME)) {
                        target = full_queue;
                        full_queue = nullptr;
                    } else {
                        if (full_queue) {
                            target = full_queue;
                            full_queue = nullptr;
                        } else {
                            target = current_queue;
                            current_queue = nullptr;
                        }
                    }
                }
                invoke(target);
            }
            put_to_empty_buffer(target);
            if (full_queue) {
                invoke(full_queue);
                put_to_empty_buffer(full_queue);
            }
            if (current_queue) {
                invoke(current_queue);
                put_to_empty_buffer(current_queue);
            }
            stop = false;
            condition.notify_one();
        });
        thread.start();
    }

    Log::~Log() {
        stop = true;
        condition.notify_all();
        Lock l(IO_lock);
        condition.wait(l, [this] { return !stop; });
        clear_empty_buffer();
    }

    void Log::get_new_buffer() {
        if (empty_queue) {
            current_queue = empty_queue;
            empty_queue = empty_queue->next;
            current_queue->next = nullptr;
        } else {
            current_queue = new Log::Queue();
        }
    }

    void Log::put_full_buffer() {
        auto target = full_queue;
        current_queue->next = nullptr;
        if (!target) {
            full_queue = current_queue;
        } else {
            while (target->next) target = target->next;
            target->next = current_queue;
        }
    }

    void Log::put_to_empty_buffer(Queue *target) {
        if (!target) return;
        auto tail = target;
        while (tail->next) tail = tail->next;
        tail->next = empty_queue;
        empty_queue = target;
    }

    void Log::clear_empty_buffer() {
        while (empty_queue) {
            auto temp = empty_queue;
            empty_queue = empty_queue->next;
            delete temp;
        }
    }

    void Log::invoke(Log::Queue *target) {
        while (target) {
            auto size = target->buffer.size();
            auto data = target->buffer.get();
            while (size) {
                if (file_current_size + size > FILE_RESTRICT) {
                    auto i = FILE_RESTRICT - file_current_size;
                    while (i && data[i] != '\n') --i;
                    file_current_size = 0;
                    if (i) {
                        out.write(data, i);
                        data += i;
                        size -= i;
                    }
                    open_new_file();
                } else {
                    file_current_size += size;
                    out.write(data, size);
                    break;
                }
            }
            target->buffer.flush();
            target = target->next;
        }
    }

    void Log::open_new_file() {
        string file_name = path + to_string(get_time_now(), false) + ".log";
        out.open(file_name.c_str(), false, true);
        if (!out) throw Exception("open new file failed.\n");
    }


    void Log::push(int rank, const char *data, uint64 size) {
        auto time = get_time_now();
        Lock l(IO_lock);
        if (!current_queue) {
            get_new_buffer();
            current_queue->buffer.append(rank, time, data, size);
        } else if (!current_queue->buffer.append(rank, time, data, size)) {
            put_full_buffer();
            get_new_buffer();
            if (!current_queue->buffer.append(rank, time, data, size))
                throw Exception("Log::push() failed.\n");
            condition.notify_one();
        }
    }

#ifdef GLOBAL_LOG

    Log Global_Logger(GLOBAL_LOG_PATH);

#endif

}
