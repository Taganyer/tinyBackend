cmake_minimum_required(VERSION 3.26)

#项目名
project(testing)

set(CMAKE_CXX_STANDARD 17)

find_package(tinyBackend REQUIRED)

FILE(GLOB_RECURSE SRC_LIST "*.cpp")

FILE(GLOB_RECURSE HEAD_LIST "*.hpp")

list(FILTER SRC_LIST EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/cmake-build-*")

add_executable(${PROJECT_NAME} ${SRC_LIST} ${HEAD_LIST})

# 链接 Base 库
target_link_libraries(${PROJECT_NAME}
        PRIVATE tinyBackend::Base
        PRIVATE tinyBackend::Net
        PRIVATE tinyBackend::Distributed
        PRIVATE tinyBackend::LogSystem)
