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
#include "remotefileservice.h"

#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QTimer>
#include <QUrl>
#include <memory>

RemoteFileService::RemoteFileService(QObject* parent) : QObject(parent)
{
}

void RemoteFileService::openRemote(const QString& urlStr)
{
    QUrl url(urlStr);
    if (!url.isValid())
    {
        emit downloadFailed(urlStr, tr("The URL \"%1\" is not valid.").arg(urlStr));
        return;
    }

    bool isSsh = urlStr.startsWith("ssh://", Qt::CaseInsensitive);
    if (isSsh)
    {
        downloadSsh(url, urlStr);
    }
    else
    {
        downloadHttp(url, urlStr);
    }
}

void RemoteFileService::downloadHttp(const QUrl& url, const QString& urlStr)
{
    emit statusMessage(tr("Downloading %1...").arg(urlStr));

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_networkManager.get(request);
    reply->setProperty("remoteUrl", urlStr);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply]()
            {
                QString urlStr = reply->property("remoteUrl").toString();

                if (reply->error() != QNetworkReply::NoError)
                {
                    QString err = reply->errorString();
                    reply->deleteLater();
                    emit downloadFailed(urlStr, err);
                    return;
                }

                QByteArray data = reply->readAll();
                QString fileName = QUrl(urlStr).fileName();
                reply->deleteLater();
                emit fileDownloaded(urlStr, fileName, data);
            });

    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, urlStr](qint64 received, qint64 total)
            {
                if (total > 0)
                {
                    emit downloadProgress(urlStr, received, total);
                }
            });
}

void RemoteFileService::downloadSsh(const QUrl& url, const QString& urlStr)
{
    emit statusMessage(tr("Downloading %1 via SSH...").arg(urlStr));

    QString user = url.userName();
    QString host = url.host();
    int port = url.port(22);
    QString remotePath = url.path();

    if (host.isEmpty() || remotePath.isEmpty())
    {
        emit downloadFailed(urlStr, tr("SSH URL must include host and path.\n"
                                       "Example: ssh://user@host/path/to/file"));
        return;
    }

    static const QRegularExpression validComponentRegex("^[a-zA-Z0-9@._:/=\\-]+$");
    if (!validComponentRegex.match(host).hasMatch() ||
        !validComponentRegex.match(remotePath).hasMatch() ||
        (!user.isEmpty() && !validComponentRegex.match(user).hasMatch()))
    {
        emit downloadFailed(urlStr, tr("SSH URL contains invalid characters."));
        return;
    }

    QString checkPath = remotePath.startsWith('/') ? remotePath.mid(1) : remotePath;
    if (host.startsWith('-') || checkPath.startsWith('-') || user.startsWith('-'))
    {
        emit downloadFailed(urlStr, tr("Invalid SSH URL: host, user, or path starts with '-'."));
        return;
    }

    if (user.isEmpty())
    {
        user = QDir::home().dirName();
    }

    auto tmpFile = std::make_unique<QTemporaryFile>(QDir::temp().filePath("devpad_remote_XXXXXX"));
    tmpFile->setAutoRemove(true);
    if (!tmpFile->open())
    {
        emit downloadFailed(urlStr, tr("Failed to create temp file."));
        return;
    }
    QString localPath = tmpFile->fileName();
    tmpFile->close();

    QStringList args;
    if (port != 22)
    {
        args << "-P" << QString::number(port);
    }
    args << QString("%1@%2:%3").arg(user, host, remotePath) << localPath;

    auto* scp = new QProcess(this);
    scp->setProperty("remoteUrl", urlStr);
    QPointer<RemoteFileService> guard(this);
    std::shared_ptr<QTemporaryFile> tmpFileShared(tmpFile.release());

    connect(scp, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, scp, tmpFileShared, urlStr, guard](int exitCode, QProcess::ExitStatus status)
            {
                if (!guard)
                    return;
                scp->deleteLater();

                if (exitCode != 0 || status != QProcess::NormalExit)
                {
                    QString err = QString::fromUtf8(scp->readAllStandardError());
                    emit downloadFailed(urlStr, err.isEmpty() ? tr("Unknown error") : err);
                    return;
                }

                QString filePath = tmpFileShared->fileName();
                QFile file(filePath);
                QByteArray data;
                if (file.open(QIODevice::ReadOnly))
                {
                    data = file.readAll();
                    file.close();
                }

                if (data.isEmpty())
                {
                    emit downloadFailed(urlStr, tr("Failed to read downloaded file."));
                    return;
                }

                QString fileName = QUrl(urlStr).fileName();
                emit fileDownloaded(urlStr, fileName, data);
            });

    connect(scp, &QProcess::errorOccurred, this,
            [this, scp, tmpFileShared, guard](QProcess::ProcessError)
            {
                if (!guard)
                    return;
                QString errMsg = QString::fromUtf8(scp->readAllStandardError());
                QString urlStr = scp->property("remoteUrl").toString();
                scp->deleteLater();
                emit downloadFailed(urlStr, errMsg.isEmpty() ? tr("Failed to start SCP process.") : errMsg);
            });

    scp->start("scp", args);

    auto* killTimer = new QTimer(this);
    killTimer->setSingleShot(true);
    connect(killTimer, &QTimer::timeout, this,
            [scp, killTimer]()
            {
                QPointer<QTimer> guard(killTimer);
                if (scp->state() != QProcess::NotRunning)
                {
                    scp->kill();
                }
                if (guard)
                    guard->deleteLater();
            });
    connect(scp, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [killTimer](int, QProcess::ExitStatus)
            {
                QPointer<QTimer> guard(killTimer);
                if (guard) {
                    guard->stop();
                    guard->deleteLater();
                }
            });
    connect(scp, &QProcess::errorOccurred, this,
            [killTimer](QProcess::ProcessError)
            {
                QPointer<QTimer> guard(killTimer);
                if (guard) {
                    guard->stop();
                    guard->deleteLater();
                }
            });
    killTimer->start(30000);
}
