#include <QApplication>
#include <QMainWindow>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <advanceddockingsystem/DockManager.h>

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

    // 3. 从对象池获取 TestPlugin 创建的 DockManager
    auto *dockManager = PluginManager::getObject<ads::CDockManager>();
    QMainWindow *mainWindow = nullptr;

    if (dockManager) {
        mainWindow = new QMainWindow;
        mainWindow->setWindowTitle("MyIDE - TestPlugin Verification");
        mainWindow->resize(800, 600);
        mainWindow->setCentralWidget(dockManager);
        mainWindow->show();
    } else {
        qWarning() << "No CDockManager found in object pool!";
    }

    int ret = app.exec();

    // 4. 先清理 UI（从对象池中移除），再关闭插件
    delete mainWindow;
    PluginManager::shutdown();

    return ret;
}
