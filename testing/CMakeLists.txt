cmake_minimum_required(VERSION 3.26)

#项目名
project(tinyBackend_testing)

set(CMAKE_CXX_STANDARD 17)

add_compile_options(
        -Wall
        $<$<CONFIG:Debug>:-g>    # 调试模式添加调试符号
        $<$<CONFIG:Release>:-O3> # 发布模式添加优化
)

find_package(tinyBackend REQUIRED)

FILE(GLOB_RECURSE SRC_LIST "*.cpp")

FILE(GLOB_RECURSE HEAD_LIST "*.hpp")

list(FILTER SRC_LIST EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/cmake-build-*")

add_executable(${PROJECT_NAME} ${SRC_LIST} ${HEAD_LIST})

target_link_libraries(${PROJECT_NAME}
        PRIVATE tinyBackend::Base
        PRIVATE tinyBackend::Net
        PRIVATE tinyBackend::Distributed
        PRIVATE tinyBackend::LogSystem)
