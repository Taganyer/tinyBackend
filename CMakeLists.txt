cmake_minimum_required(VERSION 3.26)

# 项目名
project(tinyBackend)

# C++ 语言标准
set(CMAKE_CXX_STANDARD 17)

# 设置编译器优化级别
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "Project root directory")

set(LOG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/global_logs CACHE INTERNAL "Project log directory")

# 生成配置头文件
configure_file(CMake_config.h.in "$CACHE{PROJECT_ROOT_DIR}/CMake_config.h")

# 添加子库
add_subdirectory(Base)

add_subdirectory(Net)

add_subdirectory(LogSystem)

add_subdirectory(AOP)

add_subdirectory(Distributed)

add_library(${PROJECT_NAME} CMake_config.h)

# 链接子库
target_link_libraries(${PROJECT_NAME} Base Net)

target_link_libraries(${PROJECT_NAME} LogSystem)

target_link_libraries(${PROJECT_NAME} AOP)

target_link_libraries(${PROJECT_NAME} Distributed)
