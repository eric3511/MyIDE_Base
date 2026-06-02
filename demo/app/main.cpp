#include <QApplication>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <advanceddockingsystem/DockManager.h>

#include "mywindow.h"

using namespace ExtensionSystem;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("MyIDE");
    app.setApplicationName("MyIDE");

    // 1. 设置插件搜索路径
    PluginManager pm;
    PluginManager::setPluginIID("org.qt-project.Qt.QtCreatorPlugin");
    QStringList pluginPaths;
    pluginPaths << qApp->applicationDirPath() + "/plugins";
    PluginManager::setPluginPaths(pluginPaths);

    // 2. 加载所有插件
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
    MyMainWindow mainWindow;

    // 4. 从对象池获取 TestPlugin 创建的 DockManager 作为中心部件
    auto *dockManager = PluginManager::getObject<ads::CDockManager>();
    if (dockManager) {
        mainWindow.setCentralWidget(dockManager);
    } else {
        qWarning() << "No CDockManager found in object pool!";
    }
    mainWindow.show();

    int ret = app.exec();

    // 5. 关闭插件；mainWindow / dockManager 在 main 返回时随栈 / Qt 对象树自动析构
    PluginManager::shutdown();

    return ret;
}
