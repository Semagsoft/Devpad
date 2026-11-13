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
#ifndef REMOTESERVICECOORDINATOR_H
#define REMOTESERVICECOORDINATOR_H

#include <QObject>

class ActionManager;
class ProjectPanel;
class QStatusBar;
class QWidget;
class RemoteFileService;
class RemoteOpenCoordinator;

/**
 * @brief Wires RemoteFileService signals to UI feedback.
 *
 * Extracted from MainWindow: status-bar progress/messages, error dialogs,
 * project-panel updates for remote folder listings, and handing downloaded
 * remote files to the RemoteOpenCoordinator. Remote *opening* itself is
 * handled by RemoteOpenCoordinator; this class owns the service <-> UI glue.
 */
class RemoteServiceCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit RemoteServiceCoordinator(RemoteFileService* service, RemoteOpenCoordinator* remoteOpen, ProjectPanel* projectPanel,
                                      ActionManager* actionManager, QStatusBar* statusBar, QWidget* uiParent, QObject* parent = nullptr);

private:
    RemoteFileService* m_service;
    RemoteOpenCoordinator* m_remoteOpen;
    ProjectPanel* m_projectPanel;
    ActionManager* m_actionManager;
    QStatusBar* m_statusBar;
    QWidget* m_uiParent;
};

#endif // REMOTESERVICECOORDINATOR_H
