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
#ifndef REMOTEFILESERVICE_H
#define REMOTEFILESERVICE_H

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <memory>

class QTemporaryFile;

class RemoteFileService : public QObject
{
    Q_OBJECT

public:
    explicit RemoteFileService(QObject* parent = nullptr);

    void openRemote(const QString& urlStr);

signals:
    void fileDownloaded(const QString& url, const QString& fileName, const QByteArray& data);
    void downloadFailed(const QString& url, const QString& error);
    void downloadProgress(const QString& url, qint64 received, qint64 total);
    void statusMessage(const QString& message);

private:
    void downloadHttp(const QUrl& url, const QString& urlStr);
    void downloadSsh(const QUrl& url, const QString& urlStr);

    QNetworkAccessManager m_networkManager;
};

#endif // REMOTEFILESERVICE_H
