add_library(app_version INTERFACE)
target_include_directories(app_version
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

#创建一个名为 app_version 的接口库。
#这个库不生成实际二进制文件。
#它携带两个 include 路径：
#当前构建目录
#当前源码目录
#这些路径只在当前项目构建阶段有效。
#谁链接了 app_version，谁就能使用这些 include 路径。

add_subdirectory(libs)
add_subdirectory(plugins)
add_subdirectory(app)
#add_subdirectory(tools)


if (NOT IDE_VERSION)
  include(\${CMAKE_CURRENT_LIST_DIR}/QtCreatorIDEBranding.cmake)
endif()