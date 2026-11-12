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
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "codeeditor.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

class RemoteFileService;

class FileManager : public QObject
{
    Q_OBJECT

public:
    explicit FileManager(QObject* parent = nullptr);
    ~FileManager() override = default;

    // saveFile() reports ordinary (local) saves synchronously. For ssh://
    // targets the transfer runs asynchronously: saveFile() returns true once
    // the upload has been started, and the outcome is delivered via
    // remoteSaveFinished() (followed by fileSaved() on success or error()
    // on failure). Use remoteSaveFinished() to track completion of remote
    // saves.
    bool loadFile(const QString& fileName, CodeEditor* editor, const QString& encoding = QString());
    bool saveFile(const QString& fileName, CodeEditor* editor, const QString& encoding = QString());

    // Injects the RemoteFileService so ssh:// file names can be saved over SCP.
    void setRemoteFileService(RemoteFileService* service);

    const QString& lastError() const
    {
        return m_lastError;
    }

    // True while a remote (ssh://) save initiated by saveFile() is still in flight.
    bool hasPendingRemoteSave(const QString& fileName) const
    {
        return m_pendingRemoteSaves.contains(fileName);
    }

    static QString detectEncoding(const QByteArray& buffer);

signals:
    void fileLoaded(const QString& fileName);
    void fileSaved(const QString& fileName);
    void error(const QString& message);
    // Delivered when an asynchronous remote (ssh://) save started by
    // saveFile() finishes. `ok` is false on failure, with `errorDetail`
    // populated (falling back to the target URL when empty).
    void remoteSaveFinished(bool ok, const QString& fileName, const QString& errorDetail);

private:
    bool saveRemoteFile(const QString& fileName, CodeEditor* editor, const QString& encoding);
    void completeRemoteSave(const QString& url, bool ok, const QString& errorDetail);

    QString m_lastError;
    RemoteFileService* m_remoteFileService = nullptr;
    // Editors with a remote save in flight, keyed by the target URL. Multiple
    // saves of the same file (e.g. re-saves) are all accounted for on
    // completion.
    QHash<QString, QList<QPointer<CodeEditor>>> m_pendingRemoteSaves;
    QMetaObject::Connection m_remoteUploadedConn;
    QMetaObject::Connection m_remoteFailedConn;
};

#endif // FILEMANAGER_H
