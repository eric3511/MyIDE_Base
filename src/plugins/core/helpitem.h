// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Minimal HelpItem stub for PR-1.
// The full Qt Creator HelpItem (with content extraction, links, etc.) is
// deferred to the Help system module (Tier 5 of the extraction roadmap).
// For now IContext::contextHelp() just needs a default-constructible handle.

#pragma once

#include "core_global.h"

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>

#include <utility>
#include <vector>

namespace Core {

class CORE_EXPORT HelpItem
{
public:
    using Link = std::pair<QString, QUrl>;
    using Links = std::vector<Link>;

    enum Category {
        ClassOrNamespace,
        Enum,
        Typedef,
        Macro,
        Brief,
        Function,
        QmlComponent,
        QmlProperty,
        QMakeVariableOfFunction,
        Unknown
    };

    HelpItem() = default;
    explicit HelpItem(const QString &helpId) : m_helpIds(helpId) {}
    HelpItem(const QString &helpId, const QString &docMark, Category category)
        : m_helpIds(helpId), m_docMark(docMark), m_category(category) {}
    HelpItem(const QStringList &helpIds, const QString &docMark, Category category)
        : m_helpIds(helpIds), m_docMark(docMark), m_category(category) {}
    explicit HelpItem(const QUrl &url) : m_helpUrl(url) {}
    HelpItem(const QUrl &url, const QString &docMark, Category category)
        : m_helpUrl(url), m_docMark(docMark), m_category(category) {}

    void setHelpUrl(const QUrl &url) { m_helpUrl = url; }
    const QUrl &helpUrl() const { return m_helpUrl; }

    void setHelpIds(const QStringList &ids) { m_helpIds = ids; }
    const QStringList &helpIds() const { return m_helpIds; }

    void setDocMark(const QString &mark) { m_docMark = mark; }
    const QString &docMark() const { return m_docMark; }

    void setCategory(Category cat) { m_category = cat; }
    Category category() const { return m_category; }

    bool isEmpty() const { return m_helpIds.isEmpty() && m_helpUrl.isEmpty(); }
    bool isValid() const { return !m_helpIds.isEmpty() || !m_helpUrl.isEmpty(); }

private:
    QUrl m_helpUrl;
    QStringList m_helpIds;
    QString m_docMark;
    Category m_category = Unknown;
};

} // namespace Core

Q_DECLARE_METATYPE(Core::HelpItem)
