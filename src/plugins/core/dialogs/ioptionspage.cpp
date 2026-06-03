// Copyright (C) 2026 MyIDE
// Derived from Qt Creator (The Qt Company Ltd.) — original copyright preserved below.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Original header (Qt Creator):
/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Falko Arps
** Copyright (C) 2016 Sven Klein
** Copyright (C) 2016 Giuliano Schneider
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "ioptionspage.h"

#include "../icore.h"

#include <utils/aspects.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QCheckBox>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>

using namespace Utils;

namespace Core {

QIcon IOptionsPage::categoryIcon() const
{
    return m_categoryIcon.icon();
}

void IOptionsPage::setWidgetCreator(const WidgetCreator &widgetCreator)
{
    m_widgetCreator = widgetCreator;
}

QWidget *IOptionsPage::widget()
{
    if (!m_widget) {
        if (m_widgetCreator) {
            m_widget = m_widgetCreator();
        } else if (m_layouter) {
            m_widget = new QWidget;
            m_layouter(m_widget);
        } else {
            QTC_CHECK(false);
        }
    }
    return m_widget;
}

void IOptionsPage::apply()
{
    if (auto widget = qobject_cast<IOptionsPageWidget *>(m_widget)) {
        widget->apply();
    } else if (m_settings) {
        if (m_settings->isDirty()) {
            m_settings->apply();
            m_settings->writeSettings(ICore::settings());
         }
    }
}

void IOptionsPage::finish()
{
    if (auto widget = qobject_cast<IOptionsPageWidget *>(m_widget))
        widget->finish();
    else if (m_settings)
        m_settings->finish();

    delete m_widget;
}

void IOptionsPage::setCategoryIconPath(const FilePath &categoryIconPath)
{
    m_categoryIcon = Icon({{categoryIconPath, Theme::PanelTextColorDark}}, Icon::Tint);
}

void IOptionsPage::setSettings(AspectContainer *settings)
{
    m_settings = settings;
}

void IOptionsPage::setLayouter(const std::function<void(QWidget *w)> &layouter)
{
    m_layouter = layouter;
}

static QList<IOptionsPage *> g_optionsPages;

IOptionsPage::IOptionsPage(QObject *parent, bool registerGlobally)
    : QObject(parent)
{
    if (registerGlobally)
        g_optionsPages.append(this);
}

IOptionsPage::~IOptionsPage()
{
    g_optionsPages.removeOne(this);
}

const QList<IOptionsPage *> IOptionsPage::allOptionsPages()
{
    return g_optionsPages;
}

bool IOptionsPage::matches(const QRegularExpression &regexp) const
{
    if (!m_keywordsInitialized) {
        auto that = const_cast<IOptionsPage *>(this);
        QWidget *widget = that->widget();
        if (!widget)
            return false;
        // find common subwidgets
        for (const QLabel *label : widget->findChildren<QLabel *>())
            m_keywords << Utils::stripAccelerator(label->text());
        for (const QCheckBox *checkbox : widget->findChildren<QCheckBox *>())
            m_keywords << Utils::stripAccelerator(checkbox->text());
        for (const QPushButton *pushButton : widget->findChildren<QPushButton *>())
            m_keywords << Utils::stripAccelerator(pushButton->text());
        for (const QGroupBox *groupBox : widget->findChildren<QGroupBox *>())
            m_keywords << Utils::stripAccelerator(groupBox->title());

        m_keywordsInitialized = true;
    }
    for (const QString &keyword : qAsConst(m_keywords))
        if (keyword.contains(regexp))
            return true;
    return false;
}

static QList<IOptionsPageProvider *> g_optionsPagesProviders;

IOptionsPageProvider::IOptionsPageProvider(QObject *parent)
    : QObject(parent)
{
    g_optionsPagesProviders.append(this);
}

IOptionsPageProvider::~IOptionsPageProvider()
{
    g_optionsPagesProviders.removeOne(this);
}

const QList<IOptionsPageProvider *> IOptionsPageProvider::allOptionsPagesProviders()
{
    return g_optionsPagesProviders;
}

QIcon IOptionsPageProvider::categoryIcon() const
{
    return m_categoryIcon.icon();
}

} // Core
