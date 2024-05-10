cmake_minimum_required(VERSION 3.27)

project(tinyBackend)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")

add_executable(
        tinyBackend main.cpp

        Base/Buffer/RingBuffer.cpp
        Base/Buffer/RingBuffer.hpp
        Base/Detail/config.hpp
        Base/Detail/NoCopy.hpp
        Base/Detail/CurrentThread.cpp
        Base/Detail/CurrentThread.hpp
        Base/Detail/Pool.hpp
        Base/Detail/iFile.hpp
        Base/Detail/oFile.hpp
        Base/Detail/ioFile.hpp
        Base/Exception.cpp
        Base/Exception.hpp
        Base/Thread.cpp
        Base/Thread.hpp
        Base/ThreadPool.hpp
        Base/Mutex.hpp
        Base/Condition.hpp
        Base/BlockQueue.hpp
        Base/File.hpp

        Base/Time/Date.cpp
        Base/Time/Date.hpp
        Base/Time/Timer.cpp
        Base/Time/Timer.hpp
        Base/Time/TimeStamp.cpp
        Base/Time/TimeStamp.hpp

        Base/Container/List.hpp
        Base/Container/SingleList.hpp

        Base/Log/SendThread.cpp
        Base/Log/SendThread.hpp
        Base/Log/Sender.hpp
        Base/Log/SendBuffer.cpp
        Base/Log/SendBuffer.hpp
        Base/Log/LogRank.hpp
        Base/Log/Log.cpp
        Base/Log/Log.hpp

        Net/EventLoop.cpp
        Net/EventLoop.hpp
        Net/NetLink.cpp
        Net/NetLink.hpp
        Net/Socket.cpp
        Net/Socket.hpp
        Net/InetAddress.cpp
        Net/InetAddress.hpp
        Net/Reactor.cpp
        Net/Reactor.hpp
        Net/Acceptor.cpp
        Net/Acceptor.hpp
        Net/Controller.cpp
        Net/Controller.hpp

        Net/functions/Interface.cpp
        Net/functions/Interface.hpp
        Net/functions/poll_interface.hpp

        Net/file/FileDescriptor.cpp
        Net/file/FileDescriptor.hpp
        Net/file/FilePool.cpp
        Net/file/FilePool.hpp

        Net/monitors/Monitor.cpp
        Net/monitors/Monitor.hpp
        Net/monitors/Selector.cpp
        Net/monitors/Selector.hpp
        Net/monitors/Poller.cpp
        Net/monitors/Poller.hpp
        Net/monitors/EPoller.cpp
        Net/monitors/EPoller.hpp
        Net/monitors/Event.cpp
        Net/monitors/Event.hpp

        Net/error/error_mark.hpp
        Net/error/errors.cpp
        Net/error/errors.hpp
        Base/Buffer/BufferPool.cpp
        Base/Buffer/BufferPool.hpp
        Base/Detail/AsyncFun.hpp
        Base/ThreadPool.cpp

)

target_link_libraries(tinyBackend mysqlclient)

target_link_libraries(tinyBackend hiredis)
