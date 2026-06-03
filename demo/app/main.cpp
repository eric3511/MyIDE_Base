#include <QApplication>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <advanceddockingsystem/DockManager.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
// NOTE: ModeBarAdapter is now constructed internally by ICore when both
// setModeBarContainer() and setCentralContainer() have been called. The
// host no longer needs to (and must not) construct it directly. This keeps
// Core.dll loaded exactly once: PluginManager loads it from app/plugins/,
// and the host only talks to ICore through Core.dll's public headers.

#include "mywindow.h"

using namespace ExtensionSystem;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("MyIDE");
    app.setApplicationName("MyIDE");

    // 1. 设置插件搜索路径 — 统一目录布局: 所有 DLL (Core, TestPlugin, 其它
    //    共享库) 都在 applicationDirPath() 下, PluginManager 通过 .dll 扩展名
    //    + Q_PLUGIN_METADATA 区分真正的插件, 共享库被静默跳过.
    PluginManager pm;
    PluginManager::setPluginIID("org.qt-project.Qt.QtCreatorPlugin");
    QStringList pluginPaths;
    pluginPaths << qApp->applicationDirPath();
    PluginManager::setPluginPaths(pluginPaths);

    // 2. 加载所有插件 (Core, TestPlugin). CorePlugin::initialize 已经创建
    //    了 ICore / ActionManager / ModeManager; extensionsInitialized 在
    //    loadPlugins 内部按序调用, 此时所有 IMode 都已注册, ModeManager 也
    //    已发出过 modesReordered (我们稍后会构造 ModeBarAdapter 重放快照).
    PluginManager::loadPlugins();

    if (PluginManager::hasError()) {
        qCritical() << "Plugin errors:" << PluginManager::allErrors();
        return 1;
    }

    qDebug() << "Loaded plugins:";
    for (PluginSpec *spec : PluginManager::plugins())
        qDebug() << "  " << spec->name() << spec->version();

    // 3. 创建主窗口 (MyMainWindow 内部已用 QWK::WidgetWindowAgent 完成
    //    frameless / 自定义标题栏 / 系统按钮 / hit-test / 居中 / 主题 等集成)
    qDebug() << "[main] step 3: creating MyMainWindow";
    MyMainWindow mainWindow;
    qDebug() << "[main] step 3 done. menuBar=" << mainWindow.menuBar();

    // 4. 注册主窗口契约到 ICore. ActionManager / 适配器 / 后续插件都通过
    //    ICore 的访问器拿到菜单/状态/对话框宿主.
    qDebug() << "[main] step 4: ICore::instance()=" << Core::ICore::instance();
    if (Core::ICore::instance()) {
        Core::ICore::setMainWindow(&mainWindow);
        Core::ICore::setMenuBar(mainWindow.menuBar());
        Core::ICore::setStatusBar(mainWindow.statusBar());
        Core::ICore::setCentralContainer(mainWindow.centralContainer());
        Core::ICore::setModeBarContainer(mainWindow.modeBarContainer());

        // 把 ActionManager 提供的 MENU_BAR 容器嵌入到主窗口的标题栏
        // (替换 setupUi() 里临时构造的占位 QMenuBar).
        Core::ActionContainer *menuBarContainer
            = Core::ActionManager::createMenuBar(Core::Constants::MENU_BAR);
        if (menuBarContainer) {
            mainWindow.replaceMenuBar(menuBarContainer->menuBar());
            Core::ICore::setMenuBar(mainWindow.menuBar());
        }
    } else {
        qCritical() << "Core::ICore not initialized — Core plugin failed to load.";
        return 2;
    }
    qDebug() << "[main] step 4 done.";

    // 5. ModeBarAdapter 桥接 ModeManager 信号 ↔ mywindow 的 mode bar / 中心容器.
    //    ICore::setModeBarContainer() / setCentralContainer() 内部已经按序触发
    //    ModeBarAdapter::rebuild(),这里不再需要(也不能)直接构造它.
    qDebug() << "[main] step 5: ModeBarAdapter constructed by ICore.";

    // 6. 从对象池获取 TestPlugin 创建的 DockManager. PR-1 暂不把 dockManager
    //    放到主窗口中心 — ModeBarAdapter 已经在 centralContainer 里放了
    //    各个 IMode 的 page, 共享同一个布局会让 setCentralWidget 把它
    //    撕掉. PR-3 EditorManager 改造时会重新协调 ADS ↔ modes.
    //    这里只验证 dockManager 存在并已被加载, 跳过挂载.
    auto *dockManager = PluginManager::getObject<ads::CDockManager>();
    if (!dockManager)
        qWarning() << "No CDockManager found in object pool!";

    mainWindow.show();
    qDebug() << "[main] window shown, entering exec";

    int ret = app.exec();
    qDebug() << "[main] exec returned" << ret;

    // 7. 关闭插件；mainWindow / dockManager / adapter 在 main 返回时随
    //    栈 / Qt 对象树自动析构
    PluginManager::shutdown();

    return ret;
}
