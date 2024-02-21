cmake_minimum_required(VERSION 3.27)

project(tinyBackend)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")

add_executable(tinyBackend main.cpp)

target_link_libraries(tinyBackend mysqlclient)

target_link_libraries(tinyBackend hiredis)
