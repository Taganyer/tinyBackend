cmake_minimum_required(VERSION 3.27)

project(tinyBackend)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")

add_executable(tinyBackend main.cpp
        Base/Detail/config.hpp
        Base/Detail/CurrentThread.cpp
        Base/Detail/CurrentThread.hpp
        Base/Detail/Pool.hpp
        Base/Exception.cpp
        Base/Exception.hpp
        Base/Thread.cpp
        Base/Thread.hpp
        Base/ThreadPool.hpp
        Base/Time/Date.cpp
        Base/Time/Date.hpp
        Base/Time/Timer.cpp
        Base/Time/Timer.hpp
        Base/Time/TimeStamp.cpp
        Base/Time/TimeStamp.hpp
        Base/Log/Log_config.hpp
        Base/Log/Log.cpp
        Base/Log/Log.hpp
        Base/Mutex.hpp
        Base/Detail/NoCopy.hpp
        Base/Condition.hpp)

target_link_libraries(tinyBackend mysqlclient)

target_link_libraries(tinyBackend hiredis)
