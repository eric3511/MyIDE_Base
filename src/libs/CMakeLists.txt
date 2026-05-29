add_subdirectory(3rdparty)
add_subdirectory(aggregation)
add_subdirectory(utils)
add_subdirectory(extensionsystem)
add_subdirectory(advanceddockingsystem)

# 添加qwindowkit
include(FetchContent)
FetchContent_Declare(
    qwindowkit
    GIT_REPOSITORY https://github.com/stdware/qwindowkit.git
    GIT_TAG        1.5.0 # 或者 main
)
FetchContent_MakeAvailable(qwindowkit)

# ==========================================
# 配置 Install 规则
# ==========================================

# Internal project targets (no CMAKE_DEBUG_POSTFIX)
set(INTERNAL_TARGETS
    Utils
    ExtensionSystem
    Aggregation
    AdvancedDockingSystem
    KSyntaxHighlighting
    3rd_cplusplus
)

install(TARGETS ${INTERNAL_TARGETS}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# QWindowKit sets CMAKE_DEBUG_POSTFIX=d in its own scope, so Debug
# artifacts are QWKCored.dll / QWKCored.lib. install(TARGETS) does not
# reliably pick up the cross-scope debug postfix, so use explicit paths.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_qwk_dll_suffix "d")
else()
    set(_qwk_dll_suffix "")
endif()
foreach(_qwk_target QWKCore QWKWidgets)
    set(_qwk_dll "${_qwk_target}${_qwk_dll_suffix}.dll")
    set(_qwk_lib "${_qwk_target}${_qwk_dll_suffix}.lib")
    install(FILES "${CMAKE_BINARY_DIR}/bin/${_qwk_dll}"  DESTINATION bin)
    install(FILES "${CMAKE_BINARY_DIR}/lib/${_qwk_lib}"  DESTINATION lib)
endforeach()

# ==========================================
# 2. 导出头文件 (必须把新增库的头文件也导出去，否则别人没法 include)
# ==========================================

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/utils/
    DESTINATION include/utils
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extensionsystem/
    DESTINATION include/extensionsystem
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/aggregation/
    DESTINATION include/aggregation
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/advanceddockingsystem/
    DESTINATION include/advanceddockingsystem
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/
    DESTINATION include/3rdparty
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(DIRECTORY ${qwindowkit_SOURCE_DIR}/include/
    DESTINATION include/qwindowkit
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)
