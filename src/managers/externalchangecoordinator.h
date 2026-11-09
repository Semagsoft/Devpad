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
#ifndef EXTERNALCHANGECOORDINATOR_H
#define EXTERNALCHANGECOORDINATOR_H

#include <QList>
#include <QObject>
#include <QPointer>

class CodeEditor;
class FileWatcherManager;
class TabManager;

/**
 * @brief Handles externally-modified open files.
 *
 * On external change, prompts the user and reloads the file while preserving
 * bookmarks and encoding. Reload requests are emitted as @ref reloadRequested
 * so MainWindow performs the actual file loading.
 */
class ExternalChangeCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit ExternalChangeCoordinator(TabManager* tabManager, FileWatcherManager* fileWatcherManager, QObject* parent = nullptr);

    /** @brief Handle an external modification of an open file. */
    void handleFileModifiedExternally(const QString& filePath);

signals:
    /**
     * @brief Reload @p filePath with @p encoding by closing the current editor
     *        and loading the file again (MainWindow executes this). The editor
     *        bookmarks captured before closing are passed back for restoration.
     */
    void reloadRequested(const QString& filePath, const QString& encoding, const QList<int>& bookmarks);

private:
    TabManager* m_tabManager;
    FileWatcherManager* m_fileWatcherManager;

    // Guards the modal "File Modified" prompt: QMessageBox spins a nested event
    // loop, so further watcher signals must not stack extra dialogs on top of it.
    bool m_promptActive = false;
};

#endif // EXTERNALCHANGECOORDINATOR_H
