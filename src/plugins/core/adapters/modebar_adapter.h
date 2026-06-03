// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// ModeBarAdapter — UI bridge between Core::ModeManager and the host main
// window. The host exposes a left-side mode bar container (a plain QWidget)
// via ICore::modeBarContainer() and a central content container via
// ICore::centralContainer(). The adapter:
//   * builds a checkable QToolButton per IMode and stacks them in the mode bar
//   * installs a QStackedLayout into the central container and adds each
//     mode's widget as a page
//   * listens to ModeManager signals to add/remove/enable/disable modes,
//     and to flip the central stack when currentModeChanged fires
//
// The adapter is owned by the host (typically main.cpp constructs it after
// CorePlugin::extensionsInitialized() has fired modesReordered).

#pragma once

#include <utils/id.h>

#include <QHash>
#include <QObject>

#include "../core_global.h"

QT_BEGIN_NAMESPACE
class QStackedLayout;
class QToolButton;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

namespace Core {

class IMode;

namespace Internal {

class CORE_EXPORT ModeBarAdapter : public QObject
{
    Q_OBJECT
public:
    // modeBarContainer: host widget that holds the mode buttons (left toolbar)
    // centralContainer: host widget where mode widgets are stacked
    // Both can be nullptr — the adapter no-ops gracefully in that case.
    explicit ModeBarAdapter(QWidget *modeBarContainer,
                            QWidget *centralContainer,
                            QObject *parent = nullptr);
    ~ModeBarAdapter() override;

    // Rebuild from the current ModeManager::modes() snapshot. Call once after
    // ModeManager::extensionsInitialized() has fired.
    void rebuild();

private:
    void addModeButton(IMode *mode);
    void removeModeButton(IMode *mode);
    void setActiveMode(IMode *mode);
    void onCurrentModeChanged(Utils::Id id, Utils::Id oldId);

    QWidget *m_modeBarContainer = nullptr;
    QWidget *m_centralContainer = nullptr;
    QVBoxLayout *m_modeBarLayout = nullptr;
    QStackedLayout *m_centralStack = nullptr;
    QHash<IMode *, QToolButton *> m_buttons;
    QHash<IMode *, QWidget *> m_pages;
};

} // namespace Internal
} // namespace Core
