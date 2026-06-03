#pragma once

#include "core_global.h"
#include <extensionsystem/iplugin.h>

namespace Core {

class ICore;
class ActionManager;
class ModeManager;

namespace Internal {

class CorePlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Core.json")

public:
    CorePlugin();
    ~CorePlugin() override;

    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    ICore *m_icore = nullptr;
    ActionManager *m_actionManager = nullptr;
    ModeManager *m_modeManager = nullptr;
};

} // namespace Internal
} // namespace Core
