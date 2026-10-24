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

namespace
{
// Enforced incrementally during transfer; a body larger than this is aborted
// instead of being fully buffered in RAM before being rejected.
constexpr qint64 kMaxRemoteFileSize = 100LL * 1024 * 1024;
} // namespace

RemoteFileService::RemoteFileService(QObject* parent) : QObject(parent)
{
}

RemoteFileService::~RemoteFileService()
{
    auto replies = std::move(m_pendingReplies);
    for (QNetworkReply* reply : replies)
    {
        reply->disconnect();
        reply->abort();
        delete reply;
    }
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
        downloadSsh(url, urlStr);
    else
        downloadHttp(url, urlStr);
}

void RemoteFileService::downloadHttp(const QUrl& url, const QString& urlStr)
{
    emit statusMessage(tr("Downloading %1...").arg(urlStr));

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, true);

    QNetworkReply* reply = m_networkManager.get(request);
    reply->setProperty("remoteUrl", urlStr);
    m_pendingReplies.append(reply);

    // Accumulate incrementally and abort as soon as the transfer exceeds the
    // limit. A readAll() after finished() would buffer the entire body in RAM
    // before the limit could be enforced.
    auto data = std::make_shared<QByteArray>();
    auto aborted = std::make_shared<bool>(false);

    connect(reply, &QNetworkReply::readyRead, this,
            [this, reply, urlStr, data, aborted]()
            {
                if (*aborted)
                    return;

                // Bail before the payload lands in RAM: either the declared
                // Content-Length is already over the limit, or the next chunk
                // would push past it.
                const qint64 declared = reply->header(QNetworkRequest::ContentLengthHeader).toLongLong();
                if (declared > kMaxRemoteFileSize || data->size() + reply->bytesAvailable() > kMaxRemoteFileSize)
                {
                    *aborted = true;
                    reply->abort();
                    emit downloadFailed(urlStr, tr("Remote file too large (limit %1 MB)").arg(kMaxRemoteFileSize / (1024LL * 1024)));
                    return;
                }

                data->append(reply->readAll());
            });

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, urlStr, data, aborted]()
            {
                m_pendingReplies.removeOne(reply);

                if (*aborted)
                {
                    // Failure was already reported when the abort happened.
                    reply->deleteLater();
                    return;
                }

                if (reply->error() != QNetworkReply::NoError)
                {
                    QString err = reply->errorString();
                    reply->deleteLater();
                    emit downloadFailed(urlStr, err);
                    return;
                }

                // Catch any tail bytes delivered with the finished state
                // instead of a final readyRead signal.
                data->append(reply->readAll());
                if (data->size() > kMaxRemoteFileSize)
                {
                    reply->deleteLater();
                    emit downloadFailed(
                        urlStr, tr("Remote file too large (%1 bytes, limit %2 MB)").arg(data->size()).arg(kMaxRemoteFileSize / (1024LL * 1024)));
                    return;
                }

                QString fileName = QUrl(urlStr).fileName();
                reply->deleteLater();
                emit fileDownloaded(urlStr, fileName, *data);
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
    if (!validComponentRegex.match(host).hasMatch() || !validComponentRegex.match(remotePath).hasMatch() ||
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

    emit statusMessage(tr("Downloading %1 via SSH...").arg(urlStr));

    QStringList args;
    if (port != 22)
    {
        args << "-P" << QString::number(port);
    }
    args << QString("%1@%2:%3").arg(user, host, remotePath);

    // Start the transfer on the next event-loop iteration. Deferring the
    // download attempt keeps the synchronous part of downloadSsh() limited to
    // URL validation and the status message, so failures (e.g. temp file or
    // process start errors) are never reported synchronously from within
    // openRemote(); some platforms deliver QProcess start failures synchronously
    // from inside start().
    QTimer::singleShot(0, this,
                       [this, urlStr, args]()
                       {
                           QPointer<RemoteFileService> guard(this);
                           if (!guard)
                               return;
                           startSshTransfer(urlStr, args);
                       });
}

void RemoteFileService::startSshTransfer(const QString& urlStr, const QStringList& args)
{
    auto* tmpFile = new QTemporaryFile(QDir::temp().filePath("devpad_remote_XXXXXX"));
    tmpFile->setAutoRemove(true);
    if (!tmpFile->open())
    {
        tmpFile->deleteLater();
        emit downloadFailed(urlStr, tr("Failed to create temp file."));
        return;
    }
    QString localPath = tmpFile->fileName();
    tmpFile->close();

    std::shared_ptr<QTemporaryFile> tmpFileShared(tmpFile);
    QStringList scpArgs = args;
    scpArgs << localPath;

    auto* scp = new QProcess(this);
    scp->setProperty("remoteUrl", urlStr);
    QPointer<RemoteFileService> guard(this);

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
                if (file.size() > kMaxRemoteFileSize)
                {
                    emit downloadFailed(
                        urlStr, tr("Remote file too large (%1 bytes, limit %2 MB)").arg(file.size()).arg(kMaxRemoteFileSize / (1024LL * 1024)));
                    return;
                }
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
                if (guard)
                {
                    guard->stop();
                    guard->deleteLater();
                }
            });
    connect(scp, &QProcess::errorOccurred, this,
            [killTimer](QProcess::ProcessError)
            {
                QPointer<QTimer> guard(killTimer);
                if (guard)
                {
                    guard->stop();
                    guard->deleteLater();
                }
            });

    scp->start("scp", scpArgs);
    killTimer->start(30000);
}
