# 各插件按字母顺序在此添加
add_subdirectory(core)
# add_subdirectory(editor)
# add_subdirectory(terminal)
# add_subdirectory(git)

# ==========================================
# Install rules for plugins
# ==========================================
# Core 是 host IDE 直接 link 的库 (通过 ICore / ActionManager / ModeManager / 适配器),
# 它的头文件 + lib 必须导出到 IDElib, demo 端才能 import_idelib(Core, coreplugin).
# 其它插件 (editor / terminal / git) 走动态加载, 只需要把 .dll 放到 bin/plugins/.
set(_plugin_lib_targets Core)
install(TARGETS ${_plugin_lib_targets}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 复制全部插件 DLL 到 bin/plugins/ (加载时被 PluginManager 扫描)
file(GLOB _plugin_dlls "${CMAKE_BINARY_DIR}/bin/plugins/*.dll")
install(FILES ${_plugin_dlls} DESTINATION bin/plugins)

# 导出 Core 头文件到 include/coreplugin
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/core/
    DESTINATION include/coreplugin
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
