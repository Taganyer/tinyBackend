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
        Base/Log/Log_buffer.hpp
        Base/Log/Log.cpp
        Base/Log/Log.hpp
        Base/Mutex.hpp
        Base/Detail/NoCopy.hpp
        Base/Condition.hpp
        Base/BlockQueue.hpp
        Base/File.hpp
        Base/Detail/iFile.hpp
        Base/Detail/oFile.hpp
        Net/Channel.cpp
        Net/Channel.hpp
        Net/Poller.cpp
        Net/Poller.hpp
        Net/poller/PollPoller.cpp
        Net/poller/PollPoller.hpp
        Net/poller/EpollPoller.cpp
        Net/poller/EpollPoller.hpp
        Base/RingBuffer.cpp
        Base/RingBuffer.hpp
        Net/EventLoop.cpp
        Net/EventLoop.hpp
        Net/ChannelsManger.cpp
        Net/ChannelsManger.hpp
        Net/NetLink.cpp
        Net/NetLink.hpp
        Net/functions/Interface.cpp
        Net/functions/Interface.hpp
        Net/functions/poll_interface.hpp
        Net/Socket.cpp
        Net/Socket.hpp
        Net/functions/errors.cpp
        Net/functions/errors.hpp
        Net/InetAddress.cpp
        Net/InetAddress.hpp)

target_link_libraries(tinyBackend mysqlclient)

target_link_libraries(tinyBackend hiredis)
