// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modebar_adapter.h"

#include "../icore.h"
#include "../imode.h"
#include "../modemanager.h"

#include <QButtonGroup>
#include <QDebug>
#include <QStackedLayout>
#include <QToolButton>
#include <QVBoxLayout>

namespace Core {
namespace Internal {

ModeBarAdapter::ModeBarAdapter(QWidget *modeBarContainer,
                               QWidget *centralContainer,
                               QObject *parent)
    : QObject(parent)
    , m_modeBarContainer(modeBarContainer)
    , m_centralContainer(centralContainer)
{
    // The mode bar uses an existing QVBoxLayout from the host. If not
    // present, install one.
    if (m_modeBarContainer) {
        m_modeBarLayout = qobject_cast<QVBoxLayout *>(m_modeBarContainer->layout());
        if (!m_modeBarLayout) {
            m_modeBarLayout = new QVBoxLayout(m_modeBarContainer);
            m_modeBarLayout->setContentsMargins(0, 4, 0, 4);
            m_modeBarLayout->setSpacing(2);
            m_modeBarLayout->addStretch();
        }
    }

    // Central container needs a stacked layout for page switching. If the
    // host already has a layout, we leave it alone (the adapter still emits
    // signals, but the host is responsible for swapping mode widgets).
    if (m_centralContainer) {
        if (!m_centralContainer->layout()) {
            m_centralStack = new QStackedLayout(m_centralContainer);
            m_centralStack->setContentsMargins(0, 0, 0, 0);
        } else {
            m_centralStack = qobject_cast<QStackedLayout *>(m_centralContainer->layout());
            if (!m_centralStack) {
                qWarning() << "ModeBarAdapter: centralContainer has an existing"
                              " layout; mode widget switching will be a no-op.";
            }
        }
    }

    ModeManager *mm = ModeManager::instance();
    if (!mm) {
        qWarning() << "ModeBarAdapter: ModeManager not yet constructed.";
        return;
    }

    connect(mm, &ModeManager::modeRegistered, this, &ModeBarAdapter::addModeButton);
    connect(mm, &ModeManager::modeUnregistered, this, &ModeBarAdapter::removeModeButton);
    connect(mm, &ModeManager::modesReordered, this, &ModeBarAdapter::rebuild);
    connect(mm, &ModeManager::currentModeChanged, this, &ModeBarAdapter::onCurrentModeChanged);
    connect(mm, &ModeManager::modeEnabledStateChanged, this,
            [this](IMode *mode, bool enabled) {
                if (auto *btn = m_buttons.value(mode))
                    btn->setEnabled(enabled);
            });
}

ModeBarAdapter::~ModeBarAdapter() = default;

void ModeBarAdapter::rebuild()
{
    // Drop existing buttons (the stretch entry is kept).
    for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it)
        it.value()->deleteLater();
    m_buttons.clear();

    // Pages stay around — widgets owned by the IMode. Just clear our cache.
    if (m_centralStack) {
        while (m_centralStack->count() > 0) {
            QWidget *w = m_centralStack->widget(0);
            m_centralStack->removeWidget(w);
        }
    }
    m_pages.clear();

    const QList<IMode *> modes = ModeManager::modes();
    for (IMode *mode : modes)
        addModeButton(mode);

    if (IMode *cur = ModeManager::currentMode())
        setActiveMode(cur);
}

void ModeBarAdapter::addModeButton(IMode *mode)
{
    if (!mode)
        return;
    if (m_buttons.contains(mode))
        return;

    if (m_modeBarLayout) {
        auto *btn = new QToolButton(m_modeBarContainer);
        btn->setObjectName(QStringLiteral("mode-button-") + mode->id().toString());
        btn->setText(mode->displayName());
        btn->setIcon(mode->icon());
        btn->setToolTip(mode->displayName());
        btn->setCheckable(true);
        btn->setAutoExclusive(true);
        btn->setEnabled(mode->isEnabled());
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setFixedSize(40, 40);
        connect(btn, &QToolButton::clicked, this,
                [id = mode->id()] { ModeManager::activateMode(id); });

        // Insert before the trailing stretch so buttons stack from the top.
        const int insertAt = qMax(0, m_modeBarLayout->count() - 1);
        m_modeBarLayout->insertWidget(insertAt, btn);
        m_buttons.insert(mode, btn);
    }

    if (m_centralStack && mode->widget()) {
        m_centralStack->addWidget(mode->widget());
        m_pages.insert(mode, mode->widget());
    }
}

void ModeBarAdapter::removeModeButton(IMode *mode)
{
    if (auto *btn = m_buttons.take(mode))
        btn->deleteLater();
    if (auto *w = m_pages.take(mode)) {
        if (m_centralStack)
            m_centralStack->removeWidget(w);
    }
}

void ModeBarAdapter::setActiveMode(IMode *mode)
{
    if (!mode)
        return;
    if (auto *btn = m_buttons.value(mode))
        btn->setChecked(true);
    if (m_centralStack) {
        QWidget *page = m_pages.value(mode, mode->widget());
        if (page && m_centralStack->indexOf(page) < 0) {
            m_centralStack->addWidget(page);
            m_pages.insert(mode, page);
        }
        if (page)
            m_centralStack->setCurrentWidget(page);
    }
}

void ModeBarAdapter::onCurrentModeChanged(Utils::Id id, Utils::Id oldId)
{
    Q_UNUSED(oldId)
    for (auto it = m_buttons.cbegin(); it != m_buttons.cend(); ++it) {
        if (it.key()->id() == id) {
            setActiveMode(it.key());
            return;
        }
    }
}

} // namespace Internal
} // namespace Core
