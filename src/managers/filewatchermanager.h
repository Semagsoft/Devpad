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
#ifndef FILEWATCHERMANAGER_H
#define FILEWATCHERMANAGER_H

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QHash>
#include <QObject>

class FileWatcherManager : public QObject
{
    Q_OBJECT

public:
    // Changes to the same file reported within this window of each other are coalesced
    // into a single fileModifiedExternally signal.
    static constexpr int COALESCE_INTERVAL_MS = 250;

    explicit FileWatcherManager(QObject* parent = nullptr);

    void watchFile(const QString& filePath);
    void unwatchFile(const QString& filePath);
    void unwatchAll();

    void updateModificationTime(const QString& filePath);

signals:
    void fileModifiedExternally(const QString& filePath);

private slots:
    void onDirectoryChanged(const QString& path);
    void onFileChanged(const QString& path);

private:
    // True when the last change we reported for filePath is still inside the coalescing
    // window, i.e. a further change is only refreshed, not re-emitted.
    bool isWithinCoalesceWindow(const QString& filePath) const;

    QFileSystemWatcher m_watcher;
    QHash<QString, QDateTime> m_modificationTimes;
    QHash<QString, QDateTime> m_lastReported;
};

#endif
