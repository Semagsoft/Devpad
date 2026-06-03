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
#include "tabmanager.h"
#include "appstrings.h"
#include "codeeditor.h"
#include "projectpanel.h"
#include "splitview.h"
#include <QDir>
#include <QFile>
#include <QTabWidget>

SessionManager::SessionManager(QObject *parent)
    : QObject(parent),
      m_sessionSettings(QStringLiteral("Semagsoft"), QStringLiteral("Devpad"))
{
}

SessionManager::SessionData SessionManager::loadSessionData() const {
    SessionData data;
    data.files = sessionFiles();
    data.activeIndex = sessionActiveIndex();
    data.projectPath = sessionProjectPath();
    data.bookmarks = loadSessionBookmarks();
    return data;
}

void SessionManager::restoreSession(
    std::function<void(const QString &filePath)> loadFileFn,
    SplitView *splitView,
    ProjectPanel *projectPanel
) {
    SessionData data = loadSessionData();

    if (!data.projectPath.isEmpty() && QDir(data.projectPath).exists()) {
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
    m_sessionSettings.setValue(QStringLiteral("Session_Files"), files);
    m_sessionSettings.setValue(QStringLiteral("Session_ActiveIndex"), activeIndex);
    m_sessionSettings.setValue(QStringLiteral("Session_ProjectPath"), projectPath);
    m_sessionSettings.setValue(QStringLiteral("Session_HasSession"), true);
}

QStringList SessionManager::sessionFiles() const
{
    return m_sessionSettings.value(QStringLiteral("Session_Files")).toStringList();
}

int SessionManager::sessionActiveIndex() const
{
    return m_sessionSettings.value(QStringLiteral("Session_ActiveIndex"), 0).toInt();
}

QString SessionManager::sessionProjectPath() const
{
    return m_sessionSettings.value(QStringLiteral("Session_ProjectPath")).toString();
}

void SessionManager::saveSessionBookmarks(const QHash<QString, QList<int>> &bookmarks)
{
    QVariantMap map;
    for (auto it = bookmarks.constBegin(); it != bookmarks.constEnd(); ++it)
    {
        QVariantList lines;
        for (int line : it.value())
            lines.append(line);
        map.insert(it.key(), lines);
    }
    m_sessionSettings.setValue(QStringLiteral("Session_Bookmarks"), map);
}

QHash<QString, QList<int>> SessionManager::loadSessionBookmarks() const
{
    QHash<QString, QList<int>> result;
    QVariantMap map = m_sessionSettings.value(QStringLiteral("Session_Bookmarks")).toMap();
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

void SessionManager::clearSession()
{
    m_sessionSettings.remove(QStringLiteral("Session_Files"));
    m_sessionSettings.remove(QStringLiteral("Session_ActiveIndex"));
    m_sessionSettings.remove(QStringLiteral("Session_ProjectPath"));
    m_sessionSettings.remove(QStringLiteral("Session_HasSession"));
    m_sessionSettings.remove(QStringLiteral("Session_Bookmarks"));
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
}
