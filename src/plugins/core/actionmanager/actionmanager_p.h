// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Extracted from Qt Creator (coreplugin/actionmanager/actionmanager_p.h).

#pragma once

#include "../icontext.h"

#include <QMap>
#include <QHash>
#include <QMultiHash>
#include <QTimer>

namespace Core {

class Command;

namespace Internal {

class ActionContainerPrivate;

class ActionManagerPrivate : public QObject
{
    Q_OBJECT

public:
    using IdCmdMap = QHash<Utils::Id, Command *>;
    using IdContainerMap = QHash<Utils::Id, ActionContainerPrivate *>;

    ~ActionManagerPrivate() override;

    void setContext(const Context &context);
    bool hasContext(int context) const;

    void saveSettings();
    static void saveSettings(Command *cmd);

    static void showShortcutPopup(const QString &shortcut);
    bool hasContext(const Context &context) const;
    Command *overridableAction(Utils::Id id);

    static void readUserSettings(Utils::Id id, Command *cmd);

    void containerDestroyed();
    void actionTriggered();

    IdCmdMap m_idCmdMap;

    IdContainerMap m_idContainerMap;

    Context m_context;

    bool m_presentationModeEnabled = false;
};

} // namespace Internal
} // namespace Core
