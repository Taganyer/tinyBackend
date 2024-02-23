//
// Created by taganyer on 23-12-28.
//

#include "Log.hpp"

namespace Base {

    namespace Detail {

        Log_basic::Log_basic(std::string dictionary_path) :
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

        Log_basic::~Log_basic() {
            stop = true;
            condition.notify_all();
            Lock l(IO_lock);
            condition.wait(l, [this] { return !stop; });
            clear_empty_buffer();
        }

        void Log_basic::get_new_buffer() {
            if (empty_queue) {
                current_queue = empty_queue;
                empty_queue = empty_queue->next;
                current_queue->next = nullptr;
            } else {
                current_queue = new Detail::Log_basic::Queue();
            }
        }

        void Log_basic::put_full_buffer() {
            auto target = full_queue;
            current_queue->next = nullptr;
            if (!target) {
                full_queue = current_queue;
            } else {
                while (target->next) target = target->next;
                target->next = current_queue;
            }
        }

        void Log_basic::put_to_empty_buffer(Log_basic::Queue *target) {
            if (!target) return;
            auto tail = target;
            while (tail->next) tail = tail->next;
            tail->next = empty_queue;
            empty_queue = target;
        }

        void Log_basic::clear_empty_buffer() {
            while (empty_queue) {
                auto temp = empty_queue;
                empty_queue = empty_queue->next;
                delete temp;
            }
        }

        void Log_basic::invoke(Log_basic::Queue *target) {
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

        void Log_basic::open_new_file() {
            if (out.is_open()) out.close();
            Time now = get_time_now();
            out.open(path + to_string(now, false) + ".log",
                     std::ios::binary | std::ios::out);
            if (!out.is_open()) throw IO_error("open new file failed.\n");
        }
    }


    void Log::push(int rank, const char *data, uint64 size) {
        auto time = get_time_now();
        Lock l(basic->IO_lock);
        if (!basic->current_queue) {
            basic->get_new_buffer();
            basic->current_queue->buffer.append(rank, time, data, size);
        } else if (!basic->current_queue->buffer.append(rank, time, data, size)) {
            basic->put_full_buffer();
            basic->get_new_buffer();
            if (!basic->current_queue->buffer.append(rank, time, data, size))
                throw Detail::IO_error("Log::push() failed.\n");
            basic->condition.notify_one();
        }
    }
}
