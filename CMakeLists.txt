cmake_minimum_required(VERSION 3.26)

# 项目名
project(tinyBackend)

# C++ 语言标准
set(CMAKE_CXX_STANDARD 17)

# 设置编译器优化级别
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -g")

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Project root directory")

set(CMAKE_INSTALL_PREFIX "/home/taganyer/Code/Clion_project/tinyBackend/install" CACHE PATH "Install prefix" FORCE)

#if (NOT DEFINED CMAKE_INSTALL_PREFIX)
#    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install prefix" FORCE)
#endif ()

if (NOT DEFINED LOG_PATH)
    set(LOG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/global_logs CACHE PATH "Project log directory" FORCE)
endif ()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/CMake_config.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/CMake_config.h")

# 设置安装路径
set(INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME} CACHE PATH "Project includes directory" FORCE)
set(INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME} CACHE PATH "Project library directory" FORCE)
set(INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME} CACHE PATH "Project run time directory" FORCE)
set(INSTALL_CONFIG_DIR ${CMAKE_INSTALL_PREFIX}/cmake/${PROJECT_NAME} CACHE PATH "Project cmake config directory" FORCE)

set(ADD_TESTING OFF CACHE BOOL "Enable testing components" FORCE)
set(ADD_AOP ON CACHE BOOL "Enable AOP components" FORCE)
set(ADD_DISTRIBUTED ON CACHE BOOL "Enable distributed components" FORCE)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.12)
    cmake_parse_arguments(
            ARG "" "" "ADD_TESTING;ADD_AOP;ADD_DISTRIBUTED" ${CMAKE_ARGV}
    )
    if (ARG_ADD_TESTING)
        set(ADD_TESTING ${ARG_ADD_TESTING} FORCE)
    endif ()
    if (ARG_ADD_AOP)
        set(ADD_AOP ${ARG_ADD_AOP} FORCE)
    endif ()
    if (ARG_ADD_DISTRIBUTED)
        set(ADD_DISTRIBUTED ${ARG_ADD_DISTRIBUTED} FORCE)
    endif ()
endif ()

set(Base_VERSION 1.0 CACHE STRING "Base Version")
add_subdirectory(Base)

set(LogSystem_VERSION 1.0 CACHE STRING "LogSystem Version")
add_subdirectory(LogSystem)

set(Net_VERSION 1.0 CACHE STRING "Net Version")
add_subdirectory(Net)

if (ADD_AOP)
    add_subdirectory(AOP)
endif ()

if (ADD_DISTRIBUTED)
    add_subdirectory(Distributed)
endif ()

if (ADD_TESTING)
    add_subdirectory(testing)
endif ()
