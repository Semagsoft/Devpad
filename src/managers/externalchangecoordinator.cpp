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
#include "externalchangecoordinator.h"

#include "codeeditor.h"
#include "filewatchermanager.h"
#include "logger.h"
#include "tabmanager.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QPointer>
#include <QWidget>

ExternalChangeCoordinator::ExternalChangeCoordinator(TabManager* tabManager, FileWatcherManager* fileWatcherManager, QObject* parent)
    : QObject(parent), m_tabManager(tabManager), m_fileWatcherManager(fileWatcherManager)
{
}

void ExternalChangeCoordinator::handleFileModifiedExternally(const QString& filePath)
{
    // The prompt below is modal, which means a nested event loop keeps delivering queued
    // watcher signals while it is open. Drop them instead of stacking another dialog.
    if (m_promptActive)
    {
        Logger::instance().debug(QString("Ignoring external change while prompt is open: %1").arg(filePath));
        return;
    }

    QPointer<CodeEditor> editor = m_tabManager->findEditorByFileName(filePath);
    if (!editor)
        return;

    QString message;
    if (editor->isModified())
    {
        message = QObject::tr("The file \"%1\" has been modified outside of Devpad.\n\n"
                              "Your version has unsaved changes.\n\n"
                              "Do you want to reload the file? (Unsaved changes will be lost)");
    }
    else
    {
        message = QObject::tr("The file \"%1\" has been modified outside of Devpad.\n\n"
                              "Do you want to reload the file?");
    }

    m_promptActive = true;
    QMessageBox::StandardButton result = QMessageBox::question(qobject_cast<QWidget*>(parent()), QObject::tr("File Modified"),
                                                               message.arg(QFileInfo(filePath).fileName()), QMessageBox::Yes | QMessageBox::No);
    m_promptActive = false;

    if (result != QMessageBox::Yes)
        return;

    // Preserve bookmarks and encoding across the reload.
    QList<int> bookmarks = editor->bookmarkLines();
    QString encoding = editor->encoding();

    int idx = m_tabManager->indexOf(editor);
    if (idx >= 0)
    {
        m_tabManager->closeEditor(idx);
    }
    m_fileWatcherManager->unwatchFile(filePath);
    emit reloadRequested(filePath, encoding, bookmarks);
}
