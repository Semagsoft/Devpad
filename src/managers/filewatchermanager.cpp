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
#include "filewatchermanager.h"

#include "logger.h"

#include <QFileInfo>
#include <QTimer>

FileWatcherManager::FileWatcherManager(QObject* parent) : QObject(parent)
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &FileWatcherManager::onFileChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &FileWatcherManager::onDirectoryChanged);
    connect(&m_pollTimer, &QTimer::timeout, this, &FileWatcherManager::pollModifications);
    m_pollTimer.start(POLL_INTERVAL_MS);
}

void FileWatcherManager::watchFile(const QString& filePath)
{
    if (!m_watcher.addPath(filePath))
    {
        Logger::instance().warning(QString("Failed to watch file: %1").arg(filePath));
    }

    QFileInfo info(filePath);
    if (info.exists())
    {
        m_modificationTimes[filePath] = info.lastModified();
        Logger::instance().debug(QString("Watching file: %1").arg(filePath));
    }
    m_lastReported.remove(filePath);
    // A fresh watch request means the app is engaging with the file again
    // (e.g. it was just saved or re-created); clear the deleted marker so
    // coalescing behaves normally from here on.
    m_deletedPaths.remove(filePath);
}

void FileWatcherManager::unwatchFile(const QString& filePath)
{
    m_watcher.removePath(filePath);
    m_modificationTimes.remove(filePath);
    m_lastReported.remove(filePath);
    m_deletedPaths.remove(filePath);
    Logger::instance().debug(QString("Unwatched file: %1").arg(filePath));
}

void FileWatcherManager::unwatchAll()
{
    m_watcher.removePaths(m_watcher.files());
    m_modificationTimes.clear();
    m_lastReported.clear();
    m_deletedPaths.clear();
}
void FileWatcherManager::updateModificationTime(const QString& filePath)
{
    QFileInfo info(filePath);
    if (info.exists())
    {
        m_modificationTimes[filePath] = info.lastModified();
    }
}

void FileWatcherManager::onDirectoryChanged(const QString& path)
{
    QFileInfo info(path);
    if (info.exists())
    {
        m_modificationTimes[path] = info.lastModified();
    }

    if (!m_watcher.directories().contains(path))
    {
        m_watcher.addPath(path);
    }

    for (auto it = m_modificationTimes.begin(); it != m_modificationTimes.end(); ++it)
    {
        const QString& watchedFile = it.key();
        if (watchedFile.startsWith(path + '/') && !m_watcher.files().contains(watchedFile))
        {
            QFileInfo fileInfo(watchedFile);
            if (fileInfo.exists())
            {
                m_watcher.addPath(watchedFile);
                it.value() = fileInfo.lastModified();
            }
        }
    }
}

bool FileWatcherManager::isWithinCoalesceWindow(const QString& filePath) const
{
    QDateTime last = m_lastReported.value(filePath);
    if (!last.isValid())
        return false;

    return last.msecsTo(QDateTime::currentDateTime()) < COALESCE_INTERVAL_MS;
}

void FileWatcherManager::pollModifications()
{
    // Fallback detector for external changes. QFileSystemWatcher on macOS stops
    // delivering fileChanged for a path after the first notification and does not
    // reliably re-notify after removePath()/addPath(), so we periodically re-stat
    // watched files to make sure no modification goes unreported.
    // QHash::erase() returns the iterator following the erased element, so we use
    // a while loop: erasing during iteration invalidates the old iterator, and
    // erase() keeps us positioned on a valid element.
    auto it = m_modificationTimes.begin();
    while (it != m_modificationTimes.end())
    {
        const QString& filePath = it.key();
        QFileInfo info(filePath);
        if (!info.exists())
        {
            // File was deleted. QFileSystemWatcher on macOS auto-removes deleted
            // paths and stops notifying, so the poll fallback must catch this and
            // surface the deletion just like onFileChanged() does. Keep the entry
            // in the poll set with an invalid marker: if the file is re-created
            // (save-via-rename), the poll path can detect it and re-add the
            // watch — dropping the entry here would leave the stale buffer
            // permanently undetectable.
            if (!m_deletedPaths.contains(filePath))
            {
                m_deletedPaths.insert(filePath);
                it.value() = QDateTime(); // invalid marker = "awaiting reappearance"
                Logger::instance().info(QString("File deleted externally: %1").arg(filePath));
                emit fileModifiedExternally(filePath);
            }
            ++it;
            continue;
        }

        QDateTime currentMod = info.lastModified();
        QDateTime knownMod = it.value();
        const bool wasDeleted = m_deletedPaths.contains(filePath);
        if (!wasDeleted && knownMod.isValid() && currentMod <= knownMod)
        {
            ++it;
            continue;
        }

        // Mirror the coalescing behavior of onFileChanged, except a file that
        // was previously reported deleted and has reappeared is always
        // re-reported (external editors save via delete + rename).
        if (isWithinCoalesceWindow(filePath) && !wasDeleted)
        {
            ++it;
            continue;
        }

        it.value() = currentMod;
        if (wasDeleted && !m_watcher.files().contains(filePath))
        {
            // The watch was dropped when the file disappeared; re-add it so
            // QFileSystemWatcher notifications resume for the new file.
            m_watcher.addPath(filePath);
        }
        m_deletedPaths.remove(filePath);
        m_lastReported[filePath] = QDateTime::currentDateTime();
        if (wasDeleted)
            Logger::instance().info(QString("File re-created externally: %1").arg(filePath));
        else
            Logger::instance().info(QString("File modified externally: %1").arg(filePath));
        emit fileModifiedExternally(filePath);
        ++it;
    }
}

void FileWatcherManager::onFileChanged(const QString& path)
{
    QFileInfo info(path);

    if (!info.exists())
    {
        Logger::instance().info(QString("File deleted externally: %1").arg(path));
        m_deletedPaths.insert(path);
        // Keep the entry with an invalid marker so the poll fallback can
        // detect the file being re-created (save-via-rename).
        m_modificationTimes[path] = QDateTime();
        emit fileModifiedExternally(path);
        m_lastReported[path] = QDateTime::currentDateTime();
        return;
    }

    QDateTime currentMod = info.lastModified();
    QDateTime knownMod = m_modificationTimes.value(path);

    if (knownMod.isValid() && currentMod > knownMod)
    {
        m_modificationTimes[path] = std::move(currentMod);

        // A single external write often touches the file more than once (truncate then
        // write, save-as, formatter + compiler). Only surface the first of those bursts.
        if (isWithinCoalesceWindow(path) && !m_deletedPaths.contains(path))
        {
            Logger::instance().debug(QString("Coalesced rapid external change: %1").arg(path));
        }
        else
        {
            // If the file was reported deleted and has now reappeared (delete +
            // rename save pattern of external editors), the change must always
            // be surfaced — even inside the coalescing window — otherwise the
            // editor keeps a stale buffer for the new file contents.
            if (m_deletedPaths.contains(path))
                Logger::instance().info(QString("File re-created externally: %1").arg(path));
            m_deletedPaths.remove(path);
            m_lastReported[path] = QDateTime::currentDateTime();
            Logger::instance().info(QString("File modified externally: %1").arg(path));
            emit fileModifiedExternally(path);
        }
    }
    else if (m_deletedPaths.contains(path))
    {
        // Reappeared with the same (or older) timestamp — still update state so
        // future polls have a valid baseline.
        m_deletedPaths.remove(path);
        m_modificationTimes[path] = std::move(currentMod);
        m_lastReported[path] = QDateTime::currentDateTime();
        Logger::instance().info(QString("File re-created externally: %1").arg(path));
        emit fileModifiedExternally(path);
    }
}
