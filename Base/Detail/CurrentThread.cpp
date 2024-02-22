//
// Created by taganyer on 24-2-20.
//

#include "CurrentThread.hpp"
#include <sys/syscall.h>
#include <unistd.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <cstdlib>

namespace Base::Detail {

    pthread_t main_thread_id = syscall(SYS_gettid);

    thread_local pid_t thread_pid = getpid();

    thread_local pthread_t thread_tid = syscall(SYS_gettid);

    std::string get_thread_name() {
        if (main_thread_id != thread_tid) {
            std::string name("T_");
            name += std::to_string(thread_tid);
            return name;
        } else {
            return "Main_thread";
        }
    }

    thread_local std::string this_thread_name(get_thread_name());

    string stackTrace(bool demangle) {
        string stack;
        const int max_frames = 200;
        void *frame[max_frames];
        int nptrs = ::backtrace(frame, max_frames);
        if (char **strings = ::backtrace_symbols(frame, nptrs)) {
            size_t len = 256;
            char *demangled = demangle ? new char[len] : nullptr;
            for (int i = 1; i < nptrs; ++i)  // skipping the 0-th, which is this function
            {
                if (demangle) {
                    char *left_par = nullptr;
                    char *plus = nullptr;
                    for (char *p = strings[i]; *p; ++p) {
                        if (*p == '(')
                            left_par = p;
                        else if (*p == '+')
                            plus = p;
                    }

                    if (left_par && plus) {
                        *plus = '\0';
                        int status = 0;
                        char *ret = abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
                        *plus = '+';
                        if (status == 0) {
                            demangled = ret;  // ret could be realloc()
                            stack.append(strings[i], left_par + 1);
                            stack.append(demangled);
                            stack.append(plus);
                            stack.push_back('\n');
                            continue;
                        }
                    }
                }
                // Fallback to mangled names
                stack.append(strings[i]);
                stack.push_back('\n');
            }
            delete[] demangled;
            free(strings);
        }
        return stack;
    }
}