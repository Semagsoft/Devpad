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
#ifndef REMOTEOPENCOORDINATOR_H
#define REMOTEOPENCOORDINATOR_H

#include <QObject>

class CodeEditor;
class EditorController;
class ProjectPanel;
class RemoteFileService;
class TabManager;

/**
 * @brief Handles opening remote files and folders.
 *
 * Owns the remote URL prompt/validation dialogs and the editor-creation flow
 * for downloaded remote files. UI fallout (status bar, panel visibility,
 * file-type label) is left to MainWindow via signals.
 */
class RemoteOpenCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit RemoteOpenCoordinator(RemoteFileService* remoteFileService, ProjectPanel* projectPanel, TabManager* tabManager,
                                   EditorController* editorController, QWidget* uiParent, QObject* parent = nullptr);

    /** @brief Prompt for a remote file URL and ask the service to fetch it. */
    void openRemote();

    /** @brief Prompt for a remote (SSH) folder URL and ask the service to list it. */
    void openRemoteFolder();

    /** @brief Activate a remote file: reuse an already-open tab, or fetch it. */
    void openRemoteFileActivated(const QString& urlStr);

    /** @brief Build an editor for an already-downloaded remote file. */
    void openRemoteFileFromData(const QString& urlStr, const QString& fileName, const QByteArray& data);

signals:
    /** @brief The current editor's file-type label may have changed. */
    void fileTypeLabelDirty();

private:
    RemoteFileService* m_remoteFileService;
    ProjectPanel* m_projectPanel;
    TabManager* m_tabManager;
    EditorController* m_editorController;
    QWidget* m_uiParent;
};

#endif // REMOTEOPENCOORDINATOR_H
