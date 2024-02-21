//
// Created by taganyer on 23-12-27.
//

#ifndef TEST_LOG_HPP
#define TEST_LOG_HPP

#include "Log_config.hpp"

namespace Base {

    using std::mutex;
    using std::unique_lock;
    using std::condition_variable;
    using namespace std::chrono_literals;

    namespace Detail {
        class Log_basic {
        public:
            std::mutex IO_lock;
            std::condition_variable condition;
            bool stop = false;

            struct Queue {
                Log_buffer buffer;
                Queue *next = nullptr;

                Queue() = default;
            };

            Queue *empty_queue = nullptr;
            Queue *full_queue = nullptr;
            Queue *current_queue = nullptr;

            Log_basic(std::string dictionary_path);

            ~Log_basic();

            void get_new_buffer();

            void put_full_buffer();

        private:
            std::string path;

            std::ofstream out;

            size_t file_current_size = 0;

            inline void put_to_empty_buffer(Log_basic::Queue *target);

            inline void clear_empty_buffer();

            void invoke(Queue *target);

            inline void open_new_file();

        };
    }

    class Log {
    public:
        Log(std::string dictionary_path) :
                basic(std::make_shared<Detail::Log_basic>(std::move(dictionary_path))) {};

        void push(int rank, const char *data, size_t size);

    private:

        std::shared_ptr<Detail::Log_basic> basic;

    };

}

#endif //TEST_LOG_HPP
