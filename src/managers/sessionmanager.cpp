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

#include "appstrings.h"
#include "codeeditor.h"
#include "logger.h"
#include "projectpanel.h"
#include "splitview.h"
#include "tabmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTabWidget>

#include <cerrno>
#include <csignal>

SessionManager::SessionManager(QObject* parent)
    : QObject(parent), m_sessionSettings(QStringLiteral("Semagsoft"), QStringLiteral("Devpad")),
      m_sessionLock(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/devpad/session.lock"))
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/devpad"));
    m_sessionLock.setStaleLockTime(30000);
}

QString SessionManager::sessionGroupPrefix() const
{
    return QStringLiteral("Session_%1").arg(QCoreApplication::applicationPid());
}

static bool processAlive(qint64 pid)
{
#if defined(Q_OS_UNIX)
    if (pid <= 0)
        return false;
    if (::kill(static_cast<pid_t>(pid), 0) == 0)
        return true;
    return errno == EPERM; // Exists but owned by another user.
#else
    Q_UNUSED(pid)
    // Cannot reliably check liveness; fail safe by keeping the group.
    return true;
#endif
}

void SessionManager::pruneDeadSessionGroups(const QString& ownGroup)
{
    // Re-read from disk first so groups written by other instances are visible.
    m_sessionSettings.sync();
    const QString prefix = QStringLiteral("Session_");
    for (const QString& g : m_sessionSettings.childGroups())
    {
        if (!g.startsWith(prefix) || g == ownGroup)
            continue;
        bool ok = false;
        const qint64 pid = g.mid(prefix.size()).toLongLong(&ok);
        if (!ok)
            continue; // Unrecognized format; leave it alone.
        if (processAlive(pid))
        {
            Logger::instance().info(QStringLiteral("SessionManager: keeping session group %1 (process is running)").arg(g));
            continue;
        }
        m_sessionSettings.remove(g);
        Logger::instance().info(QStringLiteral("SessionManager: pruned stale session group %1").arg(g));
    }
}

QVariant SessionManager::sessionValue(const QString& group, const QString& key, const QString& legacyKey)
{
    if (m_sessionSettings.childGroups().contains(group))
    {
        m_sessionSettings.beginGroup(group);
        const bool has = m_sessionSettings.contains(key);
        const QVariant v = has ? m_sessionSettings.value(key) : QVariant();
        m_sessionSettings.endGroup();
        if (has)
            return v;
    }
    return m_sessionSettings.value(legacyKey);
}

SessionManager::SessionData SessionManager::loadSessionData()
{
    m_lastLoadFailed = false;
    SessionData data;
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("SessionManager: failed to acquire lock in loadSessionData");
        data.lockFailed = true;
        m_lastLoadFailed = true;
        return data;
    }
    // Pick up changes written by other instances before reading.
    m_sessionSettings.sync();

    const QString group = sessionGroupPrefix();

    // Read ONLY this instance's own session group, falling back to the legacy
    // flat keys (the last clean-quit snapshot). Merging every Session_* group
    // resurrected files from crashed or exited instances uninvited; their stale
    // groups are pruned below instead of being loaded.
    QVariantMap bookmarkMap;
    QVariantMap cursorMap;
    data.files = sessionValue(group, QStringLiteral("Files"), QStringLiteral("Session_Files")).toStringList();
    data.activeIndex = sessionValue(group, QStringLiteral("ActiveIndex"), QStringLiteral("Session_ActiveIndex")).toInt();
    data.projectPath = sessionValue(group, QStringLiteral("ProjectPath"), QStringLiteral("Session_ProjectPath")).toString();
    data.terminalWorkingDir = sessionValue(group, QStringLiteral("TerminalWorkingDir"), QStringLiteral("Session_TerminalWorkingDir")).toString();
    bookmarkMap = sessionValue(group, QStringLiteral("Bookmarks"), QStringLiteral("Session_Bookmarks")).toMap();
    cursorMap = sessionValue(group, QStringLiteral("Cursors"), QStringLiteral("Session_Cursors")).toMap();

    data.activeIndex = qMin(data.activeIndex, qMax(data.files.size() - 1, 0));

    for (auto it = bookmarkMap.constBegin(); it != bookmarkMap.constEnd(); ++it)
    {
        QList<int> lines;
        for (const QVariant& v : it.value().toList())
            lines.append(v.toInt());
        if (!lines.isEmpty())
            data.bookmarks.insert(it.key(), lines);
    }

    for (auto it = cursorMap.constBegin(); it != cursorMap.constEnd(); ++it)
    {
        QList<int> values;
        for (const QVariant& v : it.value().toList())
            values.append(v.toInt());
        if (values.size() >= 3)
            data.cursors.insert(it.key(), values.mid(0, 3));
    }

    // Prune groups of instances whose process is gone (crashed or exited).
    // Groups of still-running instances are left intact; previously every
    // foreign group was removed, which silently destroyed other windows'
    // sessions.
    pruneDeadSessionGroups(group);

    m_sessionLock.unlock();

    return data;
}

SessionManager::SessionData SessionManager::restoreSession(std::function<void(const QString& filePath)> loadFileFn, SplitView* splitView,
                                                           ProjectPanel* projectPanel)
{
    SessionData data = loadSessionData();

    if (!data.projectPath.isEmpty() && QDir(data.projectPath).exists())
    {
        projectPanel->setRootPath(data.projectPath);
        projectPanel->show();
    }

    if (data.files.isEmpty())
    {
        return data;
    }

    int targetIndex = 0;
    int loadedCount = 0;
    for (int fi = 0; fi < data.files.size(); ++fi)
    {
        if (QFile::exists(data.files[fi]))
        {
            loadFileFn(data.files[fi]);
            if (fi <= data.activeIndex)
            {
                targetIndex = loadedCount;
            }
            loadedCount++;
        }
    }

    int editorCount = 0;
    for (int p = 0; p < splitView->paneCount(); ++p)
    {
        QTabWidget* pane = splitView->paneAt(p);
        for (int i = 0; i < pane->count(); ++i)
        {
            auto* w = pane->widget(i);
            auto* editor = w ? w->findChild<CodeEditor*>() : nullptr;
            if (editor)
            {
                auto it = data.bookmarks.constFind(editor->fileName());
                if (it != data.bookmarks.constEnd())
                {
                    editor->setBookmarks(it.value());
                }
                auto cursorIt = data.cursors.constFind(editor->fileName());
                if (cursorIt != data.cursors.constEnd() && cursorIt->size() == 3)
                {
                    // Apply before setting the cursor so the first visible line
                    // already reflects the remembered scroll offset.
                    editor->setFirstVisibleLine((*cursorIt)[2]);
                    editor->setCursorPosition((*cursorIt)[0], (*cursorIt)[1]);
                }
                if (editorCount == targetIndex)
                {
                    pane->setCurrentIndex(i);
                }
                editorCount++;
            }
        }
    }

    emit sessionRestored();
    return data;
}

void SessionManager::saveSessionData(const QStringList& files, int activeIndex, const QString& projectPath, const QString& terminalWorkingDir)
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("Failed to acquire session lock for saveSessionData");
        m_lastSaveFailed = true;
        return;
    }

    m_sessionSettings.beginGroup(sessionGroupPrefix());
    m_sessionSettings.setValue(QStringLiteral("Files"), files);
    m_sessionSettings.setValue(QStringLiteral("ActiveIndex"), activeIndex);
    m_sessionSettings.setValue(QStringLiteral("ProjectPath"), projectPath);
    m_sessionSettings.setValue(QStringLiteral("TerminalWorkingDir"), terminalWorkingDir);
    m_sessionSettings.setValue(QStringLiteral("HasSession"), true);
    m_sessionSettings.endGroup();

    // Mirror the snapshot into the legacy flat keys: the last clean-quit
    // snapshot is discoverable by later launches (and other front-ends) this
    // way, without merging per-PID groups of crashed or exited instances.
    m_sessionSettings.setValue(QStringLiteral("Session_Files"), files);
    m_sessionSettings.setValue(QStringLiteral("Session_ActiveIndex"), activeIndex);
    m_sessionSettings.setValue(QStringLiteral("Session_ProjectPath"), projectPath);
    m_sessionSettings.setValue(QStringLiteral("Session_TerminalWorkingDir"), terminalWorkingDir);

    // Remove stale session groups from previous (crashed) process PIDs; keep
    // groups of still-running instances.
    pruneDeadSessionGroups(sessionGroupPrefix());

    m_sessionSettings.sync();
    m_sessionLock.unlock();
}

QStringList SessionManager::sessionFiles()
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("SessionManager: failed to acquire lock in sessionFiles");
        m_lastLoadFailed = true;
        return {};
    }
    QString group = sessionGroupPrefix();
    const QStringList files = sessionValue(group, QStringLiteral("Files"), QStringLiteral("Session_Files")).toStringList();
    m_sessionLock.unlock();
    return files;
}

int SessionManager::sessionActiveIndex()
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("SessionManager: failed to acquire lock in sessionActiveIndex");
        m_lastLoadFailed = true;
        return 0;
    }
    QString group = sessionGroupPrefix();
    const int idx = sessionValue(group, QStringLiteral("ActiveIndex"), QStringLiteral("Session_ActiveIndex")).toInt();
    m_sessionLock.unlock();
    return idx;
}

QString SessionManager::sessionProjectPath()
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("SessionManager: failed to acquire lock in sessionProjectPath");
        m_lastLoadFailed = true;
        return {};
    }
    QString group = sessionGroupPrefix();
    const QString path = sessionValue(group, QStringLiteral("ProjectPath"), QStringLiteral("Session_ProjectPath")).toString();
    m_sessionLock.unlock();
    return path;
}

QString SessionManager::sessionTerminalWorkingDir()
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("SessionManager: failed to acquire lock in sessionTerminalWorkingDir");
        m_lastLoadFailed = true;
        return {};
    }
    QString group = sessionGroupPrefix();
    const QString dir = sessionValue(group, QStringLiteral("TerminalWorkingDir"), QStringLiteral("Session_TerminalWorkingDir")).toString();
    m_sessionLock.unlock();
    return dir;
}

void SessionManager::saveSessionBookmarks(const QHash<QString, QList<int>>& bookmarks)
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("Failed to acquire session lock for saveSessionBookmarks");
        m_lastSaveFailed = true;
        return;
    }

    QVariantMap map;
    for (auto it = bookmarks.constBegin(); it != bookmarks.constEnd(); ++it)
    {
        QVariantList lines;
        for (int line : it.value())
            lines.append(line);
        map.insert(it.key(), lines);
    }
    m_sessionSettings.beginGroup(sessionGroupPrefix());
    m_sessionSettings.setValue(QStringLiteral("Bookmarks"), map);
    m_sessionSettings.endGroup();
    m_sessionSettings.setValue(QStringLiteral("Session_Bookmarks"), map);
    m_sessionSettings.sync();

    m_sessionLock.unlock();
}

QHash<QString, QList<int>> SessionManager::loadSessionBookmarks()
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("SessionManager: failed to acquire lock in loadSessionBookmarks");
        m_lastLoadFailed = true;
        return {};
    }

    QHash<QString, QList<int>> result;
    const QString group = sessionGroupPrefix();
    const QVariantMap map = sessionValue(group, QStringLiteral("Bookmarks"), QStringLiteral("Session_Bookmarks")).toMap();

    m_sessionLock.unlock();

    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
    {
        QList<int> lines;
        for (const QVariant& v : it.value().toList())
            lines.append(v.toInt());
        if (!lines.isEmpty())
            result.insert(it.key(), lines);
    }
    return result;
}

void SessionManager::saveSessionCursors(const QHash<QString, QList<int>>& cursors)
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("Failed to acquire session lock for saveSessionCursors");
        m_lastSaveFailed = true;
        return;
    }

    QVariantMap map;
    for (auto it = cursors.constBegin(); it != cursors.constEnd(); ++it)
    {
        QVariantList values;
        for (int v : it.value())
            values.append(v);
        map.insert(it.key(), values);
    }
    m_sessionSettings.beginGroup(sessionGroupPrefix());
    m_sessionSettings.setValue(QStringLiteral("Cursors"), map);
    m_sessionSettings.endGroup();
    m_sessionSettings.setValue(QStringLiteral("Session_Cursors"), map);
    m_sessionSettings.sync();

    m_sessionLock.unlock();
}

QHash<QString, QList<int>> SessionManager::loadSessionCursors()
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("Failed to acquire session lock in loadSessionCursors");
        m_lastLoadFailed = true;
        return {};
    }

    QHash<QString, QList<int>> result;
    const QString group = sessionGroupPrefix();
    const QVariantMap map = sessionValue(group, QStringLiteral("Cursors"), QStringLiteral("Session_Cursors")).toMap();

    m_sessionLock.unlock();

    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
    {
        QList<int> values;
        for (const QVariant& v : it.value().toList())
            values.append(v.toInt());
        if (values.size() >= 3)
            result.insert(it.key(), values.mid(0, 3));
    }
    return result;
}

void SessionManager::saveSessionPinnedFiles(const QStringList& pinnedFiles)
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("Failed to acquire session lock for saveSessionPinnedFiles");
        m_lastSaveFailed = true;
        return;
    }

    m_sessionSettings.beginGroup(sessionGroupPrefix());
    m_sessionSettings.setValue(QStringLiteral("PinnedFiles"), pinnedFiles);
    m_sessionSettings.endGroup();
    m_sessionSettings.setValue(QStringLiteral("Session_PinnedFiles"), pinnedFiles);
    m_sessionSettings.sync();

    m_sessionLock.unlock();
}

QStringList SessionManager::loadSessionPinnedFiles()
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("SessionManager: failed to acquire lock in loadSessionPinnedFiles");
        m_lastLoadFailed = true;
        return {};
    }

    const QString group = sessionGroupPrefix();
    const QStringList files = sessionValue(group, QStringLiteral("PinnedFiles"), QStringLiteral("Session_PinnedFiles")).toStringList();

    m_sessionLock.unlock();

    return files;
}

void SessionManager::saveSessionSplitState(bool splitVisible, const QVariantList& tabPanes)
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("Failed to acquire session lock for saveSessionSplitState");
        m_lastSaveFailed = true;
        return;
    }

    m_sessionSettings.beginGroup(sessionGroupPrefix());
    m_sessionSettings.setValue(QStringLiteral("SplitVisible"), splitVisible);
    m_sessionSettings.setValue(QStringLiteral("TabPanes"), tabPanes);
    m_sessionSettings.endGroup();
    m_sessionSettings.setValue(QStringLiteral("Session_SplitVisible"), splitVisible);
    m_sessionSettings.setValue(QStringLiteral("Session_TabPanes"), tabPanes);
    m_sessionSettings.sync();

    m_sessionLock.unlock();
}

SessionManager::SplitSessionState SessionManager::loadSessionSplitState()
{
    SplitSessionState state;
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("Failed to acquire session lock in loadSessionSplitState");
        m_lastLoadFailed = true;
        return state;
    }

    const QString group = sessionGroupPrefix();
    state.splitVisible = sessionValue(group, QStringLiteral("SplitVisible"), QStringLiteral("Session_SplitVisible")).toBool();
    state.tabPanes = sessionValue(group, QStringLiteral("TabPanes"), QStringLiteral("Session_TabPanes")).toList();

    m_sessionLock.unlock();

    return state;
}

void SessionManager::clearSession()
{
    if (!m_sessionLock.tryLock(5000))
    {
        Logger::instance().error("Failed to acquire session lock for clearSession");
        m_lastSaveFailed = true;
        return;
    }

    QString group = sessionGroupPrefix();

    pruneDeadSessionGroups(group);
    m_sessionSettings.remove(group);

    m_sessionSettings.remove(QStringLiteral("Session_Files"));
    m_sessionSettings.remove(QStringLiteral("Session_ActiveIndex"));
    m_sessionSettings.remove(QStringLiteral("Session_ProjectPath"));
    m_sessionSettings.remove(QStringLiteral("Session_Bookmarks"));
    m_sessionSettings.remove(QStringLiteral("Session_Cursors"));
    m_sessionSettings.remove(QStringLiteral("Session_PinnedFiles"));
    m_sessionSettings.remove(QStringLiteral("Session_TerminalWorkingDir"));
    m_sessionSettings.remove(QStringLiteral("Session_SplitVisible"));
    m_sessionSettings.remove(QStringLiteral("Session_TabPanes"));
    m_sessionSettings.remove(QStringLiteral("HasSession"));
    m_sessionSettings.sync();

    m_sessionLock.unlock();
}

void SessionManager::saveSession(TabManager* tabManager, ProjectPanel* projectPanel, const QString& terminalWorkingDir)
{
    m_lastSaveFailed = false;
    QStringList files;
    int activeIndex = 0;
    QTabWidget* currentPane = tabManager->activePane();
    for (QTabWidget* pane : tabManager->panes())
    {
        for (int i = 0; i < pane->count(); ++i)
        {
            auto* w = pane->widget(i);
            auto* editor = w ? w->findChild<CodeEditor*>() : nullptr;
            if (!editor)
                continue;
            QString fileName = editor->fileName();
            if (fileName == Strings::untitled() || fileName.isEmpty())
            {
                continue;
            }
            if (pane == currentPane && i == pane->currentIndex())
            {
                activeIndex = files.size();
            }
            files.append(fileName);
        }
    }

    QHash<QString, QList<int>> bookmarks;
    QHash<QString, QList<int>> cursors;
    for (QTabWidget* pane : tabManager->panes())
    {
        for (int i = 0; i < pane->count(); ++i)
        {
            auto* w = pane->widget(i);
            auto* editor = w ? w->findChild<CodeEditor*>() : nullptr;
            if (editor && editor->fileName() != Strings::untitled() && !editor->fileName().isEmpty())
            {
                QList<int> lines = editor->bookmarkLines();
                if (!lines.isEmpty())
                {
                    bookmarks.insert(editor->fileName(), lines);
                }

                int line = 0;
                int index = 0;
                editor->getCursorPosition(&line, &index);
                cursors.insert(editor->fileName(), {line, index, editor->firstVisibleLine()});
            }
        }
    }

    QString projectPath = projectPanel->isVisible() ? projectPanel->rootPath() : QString();
    saveSessionData(files, activeIndex, projectPath, terminalWorkingDir);
    saveSessionBookmarks(bookmarks);
    saveSessionCursors(cursors);
    saveSessionPinnedFiles(tabManager->pinnedFiles());
}
