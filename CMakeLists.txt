cmake_minimum_required(VERSION 3.26)

# 项目名
project(tinyBackend)

# C++ 语言标准
set(CMAKE_CXX_STANDARD 17)

# 设置编译器优化级别
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")


# 获取当前项目的绝对路径
get_filename_component(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

# 将项目路径传递给配置头文件
set(PROJECT_ROOT_DIR ${PROJECT_ROOT_DIR} CACHE INTERNAL "Project root directory")

# 生成配置头文件
configure_file(CMake_config.h.in "$CACHE{PROJECT_ROOT_DIR}/CMake_config.h")


# 添加 Base 子库
add_subdirectory(Base)

# 添加 Net 子库
add_subdirectory(Net)

add_subdirectory(AOP)

add_subdirectory(LogSystem)

# 生成可执行文件
add_executable(${PROJECT_NAME} main.cpp CMake_config.h)

# 链接 Base Net 子库
target_link_libraries(${PROJECT_NAME} Base Net AOP LogSystem)
