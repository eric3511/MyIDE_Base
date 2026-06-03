# ============================================================================
# Core Plugin — MyIDE 的核心服务门面
# ============================================================================
# 提供 ICore / ActionManager / ModeManager / DocumentManager 等基础服务
# 详见 temp/coreplugin-extraction-roadmap.md
# ============================================================================

add_qtc_plugin(Core
  PUBLIC_DEPENDS
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Aggregation
    ExtensionSystem
    Utils
    app_version
  SOURCES
    # ---- IPlugin 入口与公共头 ----
    coreplugin.cpp coreplugin.h
    core_global.h

    # ---- PR-1 步骤2: 常量与 icontext ----
    coreconstants.h
    icontext.h icontext.cpp
    helpitem.h

    # ---- PR-1 步骤3+4: ICore (stub) + ActionManager ----
    icore.h icore.cpp
    actionmanager/actionmanager.h
    actionmanager/actionmanager_p.h
    actionmanager/actionmanager.cpp
    actionmanager/actioncontainer.h
    actionmanager/actioncontainer_p.h
    actionmanager/actioncontainer.cpp
    actionmanager/command.h
    actionmanager/command_p.h
    actionmanager/command.cpp
    actionmanager/commandbutton.h
    actionmanager/commandbutton.cpp

    # ---- PR-1 步骤5: IMode + ModeManager (UI-free 重写) ----
    imode.h imode.cpp
    modemanager.h modemanager.cpp

    # ---- PR-1 步骤7: 适配器层 (ModeManager ↔ host 主窗口) ----
    adapters/modebar_adapter.h
    adapters/modebar_adapter.cpp

    # ---- 后续步骤会逐步搬入: ----
    # PR-1 步骤2.5: coreicons.{h,cpp}, core.qrc (清理 Manhattan/Fancy 后再加)
    # PR-1 步骤3.5: commandsfile.{h,cpp} + commandmappings.{h,cpp} (需 shortcutsettings → PR-2)
    # PR-1 步骤7: adapters/modebar_adapter.{h,cpp} (IMode ↔ 左侧 toolbar)
)
