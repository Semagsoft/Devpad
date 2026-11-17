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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "managers/updateservicecoordinator.h"

#include "managers/updatechecker.h"
#include "updatedialog.h"

#include <QStatusBar>

UpdateServiceCoordinator::UpdateServiceCoordinator(UpdateChecker* checker, QStatusBar* statusBar, QWidget* dialogParent, QObject* parent)
    : QObject(parent), m_checker(checker), m_statusBar(statusBar), m_dialogParent(dialogParent)
{
    connect(m_checker, &UpdateChecker::updateAvailable, this,
            [this](const QString& latestVersion, const QString& releaseUrl)
            {
                m_statusBar->clearMessage();
                UpdateDialog::showUpdateAvailable(m_dialogParent, latestVersion, releaseUrl);
            });
    connect(m_checker, &UpdateChecker::upToDate, this,
            [this](const QString& latestVersion)
            {
                m_statusBar->clearMessage();
                UpdateDialog::showUpToDate(m_dialogParent, latestVersion);
            });
    connect(m_checker, &UpdateChecker::checkFailed, this,
            [this](const QString& friendlyMessage, const QString& details)
            {
                m_statusBar->clearMessage();
                if (UpdateDialog::showCheckFailed(m_dialogParent, friendlyMessage, details))
                    emit retryCheckRequested();
            });
}
