//
// Created by taganyer on 23-12-27.
//

#ifndef BASE_LOG_HPP
#define BASE_LOG_HPP

#include <memory>
#include "Log_config.hpp"
#include "../Thread.hpp"
#include "../Condition.hpp"

namespace Base {


    namespace Detail {
        class Log_basic {
        public:
            Mutex IO_lock;
            Condition condition;
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

        void push(int rank, const char *data, uint64 size);

    private:

        std::shared_ptr<Detail::Log_basic> basic;

    };

}

#endif //BASE_LOG_HPP
