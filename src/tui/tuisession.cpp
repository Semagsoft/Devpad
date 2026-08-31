/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuisession.h"

#include "core/fileservice.h"
#include "managers/sessionmanager.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>

bool TuiSession::tryRestore(const QCommandLineParser& parser, const QStringList& positionalFiles, TuiTabModel& tabs,
                            TuiFileTree& fileTree)
{
    bool noSession = parser.isSet(QStringLiteral("no-session"));
    if (!positionalFiles.isEmpty() || noSession)
        return false;
    SessionManager sm;
    QStringList sFiles = sm.sessionFiles();
    if (sFiles.isEmpty())
        return false;
    bool isUntitledEmpty = (tabs.count() == 1 && tabs.bufferAt(0) && tabs.bufferAt(0)->filePath().isEmpty()
                            && tabs.bufferAt(0)->text().isEmpty());
    if (!isUntitledEmpty)
        return false;

    TuiTabModel newTabs;
    auto bookmarks = sm.loadSessionBookmarks();
    QStringList sPinned = sm.loadSessionPinnedFiles();
    QSet<QString> pinnedSet;
    for (const QString& p : sPinned)
        pinnedSet.insert(p);
    int loaded = 0;
    for (const QString& fp : sFiles)
    {
        QFileInfo fi(fp);
        if (!fi.exists() || !fi.isFile())
            continue;
        FileLoadResult res = FileService::load(fp);
        if (res.ok)
        {
            TuiBuffer buf(fp, res.text, res.encoding);
            auto it = bookmarks.find(fp);
            if (it != bookmarks.end())
                buf.setBookmarks(it.value());
            newTabs.addBuffer(buf);
            ++loaded;
        }
    }
    if (loaded == 0)
        return false;
    tabs = newTabs;
    int active = sm.sessionActiveIndex();
    tabs.setCurrentIndex(qBound(0, active, tabs.count() - 1));
    tabs.setPinnedFiles(pinnedSet);
    QString proj = sm.sessionProjectPath();
    if (!proj.isEmpty() && QDir(proj).exists())
        fileTree.setRootPath(proj);
    return true;
}

void TuiSession::save(const QCommandLineParser& parser, const TuiTabModel& tabs, const TuiFileTree& fileTree)
{
    if (parser.isSet(QStringLiteral("no-session")))
        return;
    SessionManager sm;
    QStringList files = tabs.allFilePaths();
    int active = tabs.currentIndex();
    QString proj = fileTree.hasRoot() ? fileTree.rootPath() : QString();
    sm.saveSessionData(files, active, proj, QDir::currentPath());
    QHash<QString, QList<int>> bms;
    for (int i = 0; i < tabs.count(); ++i)
    {
        const TuiBuffer* b = tabs.bufferAt(i);
        if (b && !b->filePath().isEmpty() && !b->bookmarks().isEmpty())
            bms.insert(b->filePath(), b->bookmarks());
    }
    sm.saveSessionBookmarks(bms);
    QStringList pinned = tabs.pinnedFiles().values();
    sm.saveSessionPinnedFiles(pinned);
}

void TuiSession::pollExternalChanges(TuiTabModel& tabs, TuiBuffer* cur, QHash<QString, QDateTime>& fileMtimes, QString& statusMsg)
{
    for (int ti = 0; ti < tabs.count(); ++ti)
    {
        TuiBuffer* b = tabs.bufferAt(ti);
        if (!b || b->filePath().isEmpty())
            continue;
        QFileInfo fi(b->filePath());
        if (!fi.exists())
            continue;
        QDateTime curMod = fi.lastModified();
        if (!fileMtimes.contains(b->filePath()))
        {
            fileMtimes[b->filePath()] = curMod;
            continue;
        }
        QDateTime lastMod = fileMtimes.value(b->filePath());
        if (!curMod.isValid() || !lastMod.isValid() || curMod == lastMod)
            continue;
        if (!b->isModified())
        {
            FileLoadResult res = FileService::load(b->filePath());
            if (res.ok)
            {
                bool isCur = (b == cur);
                b->setText(res.text);
                b->setEncoding(res.encoding);
                b->setModified(false);
                fileMtimes[b->filePath()] = curMod;
                if (isCur)
                    statusMsg = QStringLiteral("File reloaded (external change): %1").arg(b->filePath());
            }
            else
            {
                fileMtimes[b->filePath()] = curMod;
                if (b == cur)
                    statusMsg = QStringLiteral("External change detected but reload failed");
            }
        }
        else
        {
            fileMtimes[b->filePath()] = curMod;
            if (b == cur)
                statusMsg = QStringLiteral("WARNING: File changed on disk — :e! to reload, :w to overwrite");
        }
    }
}

void TuiSession::pollFileTree(TuiFileTree& fileTree, qint64& lastPollMs, QDateTime& lastDirMTime, QString& statusMsg)
{
    if (!fileTree.hasRoot())
        return;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastPollMs <= 1500)
        return;
    lastPollMs = now;
    QFileInfo dirFi(fileTree.rootPath());
    QDateTime curDirMod = dirFi.exists() ? dirFi.lastModified() : QDateTime();
    bool needRefresh = false;
    if (!lastDirMTime.isValid() || curDirMod != lastDirMTime)
        needRefresh = true;
    else
    {
        static qint64 lastForcedRefresh = 0;
        if (now - lastForcedRefresh > 5000)
        {
            lastForcedRefresh = now;
            needRefresh = true;
        }
    }
    if (!needRefresh)
        return;
    int beforeCount = fileTree.visibleNodes().size();
    fileTree.refresh();
    int afterCount = fileTree.visibleNodes().size();
    if (beforeCount != afterCount)
        statusMsg = QStringLiteral("Tree refreshed (%1 items)").arg(afterCount);
    lastDirMTime = curDirMod;
}
