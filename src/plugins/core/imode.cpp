// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Extracted from Qt Creator coreplugin/imode.cpp — unchanged.

#include "imode.h"

#include "modemanager.h"

namespace Core {

IMode::IMode(QObject *parent) : IContext(parent)
{
    ModeManager::addMode(this);
}

void IMode::setEnabled(bool enabled)
{
    if (m_isEnabled == enabled)
        return;
    m_isEnabled = enabled;
    emit enabledStateChanged(m_isEnabled);
}

bool IMode::isEnabled() const
{
    return m_isEnabled;
}

} // namespace Core
