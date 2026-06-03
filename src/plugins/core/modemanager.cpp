// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// ModeManager rewrite — strips out FancyTabWidget/FancyActionBar/MainWindow
// coupling from the original Qt Creator implementation. Keeps the public
// contract intact so plugins and the IMode constructor still work unchanged.

#include "modemanager.h"

#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "coreconstants.h"
#include "icore.h"
#include "imode.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QDebug>
#include <QKeySequence>

using namespace Utils;

namespace Core {

struct ModeBarActionEntry {
    QAction *action;
    int priority;
};

struct ModeManagerPrivate
{
    QVector<IMode *> m_modes;
    QVector<Command *> m_modeCommands;
    QVector<ModeBarActionEntry> m_actions;
    QAction *m_projectSelector = nullptr;
    Context m_addedContexts;
    IMode *m_currentMode = nullptr;
    ModeManager::Style m_modeStyle = ModeManager::Style::IconsAndText;

    bool m_startingUp = true;
    Id m_pendingFirstActiveMode; // Valid before extensionsInitialized.

    void registerModeCommand(IMode *mode, int index);
    void activateModeHelper(Id id);
};

static ModeManagerPrivate *d = nullptr;
static ModeManager *m_instance = nullptr;

static int indexOf(Id id)
{
    if (!d)
        return -1;
    for (int i = 0; i < d->m_modes.count(); ++i) {
        if (d->m_modes.at(i)->id() == id)
            return i;
    }
    qDebug() << "ModeManager: no such mode" << id.toString();
    return -1;
}

static IMode *findMode(Id id)
{
    const int index = indexOf(id);
    return (index >= 0) ? d->m_modes.at(index) : nullptr;
}

ModeManager::ModeManager()
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;
    d = new ModeManagerPrivate;
}

ModeManager::~ModeManager()
{
    delete d;
    d = nullptr;
    m_instance = nullptr;
}

ModeManager *ModeManager::instance()
{
    return m_instance;
}

IMode *ModeManager::currentMode()
{
    return d ? d->m_currentMode : nullptr;
}

Id ModeManager::currentModeId()
{
    if (!d || !d->m_currentMode)
        return {};
    return d->m_currentMode->id();
}

void ModeManager::addMode(IMode *mode)
{
    QTC_ASSERT(d, return);
    QTC_ASSERT(d->m_startingUp, return);
    d->m_modes.append(mode);
}

void ModeManager::removeMode(IMode *mode)
{
    QTC_ASSERT(d, return);
    const int index = d->m_modes.indexOf(mode);
    if (index < 0)
        return;

    // If we're removing the active mode, fall back to a neighbor first.
    if (mode == d->m_currentMode && d->m_modes.size() > 1) {
        const int fallback = (index > 0) ? index - 1 : 1;
        activateMode(d->m_modes.at(fallback)->id());
    }

    d->m_modes.remove(index);
    if (index < d->m_modeCommands.size())
        d->m_modeCommands.remove(index);

    if (m_instance)
        emit m_instance->modeUnregistered(mode);
}

void ModeManager::activateMode(Id id)
{
    QTC_ASSERT(d, return);
    d->activateModeHelper(id);
}

void ModeManagerPrivate::activateModeHelper(Id id)
{
    if (m_startingUp) {
        m_pendingFirstActiveMode = id;
        return;
    }

    IMode *newMode = findMode(id);
    if (!newMode || newMode == m_currentMode)
        return;

    if (m_instance)
        emit m_instance->currentModeAboutToChange(id);

    // Update context: drop the outgoing mode's context, add the incoming one.
    ICore::updateAdditionalContexts(m_addedContexts, newMode->context());
    m_addedContexts = newMode->context();

    IMode *oldMode = m_currentMode;
    m_currentMode = newMode;

    if (m_instance)
        emit m_instance->currentModeChanged(id, oldMode ? oldMode->id() : Id());
}

void ModeManager::extensionsInitialized()
{
    QTC_ASSERT(d, return);
    d->m_startingUp = false;

    Utils::sort(d->m_modes, &IMode::priority);
    std::reverse(d->m_modes.begin(), d->m_modes.end());

    // Register a per-mode QAction with shortcut Ctrl+1, Ctrl+2, ... so users
    // can switch modes from the keyboard. ActionManager hands out the
    // canonical action; the adapter doesn't need to know.
    for (int i = 0; i < d->m_modes.count(); ++i) {
        d->registerModeCommand(d->m_modes.at(i), i);
        if (m_instance)
            emit m_instance->modeRegistered(d->m_modes.at(i));
    }

    if (m_instance)
        emit m_instance->modesReordered();

    if (d->m_pendingFirstActiveMode.isValid())
        d->activateModeHelper(d->m_pendingFirstActiveMode);
    else if (!d->m_modes.isEmpty())
        d->activateModeHelper(d->m_modes.first()->id());
}

void ModeManagerPrivate::registerModeCommand(IMode *mode, int index)
{
    const Id actionId = mode->id().withPrefix("MyIDE.Mode.");
    auto *action = new QAction(ModeManager::tr("Switch to <b>%1</b> mode")
                                   .arg(mode->displayName()),
                               m_instance);
    Command *cmd = ActionManager::registerAction(action, actionId);
    cmd->setDefaultKeySequence(
        QKeySequence(useMacShortcuts ? QString("Meta+%1").arg(index + 1)
                                     : QString("Ctrl+%1").arg(index + 1)));
    m_modeCommands.append(cmd);

    QObject::connect(action, &QAction::triggered, m_instance, [id = mode->id()] {
        ModeManager::activateMode(id);
    });

    QObject::connect(mode, &IMode::enabledStateChanged, m_instance,
                     [mode](bool enabled) {
                         if (m_instance)
                             emit m_instance->modeEnabledStateChanged(mode, enabled);
                         // Bail out of disabled current mode.
                         if (mode == d->m_currentMode && !enabled) {
                             for (IMode *m : d->m_modes) {
                                 if (m != mode && m->isEnabled()) {
                                     ModeManager::activateMode(m->id());
                                     break;
                                 }
                             }
                         }
                     });
}

void ModeManager::addAction(QAction *action, int priority)
{
    QTC_ASSERT(d, return);
    // Insert sorted descending by priority.
    int insertAt = 0;
    for (; insertAt < d->m_actions.size(); ++insertAt) {
        if (d->m_actions.at(insertAt).priority < priority)
            break;
    }
    d->m_actions.insert(insertAt, {action, priority});

    if (m_instance)
        emit m_instance->modeBarActionAdded(action, priority);
}

void ModeManager::addProjectSelector(QAction *action)
{
    QTC_ASSERT(d, return);
    d->m_projectSelector = action;
    if (m_instance)
        emit m_instance->projectSelectorChanged(action);
}

QList<IMode *> ModeManager::modes()
{
    if (!d)
        return {};
    return QList<IMode *>(d->m_modes.cbegin(), d->m_modes.cend());
}

QList<QAction *> ModeManager::modeBarActions()
{
    if (!d)
        return {};
    QList<QAction *> result;
    result.reserve(d->m_actions.size());
    for (const auto &entry : d->m_actions)
        result.append(entry.action);
    return result;
}

QAction *ModeManager::projectSelectorAction()
{
    return d ? d->m_projectSelector : nullptr;
}

void ModeManager::setFocusToCurrentMode()
{
    IMode *mode = currentMode();
    QTC_ASSERT(mode, return);
    QWidget *widget = mode->widget();
    if (!widget)
        return;
    QWidget *focusWidget = widget->focusWidget();
    if (!focusWidget)
        focusWidget = widget;
    focusWidget->setFocus();
}

void ModeManager::setModeStyle(Style style)
{
    QTC_ASSERT(d, return);
    if (d->m_modeStyle == style)
        return;
    d->m_modeStyle = style;
    if (m_instance)
        emit m_instance->modeStyleChanged(style);
}

void ModeManager::cycleModeStyle()
{
    auto next = Style((int(modeStyle()) + 1) % 3);
    setModeStyle(next);
}

ModeManager::Style ModeManager::modeStyle()
{
    return d ? d->m_modeStyle : Style::IconsAndText;
}

} // namespace Core
