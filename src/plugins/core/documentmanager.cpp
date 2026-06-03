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

#include "documentmanager.h"

#include "idocument.h"
#include "idocumentfactory.h"
#include "coreconstants.h"
#include "icore.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/globalfilechangeblocker.h>
#include <utils/hostosinfo.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QLoggingCategory>
#include <QSettings>
#include <QStringList>
#include <QTimer>

// PR-1 阶段的 Core 还没接入 EditorManager. PR-2 步骤3 的目标是先把数据流
// 跑通 (add/remove/track modified/save silent/recent files), UI 弹窗和
// 重新载入对话框依赖 EditorManager/IDocument 相关视图, 留到 PR-3 接入.

static const bool kUseProjectsDirectoryDefault = true;
static Q_LOGGING_CATEGORY(log, "qtc.core.documentmanager", QtWarningMsg)

static const char settingsGroupC[] = "RecentFiles";
static const char filesKeyC[] = "Files";
static const char editorsKeyC[] = "EditorIds";

static const char directoryGroupC[] = "Directories";
static const char projectDirectoryKeyC[] = "Projects";
static const char useProjectDirectoryKeyC[] = "UseProjectsDirectory";

using namespace Utils;

namespace Core {

namespace Internal {

struct FileStateItem
{
    QDateTime modified;
    QFile::Permissions permissions;
};

struct FileState
{
    FilePath watchedFilePath;
    QMap<IDocument *, FileStateItem> lastUpdatedState;
    FileStateItem expected;
};

class DocumentManagerPrivate : public QObject
{
    Q_OBJECT
public:
    DocumentManagerPrivate();
    QFileSystemWatcher *fileWatcher();

    QMap<FilePath, FileState> m_states; // filePathKey -> FileState
    QList<IDocument *> m_documentsWithoutWatch;
    QMap<IDocument *, FilePaths> m_documentsWithWatch; // document -> list of filePathKeys
    QSet<FilePath> m_expectedFileNames;

    QList<DocumentManager::RecentFile> m_recentFiles;

    bool m_postponeAutoReload = false;
    bool m_useProjectsDirectory = kUseProjectsDirectoryDefault;

    QFileSystemWatcher *m_fileWatcher = nullptr; // Delayed creation.
    FilePath m_lastVisitedDirectory = FilePath::fromString(QDir::currentPath());
    FilePath m_defaultLocationForNewFiles;
    FilePath m_projectsDirectory;
    IDocument *m_blockedIDocument = nullptr;

    QAction *m_saveAllAction = nullptr;
    QString fileDialogFilterOverride;
};

DocumentManager *m_instance = nullptr;
DocumentManagerPrivate *d = nullptr;

QFileSystemWatcher *DocumentManagerPrivate::fileWatcher()
{
    if (!m_fileWatcher) {
        m_fileWatcher = new QFileSystemWatcher(Internal::m_instance);
    }
    return m_fileWatcher;
}

DocumentManagerPrivate::DocumentManagerPrivate()
{
    // PR-1 阶段: 不在构造时创建 QAction, 等 EditorManager 接入后再加 (PR-3).
}

} // namespace Internal

} // namespace Core

// Re-open namespaces and put moc include inside Core::Internal so that the
// unqualified DocumentManagerPrivate reference inside the generated moc
// resolves to Core::Internal::DocumentManagerPrivate.
namespace Core {
namespace Internal {

#include "documentmanager.moc"

} // namespace Internal
} // namespace Core

namespace Core {

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

static void readSettings()
{
    QSettings *qs = ICore::settings();

    qs->beginGroup(QLatin1String(settingsGroupC));
    Internal::d->m_recentFiles.clear();
    int num = qs->beginReadArray(QLatin1String(filesKeyC));
    QList<DocumentManager::RecentFile> files;
    for (int i = 0; i < num; ++i) {
        qs->setArrayIndex(i);
        files.append({FilePath::fromString(qs->value(QLatin1String(filesKeyC)).toString()),
                      Id::fromSetting(qs->value(QLatin1String(editorsKeyC)))});
    }
    qs->endArray();
    Internal::d->m_recentFiles = files;
    qs->endGroup();

    qs->beginGroup(QLatin1String(directoryGroupC));
    Internal::d->m_projectsDirectory = FilePath::fromString(qs->value(QLatin1String(projectDirectoryKeyC),
                                                             QDir::currentPath()).toString());
    Internal::d->m_useProjectsDirectory = qs->value(QLatin1String(useProjectDirectoryKeyC),
                                          kUseProjectsDirectoryDefault).toBool();
    qs->endGroup();
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

DocumentManager::DocumentManager(QObject *parent)
    : QObject(parent)
{
    Internal::d = new Internal::DocumentManagerPrivate;
    Internal::m_instance = this;

    connect(Utils::GlobalFileChangeBlocker::instance(),
            &Utils::GlobalFileChangeBlocker::stateChanged,
            this, [](bool blocked) {
                // PR-1 阶段没有 EditorManager::checkForReload, 留待 PR-3.
                Q_UNUSED(blocked);
            });

    readSettings();

    if (Internal::d->m_useProjectsDirectory)
        setFileDialogLastVisitedDirectory(Internal::d->m_projectsDirectory);
}

DocumentManager::~DocumentManager()
{
    delete Internal::d;
    Internal::d = nullptr;
    Internal::m_instance = nullptr;
}

DocumentManager *DocumentManager::instance()
{
    return Internal::m_instance;
}

// ---------------------------------------------------------------------------
// File pool
// ---------------------------------------------------------------------------

static FilePath filePathKey(const FilePath &filePath, DocumentManager::ResolveMode resolveMode)
{
    if (resolveMode == DocumentManager::KeepLinks)
        return filePath;
    return filePath.canonicalPath();
}

FilePath DocumentManager::filePathKey(const FilePath &filePath, ResolveMode resolveMode)
{
    return Core::filePathKey(filePath, resolveMode);
}

void DocumentManager::addDocuments(const QList<IDocument *> &documents, bool addWatcher)
{
    for (IDocument *doc : documents)
        addDocument(doc, addWatcher);
}

void DocumentManager::addDocument(IDocument *document, bool addWatcher)
{
    if (!document)
        return;
    QTC_ASSERT(document->id().isValid(), return);

    // track lifetime
    connect(document, &QObject::destroyed, Internal::m_instance, &DocumentManager::documentDestroyed);
    connect(document, &IDocument::filePathChanged,
            Internal::m_instance, [](const FilePath &oldName, const FilePath &newName) {
                if (Internal::d->m_blockedIDocument)
                    return;
                const bool wasWatched = Internal::d->m_documentsWithWatch.contains(Internal::d->m_blockedIDocument);
                Q_UNUSED(wasWatched);
                // EditorManager::updateWindowTitles; PR-3
                Q_UNUSED(oldName);
                Q_UNUSED(newName);
            });
    connect(document, &IDocument::changed,
            Internal::m_instance, []() {
                emit Internal::m_instance->filesChangedExternally({});
            });
    connect(document, &IDocument::contentsChanged,
            Internal::m_instance, []() {
                // For now nothing — the EditorManager observes this for save state
            });

    if (addWatcher) {
        if (!Internal::d->m_documentsWithWatch.contains(document)) {
            const FilePaths paths = {filePathKey(document->filePath(), ResolveLinks)};
            Internal::d->m_documentsWithWatch.insert(document, paths);
        }
    } else {
        Internal::d->m_documentsWithoutWatch.append(document);
    }
}

bool DocumentManager::removeDocument(IDocument *document)
{
    if (!document)
        return false;
    QObject::disconnect(document, nullptr, Internal::m_instance, nullptr);
    if (Internal::d->m_documentsWithWatch.contains(document)) {
        QFileSystemWatcher *watcher = Internal::d->fileWatcher();
        if (watcher) {
            const FilePaths paths = Internal::d->m_documentsWithWatch.value(document);
            for (const FilePath &fp : paths)
                watcher->removePath(fp.toString());
        }
        Internal::d->m_documentsWithWatch.remove(document);
    }
    Internal::d->m_documentsWithoutWatch.removeAll(document);
    return true;
}

QList<IDocument *> DocumentManager::modifiedDocuments()
{
    QList<IDocument *> modified;
    auto isModified = [&modified](IDocument *doc) {
        if (doc->isModified() && !doc->isTemporary())
            modified << doc;
    };
    for (auto it = Internal::d->m_documentsWithWatch.cbegin(); it != Internal::d->m_documentsWithWatch.cend(); ++it)
        isModified(it.key());
    for (IDocument *doc : std::as_const(Internal::d->m_documentsWithoutWatch))
        isModified(doc);
    return modified;
}

void DocumentManager::renamedFile(const Utils::FilePath &from, const Utils::FilePath &to)
{
    Q_UNUSED(from)
    Q_UNUSED(to)
    // EditorManager handles editor state on rename. PR-3.
}

void DocumentManager::documentDestroyed(QObject *obj)
{
    auto *doc = qobject_cast<IDocument *>(obj);
    if (!doc)
        return;
    Internal::d->m_documentsWithWatch.remove(doc);
    Internal::d->m_documentsWithoutWatch.removeAll(doc);
}

void DocumentManager::expectFileChange(const Utils::FilePath &filePath)
{
    Internal::d->m_expectedFileNames.insert(filePath);
}

void DocumentManager::unexpectFileChange(const Utils::FilePath &filePath)
{
    Internal::d->m_expectedFileNames.remove(filePath);
}

// ---------------------------------------------------------------------------
// Recent files
// ---------------------------------------------------------------------------

void DocumentManager::addToRecentFiles(const Utils::FilePath &filePath, Utils::Id editorId)
{
    if (filePath.isEmpty())
        return;
    DocumentManager::RecentFile f { filePath, editorId };
    Internal::d->m_recentFiles.removeAll(f);
    Internal::d->m_recentFiles.prepend(f);
    while (Internal::d->m_recentFiles.size() > 25)  // 硬编码上限; PR-3 接 EditorManager 时换 maxRecentFiles()
        Internal::d->m_recentFiles.removeLast();
    emit Internal::m_instance->filesChangedExternally({});
}

void DocumentManager::clearRecentFiles()
{
    Internal::d->m_recentFiles.clear();
}

QList<DocumentManager::RecentFile> DocumentManager::recentFiles()
{
    return Internal::d->m_recentFiles;
}

void DocumentManager::saveSettings()
{
    QSettings *qs = ICore::settings();
    qs->beginGroup(QLatin1String(settingsGroupC));
    qs->beginWriteArray(QLatin1String(filesKeyC), Internal::d->m_recentFiles.size());
    for (int i = 0; i < Internal::d->m_recentFiles.size(); ++i) {
        qs->setArrayIndex(i);
        qs->setValue(QLatin1String(filesKeyC), Internal::d->m_recentFiles.at(i).first.toVariant());
        qs->setValue(QLatin1String(editorsKeyC), Internal::d->m_recentFiles.at(i).second.toSetting());
    }
    qs->endArray();
    qs->endGroup();

    qs->beginGroup(QLatin1String(directoryGroupC));
    qs->setValue(QLatin1String(projectDirectoryKeyC), Internal::d->m_projectsDirectory.toString());
    qs->setValue(QLatin1String(useProjectDirectoryKeyC), Internal::d->m_useProjectsDirectory);
    qs->endGroup();
}

// ---------------------------------------------------------------------------
// Save
// ---------------------------------------------------------------------------

static bool saveDocumentHelper(IDocument *document, const FilePath &filePath, bool *isReadOnly)
{
    if (!document)
        return false;
    document->checkPermissions();

    // Real save path: PR-3 will plug in EditorManagerPrivate::saveDocument which
    // handles EditorArea activation + autosave stash. For PR-2 we just call
    // IDocument::save and surface read-only state.
    QString error;
    bool ok = document->save(&error, filePath, /*autoSave=*/false);
    if (isReadOnly)
        *isReadOnly = document->isFileReadOnly();
    return ok;
}

bool DocumentManager::saveDocument(IDocument *document,
                                   const Utils::FilePath &filePath,
                                   bool *isReadOnly)
{
    return saveDocumentHelper(document, filePath, isReadOnly);
}

static bool saveModifiedFilesHelper(const QList<IDocument *> &documents,
                                    const QString &message,
                                    bool *cancelled, bool silently,
                                    const QString &alwaysSaveMessage,
                                    bool *alwaysSave, QList<IDocument *> *failedToSave)
{
    Q_UNUSED(message)
    Q_UNUSED(alwaysSaveMessage)
    if (cancelled)
        *cancelled = false;
    if (alwaysSave)
        *alwaysSave = false;
    if (failedToSave)
        failedToSave->clear();

    bool allOk = true;
    for (IDocument *doc : documents) {
        if (doc->isModified() && !doc->isTemporary()) {
            QString error;
            bool ok = false;
            if (silently) {
                ok = doc->save(&error);
            } else {
                ok = saveDocumentHelper(doc, doc->filePath(), nullptr);
            }
            if (!ok) {
                allOk = false;
                if (failedToSave)
                    failedToSave->append(doc);
            }
        }
    }
    return allOk;
}

bool DocumentManager::saveModifiedDocumentSilently(IDocument *document, bool *canceled,
                                                   QList<IDocument *> *failedToClose)
{
    return saveModifiedFilesHelper({document}, {}, canceled, true, {}, nullptr, failedToClose);
}

bool DocumentManager::saveModifiedDocumentsSilently(const QList<IDocument *> &documents,
                                                    bool *canceled,
                                                    QList<IDocument *> *failedToClose)
{
    return saveModifiedFilesHelper(documents, {}, canceled, true, {}, nullptr, failedToClose);
}

bool DocumentManager::saveAllModifiedDocumentsSilently(bool *canceled,
                                                       QList<IDocument *> *failedToClose)
{
    return saveModifiedFilesHelper(modifiedDocuments(), {}, canceled, true, {}, nullptr, failedToClose);
}

bool DocumentManager::saveAllModifiedDocuments(const QString &message, bool *canceled,
                                               const QString &alwaysSaveMessage, bool *alwaysSave,
                                               QList<IDocument *> *failedToClose)
{
    return saveModifiedFilesHelper(modifiedDocuments(), message, canceled, false,
                                    alwaysSaveMessage, alwaysSave, failedToClose);
}

bool DocumentManager::saveModifiedDocuments(const QList<IDocument *> &documents,
                                            const QString &message, bool *canceled,
                                            const QString &alwaysSaveMessage, bool *alwaysSave,
                                            QList<IDocument *> *failedToClose)
{
    return saveModifiedFilesHelper(documents, message, canceled, false,
                                    alwaysSaveMessage, alwaysSave, failedToClose);
}

bool DocumentManager::saveModifiedDocument(IDocument *document, const QString &message,
                                           bool *canceled, const QString &alwaysSaveMessage,
                                           bool *alwaysSave, QList<IDocument *> *failedToClose)
{
    return saveModifiedFilesHelper({document}, message, canceled, false,
                                    alwaysSaveMessage, alwaysSave, failedToClose);
}

// ---------------------------------------------------------------------------
// File dialog helpers (UI dialogs are PR-3 work; we expose the data primitives)
// ---------------------------------------------------------------------------

QString DocumentManager::allDocumentFactoryFiltersString(QString *allFilesFilter)
{
    QStringList filters;
    if (allFilesFilter)
        *allFilesFilter = QObject::tr("All Files (*)");
    for (IDocumentFactory *factory : IDocumentFactory::allDocumentFactories()) {
        const QStringList mts = factory->mimeTypes();
        for (const QString &mt : mts)
            filters.append(mt);
    }
    filters.removeDuplicates();
    return filters.join(QStringLiteral(";;"));
}

Utils::FilePaths DocumentManager::getOpenFileNames(const QString &filters,
                                                   const Utils::FilePath &path,
                                                   QString *selectedFilter)
{
    return Utils::FilePaths{path};
}

Utils::FilePath DocumentManager::getSaveFileName(const QString &title,
                                                 const Utils::FilePath &pathIn,
                                                 const QString &filter,
                                                 QString *selectedFilter)
{
    Q_UNUSED(title)
    Q_UNUSED(filter)
    Q_UNUSED(selectedFilter)
    return pathIn;
}

Utils::FilePath DocumentManager::getSaveFileNameWithExtension(const QString &title,
                                                              const Utils::FilePath &pathIn,
                                                              const QString &filter)
{
    return getSaveFileName(title, pathIn, filter);
}

Utils::FilePath DocumentManager::getSaveAsFileName(const IDocument *document)
{
    if (document)
        return document->filePath();
    return {};
}

void DocumentManager::showFilePropertiesDialog(const Utils::FilePath &filePath)
{
    Q_UNUSED(filePath)
    // PR-3: open the FilePropertiesDialog.
}

Utils::FilePath DocumentManager::fileDialogLastVisitedDirectory()
{
    return Internal::d->m_lastVisitedDirectory;
}

void DocumentManager::setFileDialogLastVisitedDirectory(const Utils::FilePath &dir)
{
    Internal::d->m_lastVisitedDirectory = dir;
}

Utils::FilePath DocumentManager::fileDialogInitialDirectory()
{
    if (Internal::d->m_useProjectsDirectory)
        return Internal::d->m_projectsDirectory;
    return Internal::d->m_lastVisitedDirectory;
}

Utils::FilePath DocumentManager::defaultLocationForNewFiles()
{
    return Internal::d->m_defaultLocationForNewFiles;
}

void DocumentManager::setDefaultLocationForNewFiles(const Utils::FilePath &location)
{
    Internal::d->m_defaultLocationForNewFiles = location;
}

bool DocumentManager::useProjectsDirectory()
{
    return Internal::d->m_useProjectsDirectory;
}

void DocumentManager::setUseProjectsDirectory(bool use)
{
    Internal::d->m_useProjectsDirectory = use;
}

Utils::FilePath DocumentManager::projectsDirectory()
{
    return Internal::d->m_projectsDirectory;
}

void DocumentManager::setProjectsDirectory(const Utils::FilePath &directory)
{
    if (Internal::d->m_projectsDirectory == directory)
        return;
    Internal::d->m_projectsDirectory = directory;
    emit Internal::m_instance->projectsDirectoryChanged(directory);
}

void DocumentManager::notifyFilesChangedInternally(const Utils::FilePaths &filePaths)
{
    emit Internal::m_instance->filesChangedInternally(filePaths);
}

void DocumentManager::setFileDialogFilter(const QString &filter)
{
    Internal::d->fileDialogFilterOverride = filter;
}

QString DocumentManager::fileDialogFilter(QString *selectedFilter)
{
    if (selectedFilter)
        *selectedFilter = Internal::d->fileDialogFilterOverride;
    return Internal::d->fileDialogFilterOverride;
}

QString DocumentManager::allFilesFilterString()
{
    return QObject::tr("All Files (*)");
}

// ---------------------------------------------------------------------------
// Internals
// ---------------------------------------------------------------------------

} // namespace Core (using Internal)

// ---------------------------------------------------------------------------
// FileChangeBlocker
// ---------------------------------------------------------------------------

namespace Core {

FileChangeBlocker::FileChangeBlocker(const FilePath &filePath)
    : m_filePath(filePath)
{
    DocumentManager::expectFileChange(filePath);
}

FileChangeBlocker::~FileChangeBlocker()
{
    DocumentManager::unexpectFileChange(m_filePath);
}

} // namespace Core