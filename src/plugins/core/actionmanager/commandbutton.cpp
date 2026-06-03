// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Extracted from Qt Creator (coreplugin/actionmanager/commandbutton.cpp).

#include "commandbutton.h"
#include "actionmanager.h"
#include "command.h"

#include <utils/proxyaction.h>

using namespace Core;
using namespace Utils;

CommandButton::CommandButton(QWidget *parent)
    : QToolButton(parent)
    , m_command(nullptr)
{
}

CommandButton::CommandButton(Id id, QWidget *parent)
    : QToolButton(parent)
    , m_command(nullptr)
{
    setCommandId(id);
}

void CommandButton::setCommandId(Id id)
{
    if (m_command)
        disconnect(m_command.data(), &Command::keySequenceChanged, this, &CommandButton::updateToolTip);

    m_command = ActionManager::command(id);

    if (m_toolTipBase.isEmpty())
        m_toolTipBase = m_command->description();

    updateToolTip();
    connect(m_command.data(), &Command::keySequenceChanged, this, &CommandButton::updateToolTip);
}

QString CommandButton::toolTipBase() const
{
    return m_toolTipBase;
}

void CommandButton::setToolTipBase(const QString &toolTipBase)
{
    m_toolTipBase = toolTipBase;
    updateToolTip();
}

void CommandButton::updateToolTip()
{
    if (m_command)
        setToolTip(Utils::ProxyAction::stringWithAppendedShortcut(m_toolTipBase,
                                                                  m_command->keySequence()));
}
