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
#include "remotefileservice.h"

#include "logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
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

// Marker emitted between the directories and files sections of the listing
// command (see startSshListing).
constexpr const char* kListingFilesMarker = "__DEVPAD_FILES__";

// Cap on the stdout of a directory listing to bail out of runaway listings.
constexpr qint64 kMaxListingBytes = 10LL * 1024 * 1024;
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

void RemoteFileService::openRemoteFolder(const QString& urlStr)
{
    QUrl url(urlStr);
    if (!url.isValid())
    {
        emit downloadFailed(urlStr, tr("The URL \"%1\" is not valid.").arg(urlStr));
        return;
    }

    if (!urlStr.startsWith("ssh://", Qt::CaseInsensitive))
    {
        emit downloadFailed(urlStr, tr("Remote folders are only supported over SSH. "
                                       "Use an ssh:// URL, e.g. ssh://user@host/path/to/folder."));
        return;
    }

    QString user = validateSshTarget(url, urlStr);
    if (user.isEmpty())
        return;

    QString remotePath = url.path();
    while (remotePath.size() > 1 && remotePath.endsWith('/'))
        remotePath.chop(1);
    if (remotePath.isEmpty() || !remotePath.startsWith('/'))
    {
        emit downloadFailed(urlStr, tr("SSH URL must include a folder path.\n"
                                       "Example: ssh://user@host/path/to/folder"));
        return;
    }

    emit statusMessage(tr("Listing remote folder via SSH..."));
    Logger::instance().info(QStringLiteral("Remote folder listing: %1").arg(urlStr));

    QTimer::singleShot(0, this,
                       [this, urlStr, remotePath, url, user]()
                       {
                           QPointer<RemoteFileService> guard(this);
                           if (!guard)
                               return;
                           startSshListing(urlStr, remotePath, sshBaseArgs(url, user));
                       });
}

void RemoteFileService::listRemoteDir(const QString& urlStr, const QString& remotePath)
{
    QUrl url(urlStr);
    QString cleaned = remotePath;
    while (cleaned.size() > 1 && cleaned.endsWith('/'))
        cleaned.chop(1);

    if (!url.isValid() || url.host().isEmpty() || cleaned.isEmpty() || !cleaned.startsWith('/') || cleaned.contains(QLatin1Char('\n')))
    {
        emit remoteListingFailed(urlStr, cleaned, tr("Invalid SSH URL or remote path."));
        emit downloadFailed(urlStr, tr("Invalid SSH URL or remote path."));
        return;
    }

    // The remote path comes from our own tree (it may legitimately contain
    // spaces or unicode), so only host/user components are strictly checked.
    static const QRegularExpression validComponentRegex("^[a-zA-Z0-9@._=\\-]+$");
    QString user = url.userName();
    QString host = url.host();
    if (!validComponentRegex.match(host).hasMatch() || (!user.isEmpty() && !validComponentRegex.match(user).hasMatch()))
    {
        emit remoteListingFailed(urlStr, cleaned, tr("SSH URL contains invalid characters."));
        emit downloadFailed(urlStr, tr("SSH URL contains invalid characters."));
        return;
    }

    if (host.startsWith('-') || user.startsWith('-'))
    {
        emit remoteListingFailed(urlStr, cleaned, tr("Invalid SSH URL: host or user starts with '-'."));
        emit downloadFailed(urlStr, tr("Invalid SSH URL: host or user starts with '-'."));
        return;
    }

    if (user.isEmpty())
        user = QDir::home().dirName();

    emit statusMessage(tr("Listing %1 via SSH...").arg(cleaned));

    QTimer::singleShot(0, this,
                       [this, urlStr, cleaned, url, user]()
                       {
                           QPointer<RemoteFileService> guard(this);
                           if (!guard)
                               return;
                           startSshListing(urlStr, cleaned, sshBaseArgs(url, user));
                       });
}

QList<RemoteFsEntry> RemoteFileService::parseListing(const QString& output, const QString& parentPath)
{
    QList<RemoteFsEntry> entries;

    QString parent = parentPath;
    while (parent.size() > 1 && parent.endsWith('/'))
        parent.chop(1);

    bool files = false;
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (QString line : lines)
    {
        while (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        if (line.isEmpty())
            continue;
        if (line == QLatin1String(kListingFilesMarker))
        {
            files = true;
            continue;
        }
        if (line.startsWith(QStringLiteral("__DEVPAD")))
            continue; // ignore stray marker-like output
        if (line.startsWith(QStringLiteral("./")))
            line.remove(0, 2);
        if (line.isEmpty() || line == QLatin1String(".") || line == QLatin1String(".."))
            continue;

        QString full = parent.endsWith(QLatin1Char('/')) ? parent + line : parent + QLatin1Char('/') + line;
        entries.append(RemoteFsEntry{full, !files});
    }
    return entries;
}

// Validates host/user of an ssh:// URL (path checks are caller-specific).
// Returns the user with the default applied, or an empty string after
// emitting downloadFailed.
QString RemoteFileService::validateSshTarget(const QUrl& url, const QString& urlStr)
{
    QString user = url.userName();
    QString host = url.host();

    if (host.isEmpty())
    {
        emit downloadFailed(urlStr, tr("SSH URL must include host and path.\n"
                                       "Example: ssh://user@host/path"));
        return {};
    }

    static const QRegularExpression validComponentRegex("^[a-zA-Z0-9@._:/=\\-]+$");
    if (!validComponentRegex.match(host).hasMatch() || (!user.isEmpty() && !validComponentRegex.match(user).hasMatch()))
    {
        emit downloadFailed(urlStr, tr("SSH URL contains invalid characters."));
        return {};
    }

    if (host.startsWith('-') || user.startsWith('-'))
    {
        emit downloadFailed(urlStr, tr("Invalid SSH URL: host, user, or path starts with '-'."));
        return {};
    }

    if (user.isEmpty())
        user = QDir::home().dirName();

    return user;
}

QStringList RemoteFileService::sshBaseArgs(const QUrl& url, const QString& user)
{
    int port = url.port(22);
    QStringList args;
    if (port != 22)
        args << "-p" << QString::number(port);
    // BatchMode prevents interactive prompts (password/host-key) without a
    // TTY; ConnectTimeout fails fast on unreachable hosts.
    args << "-o" << "BatchMode=yes" << "-o" << "ConnectTimeout=15";
    args << QString("%1@%2").arg(user, url.host());
    return args;
}

void RemoteFileService::startSshListing(const QString& urlStr, const QString& remotePath, const QStringList& baseArgs)
{
    QString sshPath = findSshPath();
    if (sshPath.isEmpty())
    {
        emit remoteListingFailed(urlStr, remotePath, tr("ssh not found on PATH. Install openssh-client."));
        emit downloadFailed(urlStr, tr("ssh not found on PATH. Install openssh-client."));
        return;
    }

    // One command prints directories, then the marker, then files. The path is
    // single-quoted so spaces and odd characters survive the remote shell.
    const QString command = QStringLiteral("cd %1 && find . ! -name . -maxdepth 1 -type d -print && echo %2 && find . -maxdepth 1 -type f -print")
                                .arg(shellQuote(remotePath), QLatin1String(kListingFilesMarker));

    auto* ssh = new QProcess(this);
    ssh->setProgram(sshPath);
    QStringList args = baseArgs;
    args << command;
    ssh->setArguments(args);
    ssh->setProperty("remoteUrl", urlStr);
    ssh->setProperty("remotePath", remotePath);

    QPointer<RemoteFileService> guard(this);
    QPointer<QProcess> procGuard(ssh);
    std::shared_ptr<bool> processed = std::make_shared<bool>(false);
    auto stdoutBuf = std::make_shared<QByteArray>();
    auto stderrBuf = std::make_shared<QByteArray>();

    // Disconnect and close the process without deleteLater() — calling
    // deleteLater() from within a QProcess's own signal handlers can cause a
    // use-after-free when both finished() and errorOccurred() fire. The
    // QProcess is a child of RemoteFileService and will be cleaned up by its
    // destructor.
    auto finishProcess = [ssh, procGuard]()
    {
        if (procGuard)
        {
            ssh->disconnect();
            ssh->close();
        }
    };

    auto reportFailure = [this, guard, urlStr, remotePath, stderrBuf](const QString& fallback)
    {
        if (!guard)
            return;
        QString err = QString::fromUtf8(*stderrBuf).trimmed();
        emit remoteListingFailed(urlStr, remotePath, err.isEmpty() ? fallback : err);
        emit downloadFailed(urlStr, err.isEmpty() ? fallback : err);
    };

    connect(ssh, &QProcess::readyReadStandardOutput, this,
            [ssh, procGuard, guard, processed, stdoutBuf, urlStr, remotePath]()
            {
                if (!guard || !procGuard || *processed)
                    return;
                stdoutBuf->append(ssh->readAllStandardOutput());
                if (stdoutBuf->size() > kMaxListingBytes)
                {
                    *processed = true;
                    ssh->disconnect();
                    ssh->kill();
                    QString msg = tr("Directory listing too large.");
                    emit guard->remoteListingFailed(urlStr, remotePath, msg);
                    emit guard->downloadFailed(urlStr, msg);
                }
            });

    connect(ssh, &QProcess::readyReadStandardError, this,
            [ssh, procGuard, stderrBuf]()
            {
                if (!procGuard)
                    return;
                stderrBuf->append(ssh->readAllStandardError());
            });

    connect(ssh, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [ssh, procGuard, guard, processed, finishProcess, reportFailure, stdoutBuf](int exitCode, QProcess::ExitStatus status)
            {
                if (!guard || !procGuard || *processed)
                    return;
                *processed = true;
                finishProcess();

                if (exitCode != 0 || status != QProcess::NormalExit)
                {
                    reportFailure(tr("ssh exited with code %1").arg(exitCode));
                    return;
                }

                QList<RemoteFsEntry> entries = parseListing(QString::fromUtf8(*stdoutBuf), ssh->property("remotePath").toString());
                emit guard->remoteDirListed(ssh->property("remoteUrl").toString(), ssh->property("remotePath").toString(), entries);
            });

    connect(ssh, &QProcess::errorOccurred, this,
            [guard, processed, finishProcess, reportFailure](QProcess::ProcessError error)
            {
                if (!guard || *processed)
                    return;
                *processed = true;
                finishProcess();

                QString reason;
                switch (error)
                {
                case QProcess::FailedToStart:
                    reason = tr("Failed to start ssh process.");
                    break;
                case QProcess::Crashed:
                    reason = tr("ssh process crashed.");
                    break;
                case QProcess::Timedout:
                    reason = tr("ssh process timed out.");
                    break;
                case QProcess::WriteError:
                    reason = tr("Write error to ssh process.");
                    break;
                case QProcess::ReadError:
                    reason = tr("Read error from ssh process.");
                    break;
                default:
                    reason = tr("Unknown ssh process error.");
                    break;
                }
                reportFailure(reason);
            });

    auto* killTimer = new QTimer(ssh);
    killTimer->setSingleShot(true);
    connect(killTimer, &QTimer::timeout, ssh,
            [ssh, guard, processed]()
            {
                if (!guard || *processed)
                    return;
                *processed = true;
                ssh->disconnect();
                if (ssh->state() != QProcess::NotRunning)
                    ssh->kill();
                QString msg = tr("Directory listing timed out.");
                emit guard->remoteListingFailed(ssh->property("remoteUrl").toString(), ssh->property("remotePath").toString(), msg);
                emit guard->downloadFailed(ssh->property("remoteUrl").toString(), msg);
            });

    ssh->start();
    killTimer->start(30000);
}

QString RemoteFileService::findSshPath()
{
    // Try common install locations first (GUI launches may have a limited PATH).
    const QStringList candidates = {
        QStringLiteral("/usr/bin/ssh"),
        QStringLiteral("/bin/ssh"),
        QStringLiteral("/usr/local/bin/ssh"),
        QStringLiteral("/opt/homebrew/bin/ssh"),
    };
    for (const QString& path : candidates)
    {
        QFileInfo fi(path);
        if (fi.exists() && fi.isExecutable())
            return path;
    }
    // Fall back to PATH lookup.
    return QStringLiteral("ssh");
}

QString RemoteFileService::findScpPath()
{
    // Try common install locations first (GUI launches may have a limited PATH).
    const QStringList candidates = {
        QStringLiteral("/usr/bin/scp"),
        QStringLiteral("/bin/scp"),
        QStringLiteral("/usr/local/bin/scp"),
        QStringLiteral("/opt/homebrew/bin/scp"),
    };
    for (const QString& path : candidates)
    {
        QFileInfo fi(path);
        if (fi.exists() && fi.isExecutable())
            return path;
    }
    // Fall back to PATH lookup.
    return QStringLiteral("scp");
}

QString RemoteFileService::shellQuote(const QString& path)
{
    QString escaped = path;
    escaped.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
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

    // Remote paths may legitimately contain spaces/unicode (they arrive from
    // remote listings), so only host/user are strictly checked; the path must
    // merely be free of shell-breaking control characters.
    static const QRegularExpression validComponentRegex("^[a-zA-Z0-9@._:/=\\-]+$");
    if (!validComponentRegex.match(host).hasMatch() || (!user.isEmpty() && !validComponentRegex.match(user).hasMatch()))
    {
        emit downloadFailed(urlStr, tr("SSH URL contains invalid characters."));
        return;
    }
    static const QRegularExpression controlCharsRegex(R"([\x00-\x1f\x7f])");
    if (controlCharsRegex.match(remotePath).hasMatch())
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
        args << "-P" << QString::number(port);
    // BatchMode prevents interactive prompts (password/host-key) without a
    // TTY; ConnectTimeout fails fast on unreachable hosts.
    args << "-o" << "BatchMode=yes" << "-o" << "ConnectTimeout=15";
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
        delete tmpFile;
        emit downloadFailed(urlStr, tr("Failed to create temp file."));
        return;
    }
    QString localPath = tmpFile->fileName();
    tmpFile->close();
    std::shared_ptr<QTemporaryFile> tmpFileShared(tmpFile);

    QStringList scpArgs = args;
    scpArgs << localPath;
    QString scpPath = findScpPath();

    auto* scp = new QProcess(this);
    scp->setProgram(scpPath);
    scp->setArguments(scpArgs);
    scp->setProperty("remoteUrl", urlStr);

    QPointer<RemoteFileService> guard(this);
    QPointer<QProcess> procGuard(scp);
    std::shared_ptr<bool> processed = std::make_shared<bool>(false);
    auto stderrBuf = std::make_shared<QByteArray>();

    // Disconnect and close the process without deleteLater() — calling
    // deleteLater() from within a QProcess's own signal handlers can cause a
    // use-after-free when both finished() and errorOccurred() fire. The
    // QProcess is a child of RemoteFileService and will be cleaned up by its
    // destructor.
    auto finishProcess = [scp, procGuard]()
    {
        if (procGuard)
        {
            scp->disconnect();
            scp->close();
        }
    };

    auto reportFailure = [this, guard, urlStr, stderrBuf](const QString& fallback)
    {
        if (!guard)
            return;
        QString err = QString::fromUtf8(*stderrBuf).trimmed();
        emit downloadFailed(urlStr, err.isEmpty() ? fallback : err);
    };

    connect(scp, &QProcess::readyReadStandardError, this,
            [scp, procGuard, stderrBuf]()
            {
                if (!procGuard)
                    return;
                stderrBuf->append(scp->readAllStandardError());
            });

    connect(scp, &QProcess::readyReadStandardOutput, this,
            [scp, procGuard, guard, urlStr]()
            {
                if (!guard || !procGuard)
                    return;
                // scp prints the current file being copied; surface the last
                // line as progress feedback.
                QString text = QString::fromUtf8(scp->readAllStandardOutput()).trimmed();
                if (!text.isEmpty())
                {
                    QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                    if (!lines.isEmpty())
                        emit guard->statusMessage(tr("Copying: %1").arg(lines.last()));
                }
            });

    connect(scp, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [procGuard, guard, processed, finishProcess, reportFailure, tmpFileShared, urlStr](int exitCode, QProcess::ExitStatus status)
            {
                if (!guard || !procGuard || *processed)
                    return;
                *processed = true;
                finishProcess();

                if (exitCode != 0 || status != QProcess::NormalExit)
                {
                    reportFailure(tr("Unknown error"));
                    return;
                }

                QString filePath = tmpFileShared->fileName();
                QFile file(filePath);
                if (file.size() > kMaxRemoteFileSize)
                {
                    emit guard->downloadFailed(
                        urlStr, tr("Remote file too large (%1 bytes, limit %2 MB)").arg(file.size()).arg(kMaxRemoteFileSize / (1024LL * 1024)));
                    return;
                }

                // scp exited 0, so the file exists; it may legitimately be
                // empty (0 bytes) — that is a successful download of an empty
                // file, not an error.
                QString fileName = QUrl(urlStr).fileName();
                if (!file.open(QIODevice::ReadOnly))
                {
                    emit guard->downloadFailed(urlStr, tr("Failed to read downloaded file."));
                    return;
                }
                QByteArray data = file.readAll();
                file.close();
                emit guard->fileDownloaded(urlStr, fileName, data);
            });

    connect(scp, &QProcess::errorOccurred, this,
            [guard, processed, finishProcess, reportFailure](QProcess::ProcessError error)
            {
                if (!guard || *processed)
                    return;
                *processed = true;
                finishProcess();

                QString reason;
                switch (error)
                {
                case QProcess::FailedToStart:
                    reason = tr("Failed to start SCP process.");
                    break;
                case QProcess::Crashed:
                    reason = tr("SCP process crashed.");
                    break;
                case QProcess::Timedout:
                    reason = tr("SCP process timed out.");
                    break;
                default:
                    reason = tr("Unknown SCP process error.");
                    break;
                }
                reportFailure(reason);
            });

    auto* killTimer = new QTimer(scp);
    killTimer->setSingleShot(true);
    connect(killTimer, &QTimer::timeout, scp,
            [scp, guard, processed]()
            {
                if (!guard || *processed)
                    return;
                *processed = true;
                scp->disconnect();
                if (scp->state() != QProcess::NotRunning)
                    scp->kill();
                emit guard->downloadFailed(scp->property("remoteUrl").toString(), tr("SCP transfer timed out."));
            });

    scp->start();
    killTimer->start(30000);
}
