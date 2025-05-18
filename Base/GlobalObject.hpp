//
// Created by taganyer on 24-10-29.
//

#pragma once

#include "tinyBackend/CMake_config.h"
#include "SystemLog.hpp"

#ifdef GLOBAL_OBJETS

inline Base::BufferPool Global_BufferPool(Global_BufferPool_SIZE);

constexpr Base::TimeInterval Global_ScheduledThread_FlushTime(Base::operator ""_ms(Global_ScheduledThread_FlushTime_MS));

inline Base::ScheduledThread Global_ScheduledThread(Global_ScheduledThread_FlushTime);

inline LogSystem::SystemLog Global_Logger(Global_ScheduledThread, Global_BufferPool, GLOBAL_LOG_PATH, LogSystem::Global_Logger_RANK);

/// FIXME 可能会存在 else 悬挂问题，使用时注意
#define G_TRACE TRACE(Global_Logger)

#define G_DEBUG DEBUG(Global_Logger)

#define G_INFO INFO(Global_Logger)

#define G_WARN WARN(Global_Logger)

#define G_ERROR ERROR(Global_Logger)

#define G_FATAL FATAL(Global_Logger)

#else

class Empty {
public:
#define Empty_fun(type) constexpr Empty &operator<<(type) { return *this; };

    Empty_fun(const std::string &)

    Empty_fun(const std::string_view &)

    Empty_fun(char)

    Empty_fun(const char *)

    Empty_fun(int)

    Empty_fun(long)

    Empty_fun(long long)

    Empty_fun(unsigned)

    Empty_fun(unsigned long)

    Empty_fun(unsigned long long)

    Empty_fun(double)

#undef Empty_fun

};

/// FIXME 可能会存在 else 悬挂问题，使用时注意
#define G_TRACE if (false) (Empty())

#define G_DEBUG if (false) (Empty())

#define G_INFO if (false) (Empty())

#define G_WARN if (false) (Empty())

#define G_ERROR if (false) (Empty())

#define G_FATAL if (false) (Empty())

#endif
