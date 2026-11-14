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
#include "remoteopencoordinator.h"

#include "codeeditor.h"
#include "editorcontroller.h"
#include "encodingutils.h"
#include "filemanager.h"
#include "logger.h"
#include "managers/remotefileservice.h"
#include "managers/settingsmanager.h"
#include "projectpanel.h"
#include "tabmanager.h"

#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QStringConverter>
#include <QStringDecoder>
#include <QUrl>

RemoteOpenCoordinator::RemoteOpenCoordinator(RemoteFileService* remoteFileService, ProjectPanel* projectPanel, TabManager* tabManager,
                                             EditorController* editorController, QWidget* uiParent, QObject* parent)
    : QObject(parent), m_remoteFileService(remoteFileService), m_projectPanel(projectPanel), m_tabManager(tabManager),
      m_editorController(editorController), m_uiParent(uiParent)
{
}

void RemoteOpenCoordinator::openRemote()
{
    bool ok;
    QString urlStr = QInputDialog::getText(m_uiParent, QObject::tr("Open Remote File"), QObject::tr("Enter remote file URL\n(HTTP/HTTPS or SSH):"),
                                           QLineEdit::Normal, "https://", &ok);
    if (!ok || urlStr.trimmed().isEmpty())
        return;

    urlStr = urlStr.trimmed();

    bool isSsh = urlStr.startsWith("ssh://", Qt::CaseInsensitive);

    if (!isSsh && !urlStr.startsWith("http://", Qt::CaseInsensitive) && !urlStr.startsWith("https://", Qt::CaseInsensitive))
    {
        urlStr = "https://" + urlStr;
    }

    QUrl url(urlStr);
    if (!url.isValid())
    {
        QMessageBox::warning(m_uiParent, QObject::tr("Invalid URL"), QObject::tr("The URL \"%1\" is not valid.").arg(urlStr));
        return;
    }

    m_remoteFileService->openRemote(urlStr);
}

void RemoteOpenCoordinator::openRemoteFolder()
{
    bool ok;
    QString urlStr = QInputDialog::getText(m_uiParent, QObject::tr("Open Remote Folder"),
                                           QObject::tr("Enter remote folder URL\n(SSH only, e.g. ssh://user@host/path/to/folder):"),
                                           QLineEdit::Normal, "ssh://", &ok);
    if (!ok || urlStr.trimmed().isEmpty())
        return;

    urlStr = urlStr.trimmed();

    if (!urlStr.startsWith("ssh://", Qt::CaseInsensitive))
    {
        QMessageBox::warning(m_uiParent, QObject::tr("Invalid URL"), QObject::tr("Remote folders require an SSH URL (ssh://user@host/path)."));
        return;
    }

    QUrl url(urlStr);
    if (!url.isValid())
    {
        QMessageBox::warning(m_uiParent, QObject::tr("Invalid URL"), QObject::tr("The URL \"%1\" is not valid.").arg(urlStr));
        return;
    }

    m_remoteFileService->openRemoteFolder(urlStr);
}

void RemoteOpenCoordinator::openRemoteFileActivated(const QString& urlStr)
{
    // Reuse the already-open tab for this remote file instead of re-downloading.
    if (CodeEditor* existing = m_tabManager->findEditorByFileName(urlStr))
    {
        m_tabManager->focusEditorPane(existing);
        existing->setFocus();
        return;
    }

    m_remoteFileService->openRemote(urlStr);
}

void RemoteOpenCoordinator::openRemoteFileFromData(const QString& urlStr, const QString& fileName, const QByteArray& data)
{
    BomResult bom = detectBom(data);
    QString detectedEnc = FileManager::detectEncoding(data);
    QStringDecoder decoder(encodingFromName(detectedEnc));
    QString content = decoder(bom.size > 0 ? data.mid(bom.size) : data);

    QString displayName = fileName.isEmpty() ? QUrl(urlStr).fileName() : fileName;
    if (displayName.isEmpty())
    {
        displayName = QObject::tr("Remote");
    }

    CodeEditor* editor = m_tabManager->createEditor();
    editor->setFileName(urlStr);
    editor->setEncoding(detectedEnc);
    editor->setText(content);
    editor->setModified(false);

    m_editorController->connectEditorSignals(editor);

    SettingsManager::instance().applyToEditor(editor);

    QString syntax = SettingsManager::instance().syntaxForFile(displayName);
    if (!syntax.isEmpty())
    {
        editor->setSyntax(syntax);
    }

    m_tabManager->addEditor(editor, displayName);
    m_editorController->updateUndoRedoState();
    emit fileTypeLabelDirty();

    Logger::instance().info(QString("Opened remote file: %1").arg(urlStr));
}
