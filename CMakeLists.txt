cmake_minimum_required(VERSION 3.26)

# 项目名
project(tinyBackend)

# C++ 语言标准
set(CMAKE_CXX_STANDARD 17)

# 设置编译器优化级别
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")

set(PROJECT_ROOT_NAME ${PROJECT_NAME} CACHE STRING "Project root name" FORCE)

set(PROJECT_ROOT_DIR ${PROJECT_SOURCE_DIR} CACHE PATH "Project root directory" FORCE)

set(CONFIG_PREFIX "${PROJECT_ROOT_DIR}/cmake_config" CACHE PATH "config prefix" FORCE)

set(CMAKE_BUILD_PREFIX "${PROJECT_ROOT_DIR}/build" CACHE PATH "build prefix" FORCE)

#set(CMAKE_INSTALL_PREFIX "${PROJECT_ROOT_DIR}/install" CACHE PATH "Install prefix" FORCE)

if (NOT DEFINED LOG_PATH)
    set(LOG_PATH ${PROJECT_ROOT_DIR}/global_logs CACHE PATH "Project log directory" FORCE)
endif ()

configure_file("${CONFIG_PREFIX}/CMake_config.h.in" "${PROJECT_ROOT_DIR}/CMake_config.h")

# 设置安装路径
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME} CACHE PATH "Project includes directory" FORCE)
set(INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME} CACHE PATH "Project library directory" FORCE)
set(INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME} CACHE PATH "Project run time directory" FORCE)
set(INSTALL_CONFIG_DIR ${CMAKE_INSTALL_PREFIX}/lib/cmake/${PROJECT_NAME} CACHE PATH "Project cmake config directory" FORCE)


install(FILES
        ${PROJECT_ROOT_DIR}/CMake_config.h
        DESTINATION ${INSTALL_INCLUDE_DIR}
)

# uninstall target
if(NOT TARGET uninstall)
    configure_file(
            "${CONFIG_PREFIX}/cmake_uninstall.cmake.in"
            "${CMAKE_BUILD_PREFIX}/cmake_uninstall.cmake"
            IMMEDIATE @ONLY)

    message(TRACE "${CMAKE_BINARY_DIR}")

    add_custom_target(uninstall
            COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()


set(ADD_AOP ON CACHE BOOL "Enable AOP components" FORCE)
set(ADD_LOGSYSTEM ON CACHE BOOL "Enable LogSystem components" FORCE)
set(ADD_DISTRIBUTED ON CACHE BOOL "Enable distributed components" FORCE)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.12)
    cmake_parse_arguments(
            ARG "" "" "ADD_LOGSYSTEM;ADD_AOP;ADD_DISTRIBUTED" ${CMAKE_ARGV}
    )
    if (ARG_ADD_LOGSYSTEM)
        set(ADD_LOGSYSTEM ${ARG_ADD_LOGSYSTEM} FORCE)
    endif ()
    if (ARG_ADD_AOP)
        set(ADD_AOP ${ARG_ADD_AOP} FORCE)
    endif ()
    if (ARG_ADD_DISTRIBUTED)
        set(ADD_DISTRIBUTED ${ARG_ADD_DISTRIBUTED} FORCE)
    endif ()
endif ()

set(LIBRARY_LIST)

set(Base_VERSION 1.0 CACHE STRING "Base Version")
add_subdirectory(Base)
list(APPEND LIBRARY_LIST Base)

set(Net_VERSION 1.0 CACHE STRING "Net Version")
add_subdirectory(Net)
list(APPEND LIBRARY_LIST Net)

if (ADD_AOP)
    set(AOP_VERSION 1.0 CACHE STRING "AOP Version")
    add_subdirectory(AOP)
endif ()

if (ADD_LOGSYSTEM)
    set(LogSystem_VERSION 1.0 CACHE STRING "LogSystem Version")
    add_subdirectory(LogSystem)
    list(APPEND LIBRARY_LIST LogSystem)
endif ()

if (ADD_DISTRIBUTED)
    set(Distributed_VERSION 1.0 CACHE STRING "Distributed Version")
    add_subdirectory(Distributed)
    list(APPEND LIBRARY_LIST Distributed)
endif ()

# 导出CMake配置，设置NAMESPACE
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_BUILD_PREFIX}/${PROJECT_NAME}Version.cmake
        VERSION ${PROJECT_NAME}_VERSION
        COMPATIBILITY AnyNewerVersion
)

# 配置和安装包配置文件
configure_file(${CONFIG_PREFIX}/${PROJECT_NAME}Config.cmake.in
        ${CMAKE_BUILD_PREFIX}/${PROJECT_NAME}Config.cmake @ONLY)
install(FILES
        "${CMAKE_BUILD_PREFIX}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_BUILD_PREFIX}/${PROJECT_NAME}Version.cmake"
        DESTINATION ${INSTALL_CONFIG_DIR}
)
