/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "sessionmanager.h"
#include "logger.h"
#include "tabmanager.h"
#include "appstrings.h"
#include "codeeditor.h"
#include "projectpanel.h"
#include "splitview.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTabWidget>

SessionManager::SessionManager(QObject *parent)
    : QObject(parent),
      m_sessionSettings(QStringLiteral("Semagsoft"), QStringLiteral("Devpad")),
      m_sessionLock(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                    + QStringLiteral("/devpad/session.lock"))
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/devpad"));
    m_sessionLock.setStaleLockTime(0);
}

QString SessionManager::sessionGroupPrefix() const
{
    return QStringLiteral("Session_%1").arg(QCoreApplication::applicationPid());
}

SessionManager::SessionData SessionManager::loadSessionData() {
    SessionData data;
    QStringList allFiles;
    QHash<QString, QList<int>> allBookmarks;
    int totalOffset = 0;

    for (const QString& group : m_sessionSettings.childGroups()) {
        if (!group.startsWith(QStringLiteral("Session_")))
            continue;

        m_sessionSettings.beginGroup(group);
        QStringList files = m_sessionSettings.value(QStringLiteral("Files")).toStringList();
        int activeIndex = m_sessionSettings.value(QStringLiteral("ActiveIndex"), 0).toInt();
        QString projectPath = m_sessionSettings.value(QStringLiteral("ProjectPath")).toString();
        QVariantMap bookmarkMap = m_sessionSettings.value(QStringLiteral("Bookmarks")).toMap();
        m_sessionSettings.endGroup();

        if (data.projectPath.isEmpty() && !projectPath.isEmpty())
            data.projectPath = projectPath;

        for (auto it = bookmarkMap.constBegin(); it != bookmarkMap.constEnd(); ++it) {
            QList<int> lines;
            for (const QVariant &v : it.value().toList())
                lines.append(v.toInt());
            if (!lines.isEmpty())
                allBookmarks.insert(it.key(), lines);
        }

        data.activeIndex = totalOffset + qMin(activeIndex, qMax(files.size() - 1, 0));
        allFiles.append(files);
        totalOffset += files.size();
    }

    data.files = allFiles;
    data.bookmarks = allBookmarks;
    return data;
}

void SessionManager::restoreSession(
    std::function<void(const QString &filePath)> loadFileFn,
    SplitView *splitView,
    ProjectPanel *projectPanel
) {
    SessionData data = loadSessionData();

    if (!data.projectPath.isEmpty() && QDir::cleanPath(data.projectPath) != QDir::currentPath() && QDir(data.projectPath).exists()) {
        projectPanel->setRootPath(data.projectPath);
        projectPanel->show();
    }

    if (data.files.isEmpty()) {
        return;
    }

    int targetIndex = 0;
    int loadedCount = 0;
    for (int fi = 0; fi < data.files.size(); ++fi) {
        if (QFile::exists(data.files[fi])) {
            loadFileFn(data.files[fi]);
            if (fi <= data.activeIndex) {
                targetIndex = loadedCount;
            }
            loadedCount++;
        }
    }

    int editorCount = 0;
    for (int p = 0; p < splitView->paneCount(); ++p) {
        QTabWidget *pane = splitView->paneAt(p);
        for (int i = 0; i < pane->count(); ++i) {
            auto *editor = qobject_cast<CodeEditor*>(pane->widget(i));
            if (editor) {
                auto it = data.bookmarks.constFind(editor->fileName());
                if (it != data.bookmarks.constEnd()) {
                    editor->setBookmarks(it.value());
                }
                if (editorCount == targetIndex) {
                    pane->setCurrentIndex(i);
                }
                editorCount++;
            }
        }
    }

    emit sessionRestored();
}

void SessionManager::saveSessionData(const QStringList &files, int activeIndex, const QString &projectPath)
{
    if (!m_sessionLock.tryLock(5000)) {
        Logger::instance().error("Failed to acquire session lock for saveSessionData");
        return;
    }

    QSettings settings(QStringLiteral("Semagsoft"), QStringLiteral("Devpad"));
    settings.beginGroup(sessionGroupPrefix());
    settings.setValue(QStringLiteral("Files"), files);
    settings.setValue(QStringLiteral("ActiveIndex"), activeIndex);
    settings.setValue(QStringLiteral("ProjectPath"), projectPath);
    settings.setValue(QStringLiteral("HasSession"), true);
    settings.endGroup();
    settings.sync();

    m_sessionLock.unlock();
}

QStringList SessionManager::sessionFiles()
{
    QString group = sessionGroupPrefix();
    if (m_sessionSettings.childGroups().contains(group)) {
        m_sessionSettings.beginGroup(group);
        QStringList files = m_sessionSettings.value(QStringLiteral("Files")).toStringList();
        m_sessionSettings.endGroup();
        if (!files.isEmpty())
            return files;
    }
    return m_sessionSettings.value(QStringLiteral("Session_Files")).toStringList();
}

int SessionManager::sessionActiveIndex()
{
    QString group = sessionGroupPrefix();
    if (m_sessionSettings.childGroups().contains(group)) {
        m_sessionSettings.beginGroup(group);
        int idx = m_sessionSettings.value(QStringLiteral("ActiveIndex"), 0).toInt();
        m_sessionSettings.endGroup();
        return idx;
    }
    return m_sessionSettings.value(QStringLiteral("Session_ActiveIndex"), 0).toInt();
}

QString SessionManager::sessionProjectPath()
{
    QString group = sessionGroupPrefix();
    if (m_sessionSettings.childGroups().contains(group)) {
        m_sessionSettings.beginGroup(group);
        QString path = m_sessionSettings.value(QStringLiteral("ProjectPath")).toString();
        m_sessionSettings.endGroup();
        return path;
    }
    return m_sessionSettings.value(QStringLiteral("Session_ProjectPath")).toString();
}

void SessionManager::saveSessionBookmarks(const QHash<QString, QList<int>> &bookmarks)
{
    if (!m_sessionLock.tryLock(5000)) {
        Logger::instance().error("Failed to acquire session lock for saveSessionBookmarks");
        return;
    }

    QSettings settings(QStringLiteral("Semagsoft"), QStringLiteral("Devpad"));
    QVariantMap map;
    for (auto it = bookmarks.constBegin(); it != bookmarks.constEnd(); ++it)
    {
        QVariantList lines;
        for (int line : it.value())
            lines.append(line);
        map.insert(it.key(), lines);
    }
    settings.beginGroup(sessionGroupPrefix());
    settings.setValue(QStringLiteral("Bookmarks"), map);
    settings.endGroup();
    settings.sync();

    m_sessionLock.unlock();
}

QHash<QString, QList<int>> SessionManager::loadSessionBookmarks()
{
    QHash<QString, QList<int>> result;
    QVariantMap map;
    QString group = sessionGroupPrefix();
    if (m_sessionSettings.childGroups().contains(group)) {
        m_sessionSettings.beginGroup(group);
        map = m_sessionSettings.value(QStringLiteral("Bookmarks")).toMap();
        m_sessionSettings.endGroup();
    }
    if (map.isEmpty())
        map = m_sessionSettings.value(QStringLiteral("Session_Bookmarks")).toMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
    {
        QList<int> lines;
        for (const QVariant &v : it.value().toList())
            lines.append(v.toInt());
        if (!lines.isEmpty())
            result.insert(it.key(), lines);
    }
    return result;
}

void SessionManager::saveSessionPinnedFiles(const QStringList &pinnedFiles)
{
    if (!m_sessionLock.tryLock(5000)) {
        Logger::instance().error("Failed to acquire session lock for saveSessionPinnedFiles");
        return;
    }

    QSettings settings(QStringLiteral("Semagsoft"), QStringLiteral("Devpad"));
    settings.beginGroup(sessionGroupPrefix());
    settings.setValue(QStringLiteral("PinnedFiles"), pinnedFiles);
    settings.endGroup();
    settings.sync();

    m_sessionLock.unlock();
}

QStringList SessionManager::loadSessionPinnedFiles()
{
    QStringList files;
    QString group = sessionGroupPrefix();
    if (m_sessionSettings.childGroups().contains(group)) {
        m_sessionSettings.beginGroup(group);
        files = m_sessionSettings.value(QStringLiteral("PinnedFiles")).toStringList();
        m_sessionSettings.endGroup();
    }
    if (files.isEmpty())
        files = m_sessionSettings.value(QStringLiteral("Session_PinnedFiles")).toStringList();
    return files;
}

void SessionManager::clearSession()
{
    if (!m_sessionLock.tryLock(5000)) {
        Logger::instance().error("Failed to acquire session lock for clearSession");
        return;
    }

    QString group = sessionGroupPrefix();

    for (const QString &g : m_sessionSettings.childGroups()) {
        if (g.startsWith(QStringLiteral("Session_"))) {
            m_sessionSettings.remove(g);
        }
    }

    m_sessionSettings.remove(QStringLiteral("Session_Files"));
    m_sessionSettings.remove(QStringLiteral("Session_ActiveIndex"));
    m_sessionSettings.remove(QStringLiteral("Session_ProjectPath"));
    m_sessionSettings.remove(QStringLiteral("Session_Bookmarks"));
    m_sessionSettings.remove(QStringLiteral("Session_PinnedFiles"));
    m_sessionSettings.remove(QStringLiteral("HasSession"));
    m_sessionSettings.sync();

    m_sessionLock.unlock();
}

void SessionManager::saveSession(
    TabManager *tabManager,
    ProjectPanel *projectPanel
) {
    QStringList files;
    int activeIndex = 0;
    QTabWidget *currentPane = tabManager->activePane();
    for (QTabWidget *pane : tabManager->panes()) {
        for (int i = 0; i < pane->count(); ++i) {
            auto *editor = qobject_cast<CodeEditor*>(pane->widget(i));
            if (!editor) continue;
            QString fileName = editor->fileName();
            if (fileName == Strings::untitled() || fileName.isEmpty()) {
                continue;
            }
            if (pane == currentPane && i == pane->currentIndex()) {
                activeIndex = files.size();
            }
            files.append(fileName);
        }
    }

    QHash<QString, QList<int>> bookmarks;
    for (QTabWidget *pane : tabManager->panes()) {
        for (int i = 0; i < pane->count(); ++i) {
            auto *editor = qobject_cast<CodeEditor*>(pane->widget(i));
            if (editor && editor->fileName() != Strings::untitled() && !editor->fileName().isEmpty()) {
                QList<int> lines = editor->bookmarkLines();
                if (!lines.isEmpty()) {
                    bookmarks.insert(editor->fileName(), lines);
                }
            }
        }
    }

    QString projectPath = projectPanel->isVisible() ? projectPanel->rootPath() : QString();
    saveSessionData(files, activeIndex, projectPath);
    saveSessionBookmarks(bookmarks);
    saveSessionPinnedFiles(tabManager->pinnedFiles());
}
