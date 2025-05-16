project(Base CXX)

set(CMAKE_CXX_STANDARD 17)

# 收集源文件和头文件
FILE(GLOB_RECURSE SRC_LIST "*.cpp")
FILE(GLOB_RECURSE HEAD_LIST "*.hpp")

# 创建静态库
add_library(${PROJECT_NAME} STATIC ${SRC_LIST} ${HEAD_LIST})

# 设置包含目录
target_include_directories(${PROJECT_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        PUBLIC
        $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
)

# 安装头文件，保持目录结构
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
        DESTINATION ${INSTALL_INCLUDE_DIR}/${PROJECT_NAME}
        FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "CMakeLists.txt" EXCLUDE)

# 安装目标，设置NAMESPACE为TinyBackend::${PROJECT_NAME}
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
        INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
)

# 导出CMake配置，设置NAMESPACE
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${PROJECT_NAME}Version.cmake
        VERSION ${PROJECT_NAME}_VERSION
        COMPATIBILITY AnyNewerVersion
)

install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE TinyBackend::${PROJECT_NAME}::  # 设置嵌套命名空间
        DESTINATION ${INSTALL_CONFIG_DIR}
)

# 配置和安装包配置文件
configure_file(${PROJECT_NAME}Config.cmake.in ${PROJECT_NAME}Config.cmake @ONLY)
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${INSTALL_CONFIG_DIR}
)