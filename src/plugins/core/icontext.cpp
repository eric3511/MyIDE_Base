// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Extracted from Qt Creator (coreplugin/icontext.cpp).
// Only the QDebug operator<< — everything else is header-only.

#include "icontext.h"

#include <QDebug>

namespace Core {
QDebug operator<<(QDebug debug, const Core::Context &context)
{
    debug.nospace() << "Context(";
    Core::Context::const_iterator it = context.begin();
    Core::Context::const_iterator end = context.end();
    if (it != end) {
        debug << *it;
        ++it;
    }
    while (it != end) {
        debug << ", " << *it;
        ++it;
    }
    debug << ')';

    return debug;
}
} // namespace Core
