//
// Created by taganyer on 24-10-29.
//

#ifndef LOGSYSTEM_GLOBALOBJECT_HPP
#define LOGSYSTEM_GLOBALOBJECT_HPP

#ifdef LOGSYSTEM_GLOBALOBJECT_HPP


/// 解除注释开启全局 BufferPool 对象
#define GLOBAL_BUFFER_POOL
#ifdef GLOBAL_BUFFER_POOL
#include "tinyBackend/Base/Buffer/BufferPool.hpp"

inline Base::BufferPool Global_BufferPool(1 << 28);
/// 解除注释开启全局 ScheduledThread 对象
#define GLOBAL_SCHEDULED_THREAD

#endif

#ifdef GLOBAL_SCHEDULED_THREAD
#include "tinyBackend/Base/ScheduledThread.hpp"

constexpr Base::TimeInterval Global_ScheduledThread_FlushTime(Base::operator ""_s(1));
inline Base::ScheduledThread Global_ScheduledThread(Global_ScheduledThread_FlushTime);
/// 解除注释开启全局日志
#define GLOBAL_LOG

#endif


/// FIXME 可能会存在 else 悬挂问题，使用时注意
#define TRACE(val) if ((val).get_rank() <= LogSystem::LogRank::TRACE) \
                        ((val).stream(LogSystem::LogRank::TRACE))

#define DEBUG(val) if ((val).get_rank() <= LogSystem::LogRank::DEBUG) \
                        ((val).stream(LogSystem::LogRank::DEBUG))

#define INFO(val) if ((val).get_rank() <= LogSystem::LogRank::INFO) \
                        ((val).stream(LogSystem::LogRank::INFO))

#define WARN(val) if ((val).get_rank() <= LogSystem::LogRank::WARN) \
                        ((val).stream(LogSystem::LogRank::WARN))

#define ERROR(val) if ((val).get_rank() <= LogSystem::LogRank::ERROR) \
                        ((val).stream(LogSystem::LogRank::ERROR))

#define FATAL(val) if ((val).get_rank() <= LogSystem::LogRank::FATAL) \
                        ((val).stream(LogSystem::LogRank::FATAL))


#endif

#endif //LOGSYSTEM_GLOBALOBJECT_HPP
