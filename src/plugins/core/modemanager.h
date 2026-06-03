// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Extracted from Qt Creator coreplugin/modemanager.h — public API preserved.
// The original implementation was tied to Internal::MainWindow + FancyTabWidget +
// FancyActionBar. This rewrite is a pure mode registry: it tracks IMode objects,
// remembers the active one, and emits signals. A UI adapter (ModeBarAdapter in
// core/adapters/) listens to these signals and wires modes into the host's
// mode-bar container + central widget.

#pragma once

#include "core_global.h"

#include <utils/id.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {

class IMode;

class CORE_EXPORT ModeManager : public QObject
{
    Q_OBJECT

public:
    enum class Style {
        IconsAndText,
        IconsOnly,
        Hidden
    };

    // Construct via CorePlugin::initialize(); single instance.
    ModeManager();
    ~ModeManager() override;

    static ModeManager *instance();

    static IMode *currentMode();
    static Utils::Id currentModeId();

    // Auxiliary actions shown on the mode bar (e.g. project selector, run).
    // Adapter consumes these via modeBarActions() / projectSelector().
    static void addAction(QAction *action, int priority);
    static void addProjectSelector(QAction *action);

    static void activateMode(Utils::Id id);
    static void setFocusToCurrentMode();
    static Style modeStyle();

    static void removeMode(IMode *mode);

    // Adapter-facing accessors (no UI dependency, just expose model state).
    static QList<IMode *> modes();
    static QList<QAction *> modeBarActions();
    static QAction *projectSelectorAction();

    // Called by CorePlugin::extensionsInitialized() once all plugins have
    // registered their modes. Sorts by priority and emits modesReordered.
    static void extensionsInitialized();

public slots:
    static void setModeStyle(Style layout);
    static void cycleModeStyle();

signals:
    // Fired right before currentMode changes.
    void currentModeAboutToChange(Utils::Id mode);

    // Fired after currentMode has changed. The default oldMode argument
    // keeps existing single-arg connect() calls compatible.
    void currentModeChanged(Utils::Id mode, Utils::Id oldMode = {});

    // Adapter signals — let the UI rebuild without coupling to it.
    void modeRegistered(IMode *mode);
    void modeUnregistered(IMode *mode);
    void modeEnabledStateChanged(IMode *mode, bool enabled);
    void modesReordered();
    void modeBarActionAdded(QAction *action, int priority);
    void projectSelectorChanged(QAction *action);
    void modeStyleChanged(Style style);

private:
    static void addMode(IMode *mode);

    friend class IMode;
};

} // namespace Core
