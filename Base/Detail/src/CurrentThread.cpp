//
// Created by taganyer on 24-2-20.
//

#include <cstring>
#include <unistd.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <sys/syscall.h>
#include "../CurrentThread.hpp"
#include "tinyBackend/Base/Exception.hpp"

using namespace Base;

CurrentThread::TerminalFun CurrentThread::global_terminal_function;

thread_local CurrentThread::TerminalFun CurrentThread::local_terminal_function;

FILE *CurrentThread::error_message_file = stderr;

const pthread_t CurrentThread::main_thread_id = [] {
    std::set_terminate(terminal);
    return syscall(SYS_gettid);
}();

const thread_local pid_t CurrentThread::thread_pid = getpid();

const thread_local pthread_t CurrentThread::thread_tid = syscall(SYS_gettid);

thread_local string CurrentThread::this_thread_name = [] {
    if (!is_main_thread()) {
        string name("T_");
        name += std::to_string(tid());
        return name;
    } else {
        return string { "Main_thread" };
    }
}();

thread_local void *CurrentThread::this_thread_mark_ptr = nullptr;

void CurrentThread::yield_this_thread() {
    sched_yield();
}

string CurrentThread::stackTrace(bool demangle) {
    string stack;
    constexpr int max_frames = 200;
    void *frame[max_frames];
    int nptrs = ::backtrace(frame, max_frames);
    if (char **strings = ::backtrace_symbols(frame, nptrs)) {
        for (int i = 1; i < nptrs; ++i) {
            if (demangle) {
                char *left_par = nullptr, *plus = nullptr;
                for (char *p = strings[i]; *p; ++p) {
                    if (*p == '(') left_par = p;
                    else if (*p == '+') plus = p;
                }

                if (left_par && plus && left_par < plus) {
                    *plus = '\0';
                    int status = 0;
                    char *ret = abi::__cxa_demangle(left_par + 1, nullptr, nullptr, &status);
                    *plus = '+';
                    if (status == 0) {
                        stack.append(strings[i], left_par + 1);
                        stack.append(ret);
                        stack.append(plus);
                        stack.push_back('\n');
                        std::free(ret);
                        continue;
                    }
                }
            }
            stack.append(strings[i]);
            stack.push_back('\n');
        }
        std::free(strings);
    } else {
        stack = "backtrace_symbols analyze failed.";
    }
    return stack;
}

bool CurrentThread::set_exit_function(ExitFun fun) {
    return std::atexit(fun) == 0;
}

void CurrentThread::emergency_exit(const string& message) {
    fprintf(error_message_file, "emergency exit in Thread %s:\n", thread_name().c_str());
    fprintf(error_message_file, "errno: %d %s\n", errno, strerror(errno));
    fprintf(error_message_file, "message: %s\n", message.c_str());
    fflush(error_message_file);
    abort();
}

void CurrentThread::exit(const string& message) {
    fprintf(error_message_file, "exit in Thread %s:\n", thread_name().c_str());
    fprintf(error_message_file, "errno: %d %s\n", errno, strerror(errno));
    fprintf(error_message_file, "message: %s\n", message.c_str());
    fflush(error_message_file);
    terminal();
}

void CurrentThread::print_error_message(const string& message) {
    fprintf(error_message_file, "error message: %s\n", message.c_str());
    fflush(error_message_file);
}

void CurrentThread::set_global_terminal_function(TerminalFun fun) {
    global_terminal_function = std::move(fun);
}

void CurrentThread::set_local_terminal_function(TerminalFun fun) {
    local_terminal_function = std::move(fun);
}

void CurrentThread::set_error_message_file(FILE *file) {
    error_message_file = file;
    setvbuf(file, nullptr, _IONBF, 0);
}

void CurrentThread::terminal() {
    auto eptr = std::current_exception();
    if (eptr && error_message_file) {
        try {
            std::rethrow_exception(eptr);
        } catch (const Exception& ex) {
            fprintf(error_message_file, "Base::Exception in Thread %s:\n", thread_name().c_str());
            fprintf(error_message_file, "Reason: %s:\n", ex.what());
            fprintf(error_message_file, "errno: %d %s\n", errno, strerror(errno));
            fprintf(error_message_file, "StackTrace: %s:\n", ex.stackTrace());
            fflush(error_message_file);
        } catch (const std::exception& ex) {
            fprintf(error_message_file, "std::exception in Thread %s:\n", thread_name().c_str());
            fprintf(error_message_file, "Reason: %s:\n", ex.what());
            fprintf(error_message_file, "errno: %d %s\n", errno, strerror(errno));
            fprintf(error_message_file, "StackTrace: %s:\n", stackTrace().data());
            fflush(error_message_file);
        } catch (...) {
            fprintf(error_message_file, "Unknown exception in Thread %s:\n", thread_name().c_str());
            fprintf(error_message_file, "errno: %d %s\n", errno, strerror(errno));
            fprintf(error_message_file, "StackTrace: %s:\n", stackTrace().data());
            fflush(error_message_file);
        }
    }
    try {
        if (local_terminal_function)
            local_terminal_function(eptr);
    } catch (...) {
        fprintf(error_message_file, "Unknown exception in local_terminal_function\n");
        fprintf(error_message_file, "errno: %d %s\n", errno, strerror(errno));
        fflush(error_message_file);
        abort();
    }
    try {
        if (global_terminal_function)
            global_terminal_function(eptr);
    } catch (...) {
        fprintf(error_message_file, "Unknown exception in global_terminal_function\n");
        fprintf(error_message_file, "errno: %d %s\n", errno, strerror(errno));
        fflush(error_message_file);
        abort();
    }
}
