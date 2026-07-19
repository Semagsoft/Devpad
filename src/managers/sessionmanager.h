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
#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QHash>
#include <QList>
#include <QLockFile>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <functional>

class TabManager;
class ProjectPanel;
class SplitView;

class SessionManager : public QObject
{
    Q_OBJECT

public:
    explicit SessionManager(QObject* parent = nullptr);

    struct SessionData
    {
        QStringList files;
        int activeIndex = 0;
        QString projectPath;
        QString terminalWorkingDir;
        QHash<QString, QList<int>> bookmarks;
    };

    SessionData restoreSession(std::function<void(const QString& filePath)> loadFileFn, SplitView* splitView, ProjectPanel* projectPanel);

    void saveSession(TabManager* tabManager, ProjectPanel* projectPanel, const QString& terminalWorkingDir);

    // ── Persistence ────────────────────────────────────────────
    void saveSessionData(const QStringList& files, int activeIndex, const QString& projectPath, const QString& terminalWorkingDir = {});
    QStringList sessionFiles();
    int sessionActiveIndex();
    QString sessionProjectPath();
    QString sessionTerminalWorkingDir();
    void saveSessionBookmarks(const QHash<QString, QList<int>>& bookmarks);
    QHash<QString, QList<int>> loadSessionBookmarks();
    void saveSessionPinnedFiles(const QStringList& pinnedFiles);
    QStringList loadSessionPinnedFiles();
    void clearSession();

signals:
    void sessionRestored();

private:
    SessionData loadSessionData();
    QString sessionGroupPrefix() const;

    QSettings m_sessionSettings;
    QLockFile m_sessionLock;
};

#endif
