project(LogSystem CXX)

set(CMAKE_CXX_STANDARD 17)

# 收集源文件和头文件
FILE(GLOB_RECURSE SRC_LIST "*.cpp")
FILE(GLOB_RECURSE HEAD_LIST "*.hpp")

# 创建静态库
add_library(${PROJECT_NAME} STATIC ${SRC_LIST} ${HEAD_LIST})

target_link_libraries(${PROJECT_NAME} PUBLIC Base PUBLIC Net)

# 设置包含目录
target_include_directories(${PROJECT_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${PROJECT_ROOT_DIR}>
        PUBLIC
        $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}"
)

# 安装头文件，保持目录结构
install(DIRECTORY ${PROJECT_ROOT_DIR}/${PROJECT_NAME}/
        DESTINATION ${INSTALL_INCLUDE_DIR}/${PROJECT_NAME}
        OPTIONAL
        FILES_MATCHING
        PATTERN "*.hpp"
        REGEX "/src/" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE)

# 安装目标，设置NAMESPACE为TinyBackend::${PROJECT_NAME}
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
        INCLUDES DESTINATION ${INSTALL_INCLUDE_DIR}
)

# 导出CMake配置，设置NAMESPACE
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_BUILD_PREFIX}/${PROJECT_NAME}/${PROJECT_NAME}Version.cmake
        VERSION ${PROJECT_NAME}_VERSION
        COMPATIBILITY AnyNewerVersion
)

install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_ROOT_NAME}::
        DESTINATION ${INSTALL_CONFIG_DIR}/${PROJECT_NAME}
)

# 配置和安装包配置文件
configure_file(${CONFIG_PREFIX}/${PROJECT_NAME}Config.cmake.in
        ${CMAKE_BUILD_PREFIX}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake @ONLY)
install(FILES
        "${CMAKE_BUILD_PREFIX}/${PROJECT_NAME}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_BUILD_PREFIX}/${PROJECT_NAME}/${PROJECT_NAME}Version.cmake"
        DESTINATION ${INSTALL_CONFIG_DIR}/${PROJECT_NAME}
)