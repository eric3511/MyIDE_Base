add_qtc_plugin(TestPlugin
  DEPENDS
    ExtensionSystem
    Utils
    Aggregation
    AdvancedDockingSystem
    KSyntaxHighlighting
    3rd_cplusplus
    Qt6::Widgets
  SOURCES
    testplugin.cpp testplugin.h
    testplugin.json.in
)
