#项目名
project(testing)

# 项目源文件和头文件列表（考虑到 IDE 的分析功能，故加入头文件）
set(src_list)

# 获取当前目录的绝对路径
get_filename_component(current_dir ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

# 使用 file() 命令和 GLOB 选项获取当前目录下的所有目录
file(GLOB subObjects RELATIVE ${current_dir} "${current_dir}/*")

# 遍历所有目录，如果当前被遍历目录下的 src 目录存在，才会将当前被遍历的子目录下的头文件和 src 目录下的源文件添加到 src_list
foreach (sub IN LISTS subObjects)

    set(sub_path ${current_dir}/${sub})

    if (IS_DIRECTORY ${sub_path} AND EXISTS "${sub_path}/src")

        file(GLOB files RELATIVE ${current_dir} "${sub_path}/src/*")

        foreach (file IN LISTS files)

            get_filename_component(extension ${file} EXT)

            if (extension STREQUAL ".cpp")

                list(APPEND src_list ${file})

            endif ()

        endforeach ()

        file(GLOB files RELATIVE ${current_dir} "${sub_path}/*")

        foreach (file IN LISTS files)

            get_filename_component(extension ${file} EXT)

            if (extension STREQUAL ".hpp")

                list(APPEND src_list ${file})

            endif ()

        endforeach ()

    endif ()

endforeach ()

# 如果当前模块下的 src 目录存在，将 src 目录下的源文件和当前目录下的头文件添加到 src_list
if (EXISTS "${current_dir}/src")

    file(GLOB files RELATIVE ${current_dir} "${current_dir}/src/*")

    foreach (file IN LISTS files)

        get_filename_component(extension ${file} EXT)

        if (extension STREQUAL ".cpp")

            list(APPEND src_list ${file})

        endif ()

    endforeach ()

    foreach (file IN LISTS subObjects)

        if (IS_DIRECTORY file)

            CONTINUE()

        endif ()

        get_filename_component(extension ${file} EXT)

        if (extension STREQUAL ".hpp")

            list(APPEND src_list ${file})

        endif ()

    endforeach ()

endif ()


# 创建 library
add_library(${PROJECT_NAME} ${src_list})

# 指定当前模块的头文件搜索路径
target_include_directories(${PROJECT_NAME} PUBLIC "${current_dir}/..")

# 链接 Base 库
target_link_libraries(${PROJECT_NAME} PRIVATE Base PRIVATE Net)
