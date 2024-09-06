//
// Created by taganyer on 24-2-20.
//

#ifndef BASE_CURRENTTHREAD_HPP
#define BASE_CURRENTTHREAD_HPP

#ifdef BASE_CURRENTTHREAD_HPP

#include <string>
#include <exception>
#include <pthread.h>
#include <functional>

namespace Base {

    using std::string;

    class CurrentThread {
    public:
        /// 判断当前线程是否为主线程。
        static bool is_main_thread() { return main_thread_id == thread_tid; };

        /// 进程id
        static pid_t pid() { return thread_pid; };

        /// 线程id
        static pthread_t tid() { return thread_tid; };

        /// 线程名
        static string& thread_name() { return this_thread_name; };

        /// 放弃当前线程运行权。
        static void yield_this_thread();

        /// 获得函数调用栈。
        static string stackTrace(bool demangle = true);

        using ExitFun = void(*)();

        /// 当程序正常退出时，调用注册的函数。可注册多个函数，线程安全，调用顺序为后进先出。
        static bool set_exit_function(ExitFun fun);

        using ExceptionPtr = std::exception_ptr;

        using TerminalFun = std::function<void(ExceptionPtr)>;

        /// 当程序异常退出时，调用注册的函数。线程不安全，调用晚于 loacl_terminal_function。
        static void set_global_terminal_function(TerminalFun fun);

        /// 当程序异常退出时，调用注册的函数。每个线程可设置不同的调用函数，线程安全。
        static void set_local_terminal_function(TerminalFun fun);

        /// 设置错误信息输出文件并把该文件设置为无缓冲模式，默认为 stderr，为 nullptr 不输出错误信息。
        static void set_error_message_file(FILE *file);

        CurrentThread(const CurrentThread &) = delete;

        CurrentThread &operator=(const CurrentThread &) = delete;

    private:
        static TerminalFun global_terminal_function;

        static thread_local TerminalFun local_terminal_function;

        static FILE *error_message_file;

        static const pthread_t main_thread_id;

        static const thread_local pid_t thread_pid;

        static const thread_local pthread_t thread_tid;

        static thread_local string this_thread_name;

        friend class Thread;

        static void terminal();

    };

}

#endif

#endif // BASE_CURRENTTHREAD_HPP
