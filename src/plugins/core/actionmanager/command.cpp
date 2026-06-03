// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Extracted from Qt Creator (coreplugin/actionmanager/command.cpp).
// Includes rewritten: <coreplugin/coreconstants.h> → "../coreconstants.h", etc.

#include "command.h"
#include "command_p.h"

#include "../coreconstants.h"
#include "../icontext.h"

#include <utils/hostosinfo.h>
#include <utils/stringutils.h>

#include <QAction>
#include <QToolButton>
#include <QTextStream>

using namespace Utils;

namespace Core {

Command::Command(Utils::Id id)
    : d(new Internal::CommandPrivate(this))
{
    d->m_id = id;
}

Command::~Command()
{
    delete d;
}

Internal::CommandPrivate::CommandPrivate(Command *parent)
    : m_q(parent)
    , m_attributes({})
    , m_action(new Utils::ProxyAction(this))
{
    m_action->setShortcutVisibleInToolTip(true);
    connect(m_action, &QAction::changed, this, &CommandPrivate::updateActiveState);
}

Id Command::id() const
{
    return d->m_id;
}

void Command::setDefaultKeySequence(const QKeySequence &key)
{
    if (!d->m_isKeyInitialized)
        setKeySequences({key});
    d->m_defaultKeys = {key};
}

void Command::setDefaultKeySequences(const QList<QKeySequence> &keys)
{
    if (!d->m_isKeyInitialized)
        setKeySequences(keys);
    d->m_defaultKeys = keys;
}

QList<QKeySequence> Command::defaultKeySequences() const
{
    return d->m_defaultKeys;
}

QAction *Command::action() const
{
    return d->m_action;
}

QString Command::stringWithAppendedShortcut(const QString &str) const
{
    return Utils::ProxyAction::stringWithAppendedShortcut(str, keySequence());
}

Context Command::context() const
{
    return d->m_context;
}

void Command::setKeySequences(const QList<QKeySequence> &keys)
{
    d->m_isKeyInitialized = true;
    d->m_action->setShortcuts(keys);
    emit keySequenceChanged();
}

QList<QKeySequence> Command::keySequences() const
{
    return d->m_action->shortcuts();
}

QKeySequence Command::keySequence() const
{
    return d->m_action->shortcut();
}

void Command::setDescription(const QString &text)
{
    d->m_defaultText = text;
}

QString Command::description() const
{
    if (!d->m_defaultText.isEmpty())
        return d->m_defaultText;
    if (QAction *act = action()) {
        const QString text = Utils::stripAccelerator(act->text());
        if (!text.isEmpty())
            return text;
    }
    return id().toString();
}

void Internal::CommandPrivate::setCurrentContext(const Context &context)
{
    m_context = context;

    QAction *currentAction = nullptr;
    for (int i = 0; i < m_context.size(); ++i) {
        if (QAction *a = m_contextActionMap.value(m_context.at(i), nullptr)) {
            currentAction = a;
            break;
        }
    }

    m_action->setAction(currentAction);
    updateActiveState();
}

void Internal::CommandPrivate::updateActiveState()
{
    setActive(m_action->isEnabled() && m_action->isVisible() && !m_action->isSeparator());
}

static QString msgActionWarning(QAction *newAction, Id id, QAction *oldAction)
{
    QString msg;
    QTextStream str(&msg);
    str << "addOverrideAction " << newAction->objectName() << '/' << newAction->text()
         << ": Action ";
    if (oldAction)
        str << oldAction->objectName() << '/' << oldAction->text();
    str << " is already registered for context " << id.toString() << '.';
    return msg;
}

void Internal::CommandPrivate::addOverrideAction(QAction *action,
                                                 const Context &context,
                                                 bool scriptable)
{
    // disallow TextHeuristic menu role, because it doesn't work with translations,
    // e.g. QTCREATORBUG-13101
    if (action->menuRole() == QAction::TextHeuristicRole)
        action->setMenuRole(QAction::NoRole);
    if (isEmpty())
        m_action->initialize(action);
    if (context.isEmpty()) {
        m_contextActionMap.insert(Constants::C_GLOBAL, action);
    } else {
        for (const Id &id : context) {
            if (m_contextActionMap.contains(id))
                qWarning("%s", qPrintable(msgActionWarning(action, id, m_contextActionMap.value(id, nullptr))));
            m_contextActionMap.insert(id, action);
        }
    }
    m_scriptableMap[action] = scriptable;
    setCurrentContext(m_context);
}

void Internal::CommandPrivate::removeOverrideAction(QAction *action)
{
    QList<Id> toRemove;
    for (auto it = m_contextActionMap.cbegin(), end = m_contextActionMap.cend(); it != end; ++it) {
        if (it.value() == nullptr || it.value() == action)
            toRemove.append(it.key());
    }
    for (Id id : toRemove)
        m_contextActionMap.remove(id);
    setCurrentContext(m_context);
}

bool Command::isActive() const
{
    return d->m_active;
}

void Internal::CommandPrivate::setActive(bool state)
{
    if (state != m_active) {
        m_active = state;
        emit m_q->activeStateChanged();
    }
}

bool Internal::CommandPrivate::isEmpty() const
{
    return m_contextActionMap.isEmpty();
}

bool Command::isScriptable() const
{
    return std::find(d->m_scriptableMap.cbegin(), d->m_scriptableMap.cend(), true)
           != d->m_scriptableMap.cend();
}

bool Command::isScriptable(const Context &context) const
{
    if (context == d->m_context && d->m_scriptableMap.contains(d->m_action->action()))
        return d->m_scriptableMap.value(d->m_action->action());

    for (int i = 0; i < context.size(); ++i) {
        if (QAction *a = d->m_contextActionMap.value(context.at(i), nullptr)) {
            if (d->m_scriptableMap.contains(a) && d->m_scriptableMap.value(a))
                return true;
        }
    }
    return false;
}

void Command::setAttribute(CommandAttribute attr)
{
    d->m_attributes |= attr;
    switch (attr) {
    case Command::CA_Hide:
        d->m_action->setAttribute(Utils::ProxyAction::Hide);
        break;
    case Command::CA_UpdateText:
        d->m_action->setAttribute(Utils::ProxyAction::UpdateText);
        break;
    case Command::CA_UpdateIcon:
        d->m_action->setAttribute(Utils::ProxyAction::UpdateIcon);
        break;
    case Command::CA_NonConfigurable:
        break;
    }
}

void Command::removeAttribute(CommandAttribute attr)
{
    d->m_attributes &= ~attr;
    switch (attr) {
    case Command::CA_Hide:
        d->m_action->removeAttribute(Utils::ProxyAction::Hide);
        break;
    case Command::CA_UpdateText:
        d->m_action->removeAttribute(Utils::ProxyAction::UpdateText);
        break;
    case Command::CA_UpdateIcon:
        d->m_action->removeAttribute(Utils::ProxyAction::UpdateIcon);
        break;
    case Command::CA_NonConfigurable:
        break;
    }
}

bool Command::hasAttribute(CommandAttribute attr) const
{
    return (d->m_attributes & attr);
}

void Command::setTouchBarText(const QString &text)
{
    d->m_touchBarText = text;
}

QString Command::touchBarText() const
{
    return d->m_touchBarText;
}

void Command::setTouchBarIcon(const QIcon &icon)
{
    d->m_touchBarIcon = icon;
}

QIcon Command::touchBarIcon() const
{
    return d->m_touchBarIcon;
}

QAction *Command::touchBarAction() const
{
    if (!d->m_touchBarAction) {
        d->m_touchBarAction = std::make_unique<Utils::ProxyAction>();
        d->m_touchBarAction->initialize(d->m_action);
        d->m_touchBarAction->setIcon(d->m_touchBarIcon);
        d->m_touchBarAction->setText(d->m_touchBarText);
        // the touch bar action should be hidden if the command is not valid for the context
        d->m_touchBarAction->setAttribute(Utils::ProxyAction::Hide);
        d->m_touchBarAction->setAction(d->m_action->action());
        connect(d->m_action,
                &Utils::ProxyAction::currentActionChanged,
                d->m_touchBarAction.get(),
                &Utils::ProxyAction::setAction);
    }
    return d->m_touchBarAction.get();
}

void Command::augmentActionWithShortcutToolTip(QAction *a) const
{
    a->setToolTip(stringWithAppendedShortcut(a->text()));
    QObject::connect(this, &Command::keySequenceChanged, a, [this, a]() {
        a->setToolTip(stringWithAppendedShortcut(a->text()));
    });
    QObject::connect(a, &QAction::changed, this, [this, a]() {
        a->setToolTip(stringWithAppendedShortcut(a->text()));
    });
}

QToolButton *Command::toolButtonWithAppendedShortcut(QAction *action, Command *cmd)
{
    auto button = new QToolButton;
    button->setDefaultAction(action);
    if (cmd)
        cmd->augmentActionWithShortcutToolTip(action);
    return button;
}

} // namespace Core
