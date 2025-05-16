#项目名
project(testing)

set(CMAKE_CXX_STANDARD 17)

FILE(GLOB_RECURSE SRC_LIST "*.cpp")

FILE(GLOB_RECURSE HEAD_LIST "*.hpp")

add_executable(${PROJECT_NAME} ${SRC_LIST} ${HEAD_LIST})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

# 链接 Base 库
target_link_libraries(${PROJECT_NAME} PRIVATE tinyBackend)
