#include "coreplugin.h"
#include "icore.h"
#include "modemanager.h"
#include "actionmanager/actionmanager.h"

#include <QDebug>

namespace Core {
namespace Internal {

CorePlugin::CorePlugin() = default;

CorePlugin::~CorePlugin() = default;

bool CorePlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // Construct the ICore singleton (replaces old Internal::MainWindow* argument
    // until step 4 wires the real backend).
    if (!ICore::instance())
        m_icore = new ICore;

    // Construct the ActionManager singleton so plugins can registerActions()
    // from their initialize() handlers. Menu/menubar containers are created on
    // demand by plugins (we don't auto-populate until step 6 wires mywindow).
    if (!ActionManager::instance())
        m_actionManager = new ActionManager;

    // ModeManager singleton — IMode constructors call ModeManager::addMode()
    // so this must exist before any plugin's initialize() runs.
    if (!ModeManager::instance())
        m_modeManager = new ModeManager;

    qDebug() << "[CorePlugin] initialize: ICore + ActionManager + ModeManager ready.";
    return true;
}

void CorePlugin::extensionsInitialized()
{
    // All plugins have registered their modes/actions; let ModeManager
    // sort and emit modesReordered so the adapter can build the mode bar.
    ModeManager::extensionsInitialized();
    qDebug() << "[CorePlugin] extensionsInitialized.";
}

ExtensionSystem::IPlugin::ShutdownFlag CorePlugin::aboutToShutdown()
{
    delete m_modeManager;
    m_modeManager = nullptr;
    delete m_actionManager;
    m_actionManager = nullptr;
    delete m_icore;
    m_icore = nullptr;
    return SynchronousShutdown;
}

} // namespace Internal
} // namespace Core
