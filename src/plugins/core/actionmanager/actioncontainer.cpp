// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Extracted from Qt Creator (coreplugin/actionmanager/actioncontainer.cpp).
// Includes rewritten: <coreplugin/coreconstants.h> → "../coreconstants.h", etc.

#include "actioncontainer_p.h"
#include "actionmanager.h"

#include "../coreconstants.h"
#include "../icontext.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTimer>
#include <QAction>
#include <QMenuBar>

Q_DECLARE_METATYPE(Core::Internal::MenuActionContainer*)

using namespace Utils;

namespace Core {

namespace Internal {

// ---------- ActionContainerPrivate ------------

ActionContainerPrivate::ActionContainerPrivate(Id id)
    : m_onAllDisabledBehavior(Disable), m_id(id), m_updateRequested(false)
{
    appendGroup(Constants::G_DEFAULT_ONE);
    appendGroup(Constants::G_DEFAULT_TWO);
    appendGroup(Constants::G_DEFAULT_THREE);
    scheduleUpdate();
}

void ActionContainerPrivate::setOnAllDisabledBehavior(OnAllDisabledBehavior behavior)
{
    m_onAllDisabledBehavior = behavior;
}

ActionContainer::OnAllDisabledBehavior ActionContainerPrivate::onAllDisabledBehavior() const
{
    return m_onAllDisabledBehavior;
}

void ActionContainerPrivate::appendGroup(Id groupId)
{
    m_groups.append(Group(groupId));
}

void ActionContainerPrivate::insertGroup(Id before, Id groupId)
{
    QList<Group>::iterator it = m_groups.begin();
    while (it != m_groups.end()) {
        if (it->id == before) {
            m_groups.insert(it, Group(groupId));
            break;
        }
        ++it;
    }
}

QList<Group>::const_iterator ActionContainerPrivate::findGroup(Id groupId) const
{
    QList<Group>::const_iterator it = m_groups.constBegin();
    while (it != m_groups.constEnd()) {
        if (it->id == groupId)
            break;
        ++it;
    }
    return it;
}


QAction *ActionContainerPrivate::insertLocation(Id groupId) const
{
    QList<Group>::const_iterator it = findGroup(groupId);
    QTC_ASSERT(it != m_groups.constEnd(), return nullptr);
    return insertLocation(it);
}

QAction *ActionContainerPrivate::actionForItem(QObject *item) const
{
    if (auto cmd = qobject_cast<Command *>(item)) {
        return cmd->action();
    } else if (auto container = qobject_cast<ActionContainerPrivate *>(item)) {
        if (container->containerAction())
            return container->containerAction();
    }
    QTC_ASSERT(false, return nullptr);
}

QAction *ActionContainerPrivate::insertLocation(QList<Group>::const_iterator group) const
{
    if (group == m_groups.constEnd())
        return nullptr;
    ++group;
    while (group != m_groups.constEnd()) {
        if (!group->items.isEmpty()) {
            QObject *item = group->items.first();
            QAction *action = actionForItem(item);
            if (action)
                return action;
        }
        ++group;
    }
    return nullptr;
}

void ActionContainerPrivate::addAction(Command *command, Id groupId)
{
    if (!canAddAction(command))
        return;

    const Id actualGroupId = groupId.isValid() ? groupId : Id(Constants::G_DEFAULT_TWO);
    QList<Group>::const_iterator groupIt = findGroup(actualGroupId);
    QTC_ASSERT(groupIt != m_groups.constEnd(), qDebug() << "Can't find group"
               << groupId.name() << "in container" << id().name(); return);
    m_groups[groupIt - m_groups.constBegin()].items.append(command);
    connect(command, &Command::activeStateChanged, this, &ActionContainerPrivate::scheduleUpdate);
    connect(command, &QObject::destroyed, this, &ActionContainerPrivate::itemDestroyed);

    QAction *beforeAction = insertLocation(groupIt);
    insertAction(beforeAction, command);

    scheduleUpdate();
}

void ActionContainerPrivate::addMenu(ActionContainer *menu, Id groupId)
{
    auto containerPrivate = static_cast<ActionContainerPrivate *>(menu);
    QTC_ASSERT(containerPrivate->canBeAddedToContainer(this), return);

    const Id actualGroupId = groupId.isValid() ? groupId : Id(Constants::G_DEFAULT_TWO);
    QList<Group>::const_iterator groupIt = findGroup(actualGroupId);
    QTC_ASSERT(groupIt != m_groups.constEnd(), return);
    m_groups[groupIt - m_groups.constBegin()].items.append(menu);
    connect(menu, &QObject::destroyed, this, &ActionContainerPrivate::itemDestroyed);

    QAction *beforeAction = insertLocation(groupIt);
    insertMenu(beforeAction, menu);

    scheduleUpdate();
}

void ActionContainerPrivate::addMenu(ActionContainer *before, ActionContainer *menu)
{
    auto containerPrivate = static_cast<ActionContainerPrivate *>(menu);
    QTC_ASSERT(containerPrivate->canBeAddedToContainer(this), return);

    for (Group &group : m_groups) {
        const int insertionPoint = group.items.indexOf(before);
        if (insertionPoint >= 0) {
            group.items.insert(insertionPoint, menu);
            break;
        }
    }
    connect(menu, &QObject::destroyed, this, &ActionContainerPrivate::itemDestroyed);

    auto beforePrivate = static_cast<ActionContainerPrivate *>(before);
    QAction *beforeAction = beforePrivate->containerAction();
    if (beforeAction)
        insertMenu(beforeAction, menu);

    scheduleUpdate();
}

Command *ActionContainerPrivate::addSeparator(const Context &context, Id group, QAction **outSeparator)
{
    static int separatorIdCount = 0;
    auto separator = new QAction(this);
    separator->setSeparator(true);
    Id sepId = id().withSuffix(".Separator.").withSuffix(++separatorIdCount);
    Command *cmd = ActionManager::registerAction(separator, sepId, context);
    addAction(cmd, group);
    if (outSeparator)
        *outSeparator = separator;
    return cmd;
}

void ActionContainerPrivate::clear()
{
    for (Group &group : m_groups) {
        for (QObject *item : qAsConst(group.items)) {
            if (auto command = qobject_cast<Command *>(item)) {
                removeAction(command);
                disconnect(command, &Command::activeStateChanged,
                           this, &ActionContainerPrivate::scheduleUpdate);
                disconnect(command, &QObject::destroyed, this, &ActionContainerPrivate::itemDestroyed);
            } else if (auto container = qobject_cast<ActionContainer *>(item)) {
                container->clear();
                disconnect(container, &QObject::destroyed,
                           this, &ActionContainerPrivate::itemDestroyed);
                removeMenu(container);
            }
        }
        group.items.clear();
    }
    scheduleUpdate();
}

void ActionContainerPrivate::itemDestroyed()
{
    QObject *obj = sender();
    for (Group &group : m_groups) {
        if (group.items.removeAll(obj) > 0)
            break;
    }
}

Id ActionContainerPrivate::id() const
{
    return m_id;
}

QMenu *ActionContainerPrivate::menu() const
{
    return nullptr;
}

QMenuBar *ActionContainerPrivate::menuBar() const
{
    return nullptr;
}

TouchBar *ActionContainerPrivate::touchBar() const
{
    return nullptr;
}

bool ActionContainerPrivate::canAddAction(Command *action)
{
    return action && action->action();
}

void ActionContainerPrivate::scheduleUpdate()
{
    if (m_updateRequested)
        return;
    m_updateRequested = true;
    QMetaObject::invokeMethod(this, &ActionContainerPrivate::update, Qt::QueuedConnection);
}

void ActionContainerPrivate::update()
{
    updateInternal();
    m_updateRequested = false;
}

// ---------- MenuActionContainer ------------

MenuActionContainer::MenuActionContainer(Id id)
    : ActionContainerPrivate(id),
      m_menu(new QMenu)
{
    m_menu->setObjectName(id.toString());
    m_menu->menuAction()->setMenuRole(QAction::NoRole);
    setOnAllDisabledBehavior(Disable);
}

MenuActionContainer::~MenuActionContainer()
{
    delete m_menu;
}

QMenu *MenuActionContainer::menu() const
{
    return m_menu;
}

QAction *MenuActionContainer::containerAction() const
{
    return m_menu->menuAction();
}

void MenuActionContainer::insertAction(QAction *before, Command *command)
{
    m_menu->insertAction(before, command->action());
}

void MenuActionContainer::insertMenu(QAction *before, ActionContainer *container)
{
    QMenu *menu = container->menu();
    QTC_ASSERT(menu, return);
    menu->setParent(m_menu, menu->windowFlags()); // work around issues with Qt Wayland (QTBUG-68636)
    m_menu->insertMenu(before, menu);
}

void MenuActionContainer::removeAction(Command *command)
{
    m_menu->removeAction(command->action());
}

void MenuActionContainer::removeMenu(ActionContainer *container)
{
    QMenu *menu = container->menu();
    QTC_ASSERT(menu, return);
    m_menu->removeAction(menu->menuAction());
}

bool MenuActionContainer::updateInternal()
{
    if (onAllDisabledBehavior() == Show)
        return true;

    bool hasitems = false;
    QList<QAction *> actions = m_menu->actions();

    for (const Group &group : qAsConst(m_groups)) {
        for (QObject *item : qAsConst(group.items)) {
            if (auto container = qobject_cast<ActionContainerPrivate*>(item)) {
                actions.removeAll(container->menu()->menuAction());
                if (container == this) {
                    QByteArray warning = Q_FUNC_INFO + QByteArray(" container '");
                    if (this->menu())
                        warning += this->menu()->title().toLocal8Bit();
                    warning += "' contains itself as subcontainer";
                    qWarning("%s", warning.constData());
                    continue;
                }
                if (container->updateInternal()) {
                    hasitems = true;
                    break;
                }
            } else if (auto command = qobject_cast<Command *>(item)) {
                actions.removeAll(command->action());
                if (command->isActive()) {
                    hasitems = true;
                    break;
                }
            } else {
                QTC_ASSERT(false, continue);
            }
        }
        if (hasitems)
            break;
    }
    if (!hasitems) {
        // look if there were actions added that we don't control and check if they are enabled
        for (const QAction *action : qAsConst(actions)) {
            if (!action->isSeparator() && action->isEnabled()) {
                hasitems = true;
                break;
            }
        }
    }

    if (onAllDisabledBehavior() == Hide)
        m_menu->menuAction()->setVisible(hasitems);
    else if (onAllDisabledBehavior() == Disable)
        m_menu->menuAction()->setEnabled(hasitems);

    return hasitems;
}

bool MenuActionContainer::canBeAddedToContainer(ActionContainerPrivate *container) const
{
    return qobject_cast<MenuActionContainer *>(container)
           || qobject_cast<MenuBarActionContainer *>(container);
}

// ---------- MenuBarActionContainer ------------

MenuBarActionContainer::MenuBarActionContainer(Id id)
    : ActionContainerPrivate(id), m_menuBar(nullptr)
{
    setOnAllDisabledBehavior(Show);
}

void MenuBarActionContainer::setMenuBar(QMenuBar *menuBar)
{
    m_menuBar = menuBar;
}

QMenuBar *MenuBarActionContainer::menuBar() const
{
    return m_menuBar;
}

QAction *MenuBarActionContainer::containerAction() const
{
    return nullptr;
}

void MenuBarActionContainer::insertAction(QAction *before, Command *command)
{
    m_menuBar->insertAction(before, command->action());
}

void MenuBarActionContainer::insertMenu(QAction *before, ActionContainer *container)
{
    QMenu *menu = container->menu();
    QTC_ASSERT(menu, return);
    menu->setParent(m_menuBar, menu->windowFlags()); // work around issues with Qt Wayland (QTBUG-68636)
    m_menuBar->insertMenu(before, menu);
}

void MenuBarActionContainer::removeAction(Command *command)
{
    m_menuBar->removeAction(command->action());
}

void MenuBarActionContainer::removeMenu(ActionContainer *container)
{
    QMenu *menu = container->menu();
    QTC_ASSERT(menu, return);
    m_menuBar->removeAction(menu->menuAction());
}

bool MenuBarActionContainer::updateInternal()
{
    if (onAllDisabledBehavior() == Show)
        return true;

    bool hasitems = false;
    QList<QAction *> actions = m_menuBar->actions();
    for (int i=0; i<actions.size(); ++i) {
        if (actions.at(i)->isVisible()) {
            hasitems = true;
            break;
        }
    }

    if (onAllDisabledBehavior() == Hide)
        m_menuBar->setVisible(hasitems);
    else if (onAllDisabledBehavior() == Disable)
        m_menuBar->setEnabled(hasitems);

    return hasitems;
}

bool MenuBarActionContainer::canBeAddedToContainer(ActionContainerPrivate *) const
{
    return false;
}

// ---------- TouchBarActionContainer ------------

const char ID_PREFIX[] = "io.qt.qtcreator.";

TouchBarActionContainer::TouchBarActionContainer(Id id, const QIcon &icon, const QString &text)
    : ActionContainerPrivate(id),
      m_touchBar(std::make_unique<TouchBar>(id.withPrefix(ID_PREFIX).name(), icon, text))
{
}

TouchBarActionContainer::~TouchBarActionContainer() = default;

TouchBar *TouchBarActionContainer::touchBar() const
{
    return m_touchBar.get();
}

QAction *TouchBarActionContainer::containerAction() const
{
    return m_touchBar->touchBarAction();
}

QAction *TouchBarActionContainer::actionForItem(QObject *item) const
{
    if (Command *command = qobject_cast<Command *>(item))
        return command->touchBarAction();
    return ActionContainerPrivate::actionForItem(item);
}

void TouchBarActionContainer::insertAction(QAction *before, Command *command)
{
    m_touchBar->insertAction(before,
                             command->id().withPrefix(ID_PREFIX).name(),
                             command->touchBarAction());
}

void TouchBarActionContainer::insertMenu(QAction *before, ActionContainer *container)
{
    TouchBar *touchBar = container->touchBar();
    QTC_ASSERT(touchBar, return);
    m_touchBar->insertTouchBar(before, touchBar);
}

void TouchBarActionContainer::removeAction(Command *command)
{
    m_touchBar->removeAction(command->touchBarAction());
}

void TouchBarActionContainer::removeMenu(ActionContainer *container)
{
    TouchBar *touchBar = container->touchBar();
    QTC_ASSERT(touchBar, return);
    m_touchBar->removeTouchBar(touchBar);
}

bool TouchBarActionContainer::canBeAddedToContainer(ActionContainerPrivate *container) const
{
    return qobject_cast<TouchBarActionContainer *>(container);
}

bool TouchBarActionContainer::updateInternal()
{
    return false;
}

} // namespace Internal

Command *ActionContainer::addSeparator(Id group)
{
    static const Context context(Constants::C_GLOBAL);
    return addSeparator(context, group);
}

} // namespace Core
