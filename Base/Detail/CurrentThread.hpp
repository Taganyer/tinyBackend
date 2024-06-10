//
// Created by taganyer on 24-2-20.
//

#ifndef BASE_CURRENTTHREAD_HPP
#define BASE_CURRENTTHREAD_HPP

#ifdef BASE_CURRENTTHREAD_HPP

#include <string>
#include "config.hpp"

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

    inline string& thread_name() { return Detail::this_thread_name; }

    void yield_this_thread();

}

#endif

#endif // BASE_CURRENTTHREAD_HPP
