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
 *
 * ── OpenSSL Requirement ──────────────────────────────────────────
 *
 * HTTPS downloads use QNetworkAccessManager which depends on OpenSSL
 * at runtime. On Linux, install openssl (or libssl-dev / openssl-devel).
 * Without it, HTTPS requests will fail silently with an SSL handshake
 * error. HTTP (non-TLS) and SSH (SCP) transports do not require OpenSSL.
 */
#ifndef REMOTEFILESERVICE_H
#define REMOTEFILESERVICE_H

#include "remotefsentry.h"

#include <QByteArray>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <memory>

class QTemporaryFile;
class QNetworkReply;

class RemoteFileService : public QObject
{
    Q_OBJECT

public:
    explicit RemoteFileService(QObject* parent = nullptr);
    ~RemoteFileService() override;

    void openRemote(const QString& urlStr);
    // Lists a remote folder over SSH (one level); the project panel loads
    // sub-directories lazily via listRemoteDir().
    void openRemoteFolder(const QString& urlStr);
    void listRemoteDir(const QString& urlStr, const QString& remotePath);

    // Uploads `data` to the remote SSH file at `urlStr` (scp). Blocks until the
    // transfer completes; returns true on success, false on failure with `error`
    // populated. Emits fileUploaded() on success.
    bool saveRemote(const QString& urlStr, const QByteArray& data, QString* error = nullptr);

    // Parses raw `find` listing output (dirs/files sections split by the
    // __DEVPAD_FILES__ marker) — public for unit tests.
    static QList<RemoteFsEntry> parseListing(const QString& output, const QString& parentPath);

signals:
    void fileDownloaded(const QString& url, const QString& fileName, const QByteArray& data);
    void fileUploaded(const QString& url);
    void remoteDirListed(const QString& url, const QString& parentPath, const QList<RemoteFsEntry>& entries);
    void remoteListingFailed(const QString& url, const QString& remotePath, const QString& error);
    void downloadFailed(const QString& url, const QString& error);
    void downloadProgress(const QString& url, qint64 received, qint64 total);
    void statusMessage(const QString& message);

private:
    void downloadHttp(const QUrl& url, const QString& urlStr);
    void downloadSsh(const QUrl& url, const QString& urlStr);
    void startSshTransfer(const QString& urlStr, const QStringList& args);
    QString validateSshTarget(const QUrl& url, const QString& urlStr);
    QStringList sshBaseArgs(const QUrl& url, const QString& user);
    void startSshListing(const QString& urlStr, const QString& remotePath, const QStringList& baseArgs);
    QString findSshPath();
    QString findScpPath();
    static QString shellQuote(const QString& path);

    QNetworkAccessManager m_networkManager;
    QList<QNetworkReply*> m_pendingReplies;
};

#endif // REMOTEFILESERVICE_H
