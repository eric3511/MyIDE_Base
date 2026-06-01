 cmake_minimum_required(VERSION 3.16)

# ============================================================================
# IDE 基座 Demo 工程 — 演示如何基于 MyIDE 基座库构建应用与插件
# ============================================================================
# 构建全部 Demo:
#   set IDELIB_DIR=D:/IDElib/Debug
#   cmake -B build -DCMAKE_PREFIX_PATH="%IDELIB_DIR%;D:/Qt/6.x.x/msvc2019_64"
#   cmake --build build
#
# 也可以单独构建某个子目录:
#   cmake -B build ... && cmake --build build --target MyIDE
#   cmake -B build ... && cmake --build build --target TestPlugin
# ============================================================================

project(MyIDEDemo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# MSVC: 确保能找到 Windows SDK 库 (mpr.lib 等)
# 从注册表或环境变量探测 Windows SDK 版本
# ============================================================================
if(MSVC)
    # 尝试从环境获取 SDK 版本
    if(DEFINED ENV{WindowsSDKVersion})
        set(_sdk_version "$ENV{WindowsSDKVersion}")
        string(REGEX REPLACE "[\\]" "" _sdk_version "${_sdk_version}") # 去掉尾部反斜杠
        set(_sdk_lib_dir "C:/Program Files (x86)/Windows Kits/10/Lib/${_sdk_version}/um/x64")
        if(EXISTS "${_sdk_lib_dir}")
            link_directories("${_sdk_lib_dir}")
            message(STATUS "Windows SDK: ${_sdk_lib_dir}")
        endif()
    endif()
endif()

# 向下传递 IDELIB_DIR
if(NOT IDELIB_DIR)
    if(DEFINED ENV{IDELIB_DIR})
        set(IDELIB_DIR "$ENV{IDELIB_DIR}" CACHE PATH "IDElib 安装目录 (Debug 或 Release)")
    else()
        foreach(_p IN LISTS CMAKE_PREFIX_PATH)
            if(EXISTS "${_p}/include/extensionsystem")
                set(IDELIB_DIR "${_p}" CACHE PATH "IDElib 安装目录 (Debug 或 Release)")
                break()
            endif()
        endforeach()
    endif()
endif()

if(NOT IDELIB_DIR)
    message(FATAL_ERROR
        "请设置 IDELIB_DIR 或 CMAKE_PREFIX_PATH 指向 IDElib 目录 (如 D:/IDElib/Debug)")
endif()

add_subdirectory(app)
add_subdirectory(testplugin)
