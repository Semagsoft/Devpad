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
#include "managers/remoteservicecoordinator.h"

#include "logger.h"
#include "managers/actionmanager.h"
#include "managers/remotefileservice.h"
#include "managers/remoteopencoordinator.h"
#include "projectpanel.h"
#include "remotefsentry.h"

#include <QMessageBox>
#include <QStatusBar>
#include <QWidget>

RemoteServiceCoordinator::RemoteServiceCoordinator(RemoteFileService* service, RemoteOpenCoordinator* remoteOpen, ProjectPanel* projectPanel,
                                                   ActionManager* actionManager, QStatusBar* statusBar, QWidget* uiParent, QObject* parent)
    : QObject(parent), m_service(service), m_remoteOpen(remoteOpen), m_projectPanel(projectPanel), m_actionManager(actionManager),
      m_statusBar(statusBar), m_uiParent(uiParent)
{
    connect(m_service, &RemoteFileService::fileDownloaded, this,
            [this](const QString& url, const QString& fileName, const QByteArray& data)
            {
                m_statusBar->clearMessage();
                m_remoteOpen->openRemoteFileFromData(url, fileName, data);
            });
    connect(m_service, &RemoteFileService::remoteDirListed, this,
            [this](const QString& url, const QString& parentPath, const QList<RemoteFsEntry>& entries)
            {
                m_statusBar->clearMessage();
                m_projectPanel->applyRemoteListing(url, parentPath, entries);
                if (!m_projectPanel->isVisible())
                {
                    m_projectPanel->show();
                    m_actionManager->projectPanelAct()->setChecked(true);
                }
                Logger::instance().info(QString("Remote folder listing: %1 (%2, %3 entries)").arg(url, parentPath).arg(entries.size()));
            });
    connect(m_service, &RemoteFileService::remoteListingFailed, this,
            [this](const QString&, const QString& remotePath, const QString&)
            {
                // Let the panel retry the directory (removes the pending state).
                m_projectPanel->onRemoteListingFailed(remotePath);
            });
    connect(m_service, &RemoteFileService::downloadFailed, this,
            [this](const QString& url, const QString& error)
            {
                m_statusBar->clearMessage();
                QMessageBox::warning(m_uiParent, tr("Download Failed"), tr("Failed to download \"%1\":\n%2").arg(url, error));
            });
    connect(m_service, &RemoteFileService::fileUploaded, this,
            [this](const QString& url)
            {
                m_statusBar->clearMessage();
                Logger::instance().info(QString("Uploaded remote file: %1").arg(url));
            });
    connect(m_service, &RemoteFileService::uploadFailed, this,
            [this](const QString&, const QString&)
            {
                // The error dialog is shown by EditorController's
                // remoteSaveFinished handler; just make sure the status bar
                // does not keep showing "Uploading...".
                m_statusBar->clearMessage();
            });
    connect(m_service, &RemoteFileService::downloadProgress, this,
            [this](const QString&, qint64 received, qint64 total)
            {
                if (total > 0)
                {
                    int percent = static_cast<int>(received * 100 / total);
                    m_statusBar->showMessage(tr("Downloading... %1%").arg(qBound(0, percent, 100)));
                }
            });
    connect(m_service, &RemoteFileService::statusMessage, this, [this](const QString& message) { m_statusBar->showMessage(message); });
}
