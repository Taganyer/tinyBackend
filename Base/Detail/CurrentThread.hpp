//
// Created by taganyer on 24-2-20.
//

#ifndef TEST_CURRENTTHREAD_HPP
#define TEST_CURRENTTHREAD_HPP

#endif //TEST_CURRENTTHREAD_HPP

#include "config.hpp"
#include <string>

namespace Base::Detail {

    extern pthread_t main_thread_id;

    extern thread_local pid_t thread_pid;

    extern thread_local pthread_t thread_tid;

    extern thread_local std::string this_thread_name;

    std::string stackTrace(bool demangle);

}

namespace Base {

    using std::string;

    inline bool is_main_thread() { return Detail::main_thread_id == Detail::thread_tid; }

    inline pid_t pid() { return Detail::thread_pid; }

    inline pthread_t tid() { return Detail::thread_tid; }

    inline string &thread_name() { return Detail::this_thread_name; }

}