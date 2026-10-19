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

FileWatcherManager::FileWatcherManager(QObject* parent) : QObject(parent)
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &FileWatcherManager::onFileChanged);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &FileWatcherManager::onDirectoryChanged);
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
}

void FileWatcherManager::unwatchFile(const QString& filePath)
{
    m_watcher.removePath(filePath);
    m_modificationTimes.remove(filePath);
    m_lastReported.remove(filePath);
    Logger::instance().debug(QString("Unwatched file: %1").arg(filePath));
}

void FileWatcherManager::unwatchAll()
{
    m_watcher.removePaths(m_watcher.files());
    m_modificationTimes.clear();
    m_lastReported.clear();
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

void FileWatcherManager::onFileChanged(const QString& path)
{
    QFileInfo info(path);

    if (!info.exists())
    {
        Logger::instance().info(QString("File deleted externally: %1").arg(path));
        emit fileModifiedExternally(path);
        m_lastReported[path] = QDateTime::currentDateTime();
        return;
    }

    QDateTime currentMod = info.lastModified();
    QDateTime knownMod = m_modificationTimes.value(path);

    if (knownMod.isValid() && currentMod > knownMod)
    {
        m_modificationTimes[path] = currentMod;

        // A single external write often touches the file more than once (truncate then
        // write, save-as, formatter + compiler). Only surface the first of those bursts.
        if (isWithinCoalesceWindow(path))
        {
            Logger::instance().debug(QString("Coalesced rapid external change: %1").arg(path));
        }
        else
        {
            m_lastReported[path] = QDateTime::currentDateTime();
            Logger::instance().info(QString("File modified externally: %1").arg(path));
            emit fileModifiedExternally(path);
        }
    }

    // Re-arm the watcher for the next change. On macOS QFileSystemWatcher stops
    // delivering fileChanged for a file after the first notification unless the
    // path is removed and re-added, so a further modification would otherwise go
    // unreported (breaks ModificationAfterCoalesceWindowEmitsAgain on macOS CI).
    m_watcher.removePath(path);
    if (info.exists())
        m_watcher.addPath(path);
}
