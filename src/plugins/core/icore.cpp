// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// ICore implementation — PR-1 step 4 will wire this up to demo/app/mywindow.cpp.
// For now this is a stub that provides just enough for ActionManager to compile:
//   - ICore::instance()            → singleton accessor
//   - ICore::mainWindow()          → set by CorePlugin::initialize() via host window
//   - ICore::settings()            → real QSettings (user scope)
//   - ICore::versionString()       → from <app/app_version.h>
//   - ICore::ideDisplayName()      → static string
// The rest throw qWarning + return defaults so callers don't crash before step 4.

#include "icore.h"

#include "adapters/modebar_adapter.h"
#include "modemanager.h"

// app/app_version.h doesn't exist yet in this base — IDE_VERSION is only a
// CMake variable. We hard-code the version here; in step 4 we'll generate a
// proper app_version.h from cmake/QtCreatorIDEBranding.cmake.
#define MYIDE_VERSION_STR       "1.0.0"
#define MYIDE_VERSION_COMPAT_STR "1.0.0"
#define MYIDE_DISPLAY_NAME       "MyIDEBase"

#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QApplication>
#include <QDebug>
#include <QLibraryInfo>
#include <QMainWindow>
#include <QMenuBar>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>
#include <QSysInfo>

namespace Core {

namespace Internal {
class MainWindow;
}

static ICore *m_instance = nullptr;
static QMainWindow *m_registeredMainWindow = nullptr;
static QWidget *m_registeredHostWidget = nullptr;
static QMenuBar *m_registeredMenuBar = nullptr;
static QStatusBar *m_registeredStatusBar = nullptr;
static QWidget *m_registeredModeBarContainer = nullptr;
static QWidget *m_registeredCentralContainer = nullptr;
static Internal::ModeBarAdapter *m_modeBarAdapter = nullptr;

// Forward-declared helper: must live in namespace Core so it can see the
// module-scope statics above. Defined below after Core::Internal helpers.
static void maybeCreateModeBarAdapter();

ICore::ICore(Internal::MainWindow *mw) : QObject(nullptr)
{
    Q_UNUSED(mw); // PR-1 step 4 will properly thread this through MainWindow
    m_instance = this;
}

ICore::~ICore()
{
    m_instance = nullptr;
}

ICore *ICore::instance()
{
    return m_instance;
}

void ICore::setMainWindow(Internal::MainWindow *mw)
{
    Q_UNUSED(mw);
    // Internal::MainWindow is a forward-declared host class; in step 4 we
    // will require the host to provide a real QMainWindow via the new
    // setMainWindow(QMainWindow*) overload instead.
}

void ICore::setMainWindow(QMainWindow *mw)
{
    m_registeredMainWindow = mw;
    m_registeredHostWidget = mw;
}

void ICore::setMainWindow(QWidget *w)
{
    m_registeredHostWidget = w;
    // If the host happens to be a QMainWindow, register that too so plugins
    // that need real QMainWindow APIs can still work.
    m_registeredMainWindow = qobject_cast<QMainWindow *>(w);
}

void ICore::setMenuBar(QMenuBar *mb)
{
    m_registeredMenuBar = mb;
}

QMenuBar *ICore::menuBar()
{
    if (m_registeredMenuBar)
        return m_registeredMenuBar;
    if (m_registeredMainWindow)
        return m_registeredMainWindow->menuBar();
    return nullptr;
}

void ICore::setStatusBar(QStatusBar *sb)
{
    m_registeredStatusBar = sb;
}

void ICore::setModeBarContainer(QWidget *w)
{
    m_registeredModeBarContainer = w;
    maybeCreateModeBarAdapter();
}

QWidget *ICore::modeBarContainer()
{
    return m_registeredModeBarContainer;
}

void ICore::setCentralContainer(QWidget *w)
{
    m_registeredCentralContainer = w;
    maybeCreateModeBarAdapter();
}

QWidget *ICore::centralContainer()
{
    return m_registeredCentralContainer;
}

QMainWindow *ICore::mainWindow()
{
    if (m_registeredMainWindow)
        return m_registeredMainWindow;
    // Fall back to first QMainWindow in QApplication's top-level widgets.
    const QWidgetList top = QApplication::topLevelWidgets();
    for (QWidget *w : top) {
        if (auto *mw = qobject_cast<QMainWindow *>(w))
            return mw;
    }
    return nullptr;
}

QWidget *ICore::dialogParent()
{
    // Prefer the explicitly-registered host widget, then any visible top-level.
    if (m_registeredHostWidget)
        return m_registeredHostWidget;
    if (m_registeredMainWindow)
        return m_registeredMainWindow;
    const QWidgetList top = QApplication::topLevelWidgets();
    for (QWidget *w : top) {
        if (w && w->isVisible())
            return w;
    }
    return QApplication::activeWindow();
}

QStatusBar *ICore::statusBar()
{
    if (m_registeredStatusBar)
        return m_registeredStatusBar;
    if (m_registeredMainWindow)
        return m_registeredMainWindow->statusBar();
    return nullptr;
}

Utils::QtcSettings *ICore::settings(QSettings::Scope scope)
{
    // Process-scope singleton QSettings; replaced with proper backend in step 4.
    static Utils::QtcSettings *userSettings = nullptr;
    if (!userSettings) {
        QSettings::Format fmt = QSettings::IniFormat;
        userSettings = new Utils::QtcSettings(fmt, QSettings::UserScope,
                                               QCoreApplication::organizationName(),
                                               QCoreApplication::applicationName());
    }
    Q_UNUSED(scope);
    return userSettings;
}

QString ICore::versionString()
{
    return QString::fromLatin1(MYIDE_VERSION_STR);
}

QString ICore::ideDisplayName()
{
    return QString::fromLatin1(MYIDE_DISPLAY_NAME);
}

QString ICore::buildCompatibilityString()
{
    return QString::fromLatin1(MYIDE_VERSION_COMPAT_STR);
}

// ----- stubs below: real implementations come in PR-1 step 4 -----

bool ICore::isNewItemDialogRunning() { return false; }
QWidget *ICore::newItemDialog() { return nullptr; }
void ICore::showNewItemDialog(const QString &, const QList<IWizardFactory *> &,
                              const Utils::FilePath &, const QVariantMap &) {}
bool ICore::showOptionsDialog(const Utils::Id, QWidget *) { return false; }
QString ICore::msgShowOptionsDialog() { return {}; }
QString ICore::msgShowOptionsDialogToolTip() { return {}; }
bool ICore::showWarningWithOptions(const QString &, const QString &, const QString &,
                                   Utils::Id, QWidget *) { return false; }
SettingsDatabase *ICore::settingsDatabase() { return nullptr; }
QPrinter *ICore::printer() { return nullptr; }
QString ICore::userInterfaceLanguage() { return {}; }
Utils::FilePath ICore::resourcePath(const QString &) { return {}; }
Utils::FilePath ICore::userResourcePath(const QString &) { return {}; }
Utils::FilePath ICore::cacheResourcePath(const QString &) { return {}; }
Utils::FilePath ICore::installerResourcePath(const QString &) { return {}; }
Utils::FilePath ICore::libexecPath(const QString &) { return {}; }
Utils::FilePath ICore::crashReportsPath() { return {}; }
Utils::InfoBar *ICore::infoBar() { return nullptr; }
void ICore::raiseWindow(QWidget *) {}
IContext *ICore::currentContextObject() { return nullptr; }
QWidget *ICore::currentContextWidget() { return nullptr; }
IContext *ICore::contextObject(QWidget *) { return nullptr; }
void ICore::updateAdditionalContexts(const Context &, const Context &, ContextPriority) {}
void ICore::addAdditionalContext(const Context &, ContextPriority) {}
void ICore::removeAdditionalContext(const Context &) {}
void ICore::addContextObject(IContext *) {}
void ICore::removeContextObject(IContext *) {}
void ICore::registerWindow(QWidget *, const Context &) {}
void ICore::openFiles(const Utils::FilePaths &, OpenFilesFlags) {}
void ICore::addPreCloseListener(const std::function<bool()> &) {}
void ICore::restart() {}
void ICore::saveSettings(SaveSettingsReason) {}
void ICore::setNewDialogFactory(const std::function<NewDialog *(QWidget *)> &) {}
QStringList ICore::additionalAboutInformation() { return {}; }
void ICore::appendAboutInformation(const QString &) {}
QString ICore::systemInformation() { return QSysInfo::prettyProductName(); }
void ICore::setupScreenShooter(const QString &, QWidget *, const QRect &) {}
QString ICore::pluginPath() { return {}; }
QString ICore::userPluginPath() { return {}; }
Utils::FilePath ICore::clangExecutable(const Utils::FilePath &) { return {}; }
Utils::FilePath ICore::clangdExecutable(const Utils::FilePath &) { return {}; }
Utils::FilePath ICore::clangTidyExecutable(const Utils::FilePath &) { return {}; }
Utils::FilePath ICore::clazyStandaloneExecutable(const Utils::FilePath &) { return {}; }
Utils::FilePath ICore::clangIncludeDirectory(const QString &, const Utils::FilePath &) { return {}; }
void ICore::updateNewItemDialogState() {}

} // namespace Core

// ----- Adapter bootstrap (forward decl used by set* accessors above) ---------
namespace Core {
namespace Internal {
static void createModeBarAdapter(QWidget *modeBar, QWidget *central)
{
    if (m_modeBarAdapter)
        return;
    m_modeBarAdapter = new ModeBarAdapter(modeBar, central);
    m_modeBarAdapter->rebuild();
}
} // namespace Internal

static void maybeCreateModeBarAdapter()
{
    if (!m_registeredModeBarContainer || !m_registeredCentralContainer)
        return;
    Core::Internal::createModeBarAdapter(m_registeredModeBarContainer,
                                          m_registeredCentralContainer);
}
} // namespace Core
