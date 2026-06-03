// Copyright (C) 2026 MyIDE
// Derived from Qt Creator (The Qt Company Ltd.) — original copyright preserved below.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Original header (Qt Creator):
/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "idocument.h"

#include <utils/fileutils.h>
#include <utils/infobar.h>
#include <utils/minimizableinfobars.h>
#include <utils/optional.h>
#include <utils/qtcassert.h>

#include <QFile>
#include <QFileInfo>

#include <memory>

using namespace Utils;

namespace Core {
namespace Internal {

class IDocumentPrivate
{
public:
    ~IDocumentPrivate()
    {
        delete infoBar;
    }

    QString mimeType;
    Utils::FilePath filePath;
    QString preferredDisplayName;
    QString uniqueDisplayName;
    Utils::FilePath autoSavePath;
    Utils::InfoBar *infoBar = nullptr;
    std::unique_ptr<MinimizableInfoBars> minimizableInfoBars;
    Id id;
    optional<bool> fileIsReadOnly;
    bool temporary = false;
    bool hasWriteWarning = false;
    bool restored = false;
    bool isSuspendAllowed = false;
};

} // namespace Internal

IDocument::IDocument(QObject *parent) : QObject(parent),
    d(new Internal::IDocumentPrivate)
{
}

IDocument::~IDocument()
{
    removeAutoSaveFile();
    delete d;
}

void IDocument::setId(Id id)
{
    d->id = id;
}

Id IDocument::id() const
{
    QTC_CHECK(d->id.isValid());
    return d->id;
}

IDocument::OpenResult IDocument::open(QString *errorString, const Utils::FilePath &filePath, const Utils::FilePath &realFilePath)
{
    Q_UNUSED(errorString)
    Q_UNUSED(filePath)
    Q_UNUSED(realFilePath)
    return OpenResult::CannotHandle;
}

bool IDocument::save(QString *errorString, const Utils::FilePath &filePath, bool autoSave)
{
    Q_UNUSED(errorString)
    Q_UNUSED(filePath)
    Q_UNUSED(autoSave)
    return false;
}

QByteArray IDocument::contents() const
{
    return QByteArray();
}

bool IDocument::setContents(const QByteArray &contents)
{
    Q_UNUSED(contents)
    return false;
}

const Utils::FilePath &IDocument::filePath() const
{
    return d->filePath;
}

IDocument::ReloadBehavior IDocument::reloadBehavior(ChangeTrigger trigger, ChangeType type) const
{
    if (type == TypeContents && trigger == TriggerInternal && !isModified())
        return BehaviorSilent;
    return BehaviorAsk;
}

bool IDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}

void IDocument::checkPermissions()
{
    bool previousReadOnly = d->fileIsReadOnly.value_or(false);
    if (!filePath().isEmpty()) {
        d->fileIsReadOnly = !filePath().isWritableFile();
    } else {
        d->fileIsReadOnly = false;
    }
    if (previousReadOnly != *(d->fileIsReadOnly))
        emit changed();
}

bool IDocument::shouldAutoSave() const
{
    return false;
}

bool IDocument::isModified() const
{
    return false;
}

bool IDocument::isSaveAsAllowed() const
{
    return false;
}

bool IDocument::isSuspendAllowed() const
{
    return d->isSuspendAllowed;
}

void IDocument::setSuspendAllowed(bool value)
{
    d->isSuspendAllowed = value;
}

bool IDocument::isFileReadOnly() const
{
    if (filePath().isEmpty())
        return false;
    if (!d->fileIsReadOnly)
        const_cast<IDocument *>(this)->checkPermissions();
    return d->fileIsReadOnly.value_or(false);
}

bool IDocument::isTemporary() const
{
    return d->temporary;
}

void IDocument::setTemporary(bool temporary)
{
    d->temporary = temporary;
}

FilePath IDocument::fallbackSaveAsPath() const
{
    return {};
}

QString IDocument::fallbackSaveAsFileName() const
{
    return QString();
}

QString IDocument::mimeType() const
{
    return d->mimeType;
}

void IDocument::setMimeType(const QString &mimeType)
{
    if (d->mimeType != mimeType) {
        d->mimeType = mimeType;
        emit mimeTypeChanged();
    }
}

bool IDocument::autoSave(QString *errorString, const FilePath &filePath)
{
    if (!save(errorString, filePath, true))
        return false;
    d->autoSavePath = filePath;
    return true;
}

static const char kRestoredAutoSave[] = "RestoredAutoSave";

void IDocument::setRestoredFrom(const Utils::FilePath &path)
{
    d->autoSavePath = path;
    d->restored = true;
    Utils::InfoBarEntry info(Id(kRestoredAutoSave),
                             tr("File was restored from auto-saved copy. "
                                "Select Save to confirm or Revert to Saved to discard changes."));
    infoBar()->addInfo(info);
}

void IDocument::removeAutoSaveFile()
{
    if (!d->autoSavePath.isEmpty()) {
        QFile::remove(d->autoSavePath.toString());
        d->autoSavePath.clear();
        if (d->restored) {
            d->restored = false;
            infoBar()->removeInfo(Id(kRestoredAutoSave));
        }
    }
}

bool IDocument::hasWriteWarning() const
{
    return d->hasWriteWarning;
}

void IDocument::setWriteWarning(bool has)
{
    d->hasWriteWarning = has;
}

Utils::InfoBar *IDocument::infoBar()
{
    if (!d->infoBar)
        d->infoBar = new Utils::InfoBar;
    return d->infoBar;
}

MinimizableInfoBars *IDocument::minimizableInfoBars()
{
    if (!d->minimizableInfoBars)
        d->minimizableInfoBars.reset(new Utils::MinimizableInfoBars(*infoBar()));
    return d->minimizableInfoBars.get();
}

void IDocument::setFilePath(const Utils::FilePath &filePath)
{
    if (d->filePath == filePath)
        return;
    Utils::FilePath oldName = d->filePath;
    d->filePath = filePath;
    emit filePathChanged(oldName, d->filePath);
    emit changed();
}

QString IDocument::displayName() const
{
    return d->uniqueDisplayName.isEmpty() ? plainDisplayName() : d->uniqueDisplayName;
}

void IDocument::setPreferredDisplayName(const QString &name)
{
    if (name == d->preferredDisplayName)
        return;
    d->preferredDisplayName = name;
    emit changed();
}

QString IDocument::preferredDisplayName() const
{
    return d->preferredDisplayName;
}

QString IDocument::plainDisplayName() const
{
    return d->preferredDisplayName.isEmpty() ? d->filePath.fileName() : d->preferredDisplayName;
}

void IDocument::setUniqueDisplayName(const QString &name)
{
    d->uniqueDisplayName = name;
}

QString IDocument::uniqueDisplayName() const
{
    return d->uniqueDisplayName;
}

} // namespace Core
