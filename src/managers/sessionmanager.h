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
        // Per file: {cursor line, cursor index, first visible line}
        QHash<QString, QList<int>> cursors;
        // True when the session file could not be locked (e.g. another
        // hung instance holds it); the data above is then empty/stale and
        // must not be mistaken for "no session exists".
        bool lockFailed = false;
    };

    SessionData restoreSession(std::function<void(const QString& filePath)> loadFileFn, SplitView* splitView, ProjectPanel* projectPanel);

    void saveSession(TabManager* tabManager, ProjectPanel* projectPanel, const QString& terminalWorkingDir);
    // True when the most recent saveSession() call could not acquire the
    // session lock and some state was dropped instead of persisted.
    bool lastSaveFailed() const { return m_lastSaveFailed; }
    // True when the most recent read could not acquire the session lock and
    // returned an empty/stale default instead of stored data.
    bool lastLoadFailed() const { return m_lastLoadFailed; }
    bool lastOperationFailed() const { return m_lastSaveFailed || m_lastLoadFailed; }

    // ── Persistence ────────────────────────────────────────────
    void saveSessionData(const QStringList& files, int activeIndex, const QString& projectPath, const QString& terminalWorkingDir = {});
    QStringList sessionFiles();
    int sessionActiveIndex();
    QString sessionProjectPath();
    QString sessionTerminalWorkingDir();
    void saveSessionBookmarks(const QHash<QString, QList<int>>& bookmarks);
    QHash<QString, QList<int>> loadSessionBookmarks();
    void saveSessionCursors(const QHash<QString, QList<int>>& cursors);
    QHash<QString, QList<int>> loadSessionCursors();
    void saveSessionPinnedFiles(const QStringList& pinnedFiles);
    QStringList loadSessionPinnedFiles();
    void clearSession();
    SessionData loadSessionData();

    // ── Split view persistence (Nextgen frontend) ────────────────
    void saveSessionSplitState(bool splitVisible, const QVariantList& tabPanes);
    struct SplitSessionState
    {
        bool splitVisible = false;
        QVariantList tabPanes;
    };
    SplitSessionState loadSessionSplitState();

signals:
    void sessionRestored();

private:
    QString sessionGroupPrefix() const;
    void pruneDeadSessionGroups(const QString& ownGroup);
    // Per-key fallback: return the value for `key` from this instance's own
    // PID group when the key EXISTS there (even if empty); otherwise the
    // legacy flat key `legacyKey`. Prevents a partially written own-PID
    // group from shadowing the legacy snapshot with empty values.
    QVariant sessionValue(const QString& group, const QString& key, const QString& legacyKey);

    QSettings m_sessionSettings;
    QLockFile m_sessionLock;
    bool m_lastSaveFailed = false;
    bool m_lastLoadFailed = false;
};

#endif
