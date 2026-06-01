cmake_minimum_required(VERSION 3.16)

# ============================================================================
# MyIDE Demo App — IDE 开发者示例：如何加载基座库构建自己的 IDE 可执行文件
# ============================================================================
# 使用方式:
#   set IDELIB_DIR=D:/IDElib/Debug   (或 Release)
#   cmake -B build -DCMAKE_PREFIX_PATH="%IDELIB_DIR%;D:/Qt/6.x.x/msvc2019_64"
#   cmake --build build
#
# 也可以直接传 IDELIB_DIR:
#   cmake -B build -DIDELIB_DIR="D:/IDElib/Debug" -DCMAKE_PREFIX_PATH="D:/Qt/6.x.x/msvc2019_64"
# ============================================================================

project(MyIDEApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

# ============================================================================
# 1. 定位 IDElib
# ============================================================================
if(NOT IDELIB_DIR)
    if(DEFINED ENV{IDELIB_DIR})
        set(IDELIB_DIR "$ENV{IDELIB_DIR}")
    else()
        foreach(_p IN LISTS CMAKE_PREFIX_PATH)
            if(EXISTS "${_p}/include/extensionsystem")
                set(IDELIB_DIR "${_p}")
                break()
            endif()
        endforeach()
    endif()
endif()

if(NOT IDELIB_DIR)
    message(FATAL_ERROR
        "请设置 IDELIB_DIR 或 CMAKE_PREFIX_PATH 指向 IDElib 目录 (如 D:/IDElib/Debug)")
endif()

message(STATUS "IDELIB_DIR = ${IDELIB_DIR}")

set(IDELIB_INCLUDE_DIR "${IDELIB_DIR}/include")
set(IDELIB_LIB_DIR     "${IDELIB_DIR}/lib")
set(IDELIB_BIN_DIR     "${IDELIB_DIR}/bin")

# ============================================================================
# 2. 查找 Qt 6
# ============================================================================
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# ============================================================================
# 3. 导入预编译 IDE 库
#    import_idelib(name [include_subdir])
#    - 不传 subdir: INTERFACE_INCLUDE_DIRECTORIES = IDElib/include
#    - 传 subdir:   INTERFACE_INCLUDE_DIRECTORIES = IDElib/include/<subdir>
# ============================================================================
macro(import_idelib name)
    if(${ARGC} GREATER 1)
        set(_inc_dir "${IDELIB_INCLUDE_DIR}/${ARGN}")
    else()
        set(_inc_dir "${IDELIB_INCLUDE_DIR}")
    endif()
    add_library(${name} SHARED IMPORTED)
    set_target_properties(${name} PROPERTIES
        IMPORTED_LOCATION               "${IDELIB_BIN_DIR}/${name}.dll"
        IMPORTED_IMPLIB                 "${IDELIB_LIB_DIR}/${name}.lib"
        INTERFACE_INCLUDE_DIRECTORIES   "${_inc_dir}"
    )
endmacro()

import_idelib(ExtensionSystem)
import_idelib(Utils)
import_idelib(Aggregation)
import_idelib(AdvancedDockingSystem)

# ============================================================================
# 4. 构建 MyIDE 可执行文件
# ============================================================================
add_executable(MyIDE
    main.cpp
)

target_link_libraries(MyIDE PRIVATE
    ExtensionSystem
    Utils
    Aggregation
    AdvancedDockingSystem
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# ============================================================================
# 5. 拷贝运行时 DLL 到输出目录
# ============================================================================
set(_runtime_dlls
    ExtensionSystem.dll
    Utils.dll
    Aggregation.dll
    AdvancedDockingSystem.dll
)

foreach(_dll IN LISTS _runtime_dlls)
    add_custom_command(TARGET MyIDE POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${IDELIB_BIN_DIR}/${_dll}"
            "$<TARGET_FILE_DIR:MyIDE>"
    )
endforeach()

# 如果 TestPlugin 也在 demo 工程中构建，将其 DLL 复制到 app 的 plugins/ 目录
# (使用顶层 demo/CMakeLists.txt 一起构建时，TestPlugin target 会先生成)
if(TARGET TestPlugin)
    add_custom_command(TARGET MyIDE POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:MyIDE>/plugins"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:TestPlugin>"
            "$<TARGET_FILE_DIR:MyIDE>/plugins/"
    )
endif()
