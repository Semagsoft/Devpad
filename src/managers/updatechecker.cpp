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
#include "updatechecker.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <memory>

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent)
{
}

void UpdateChecker::checkForUpdates()
{
    if (m_checkInProgress)
        return;
    m_checkInProgress = true;

    QNetworkRequest request(QUrl(QStringLiteral("https://api.github.com/repos/Semagsoft/Devpad/releases/latest")));
    request.setRawHeader("User-Agent", "Devpad/" + QCoreApplication::applicationVersion().toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(15000);

    QNetworkReply* reply = m_networkManager.get(request);

    // Read incrementally and abort once the response exceeds the limit; a
    // readAll() after finished() would buffer an unbounded body in RAM first.
    // A real release JSON response is a few kilobytes.
    static constexpr qint64 MaxUpdateResponseSize = 5LL * 1024 * 1024;
    auto data = std::make_shared<QByteArray>();
    auto aborted = std::make_shared<bool>(false);

    connect(reply, &QNetworkReply::readyRead, this,
            [reply, data, aborted]()
            {
                if (*aborted)
                    return;
                *data += reply->readAll();
                if (data->size() > MaxUpdateResponseSize)
                {
                    *aborted = true;
                    reply->abort();
                }
            });

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, data, aborted]()
            {
                if (*aborted)
                {
                    m_checkInProgress = false;
                    reply->deleteLater();
                    emit checkFailed(tr("The update server response was too large."), QString());
                    return;
                }

                // Catch any tail bytes delivered with the finished state
                // instead of a final readyRead signal.
                data->append(reply->readAll());
                handleReply(reply, *data);
            });
}

void UpdateChecker::handleReply(QNetworkReply* reply, const QByteArray& payload)
{
    m_checkInProgress = false;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        emit checkFailed(friendlyFailureMessage(*reply), reply->errorString());
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    const QJsonObject obj = doc.object();
    const QString tag = obj.value(QStringLiteral("tag_name")).toString();
    const QString releaseUrl = obj.value(QStringLiteral("html_url")).toString();
    if (tag.isEmpty() || releaseUrl.isEmpty())
    {
        emit checkFailed(tr("The update server returned an invalid response."), QString());
        return;
    }

    const QString latest = normalizeVersion(tag);
    if (compareVersions(latest, QCoreApplication::applicationVersion()) > 0)
        emit updateAvailable(latest, releaseUrl);
    else
        emit upToDate(latest);
}

QString UpdateChecker::friendlyFailureMessage(const QNetworkReply& reply)
{
    const int status = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // A TLS initialization failure ("TLS initialization failed") surfaces as
    // SslHandshakeFailedError. The most common cause in shipped packages is a
    // broken OpenSSL bundle, where libssl/libcrypto resolve from different
    // environments. Point the user at reinstalling rather than showing the
    // raw Qt error string.
    if (reply.error() == QNetworkReply::SslHandshakeFailedError)
        return tr("A secure connection to the update server could not be established. "
                  "This installation may be missing TLS support; "
                  "please download Devpad again from the website.");

    if (status == 403)
        return tr("The update server refused the request (rate limit). Please try again later.");

    switch (reply.error())
    {
    case QNetworkReply::ConnectionRefusedError:
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::TimeoutError:
    case QNetworkReply::OperationCanceledError:
    case QNetworkReply::UnknownNetworkError:
        return tr("Could not reach the update server. Check your internet connection and try again.");
    default:
        return tr("The update check failed. Please try again later.");
    }
}

int UpdateChecker::compareVersions(const QString& a, const QString& b)
{
    const QStringList partsA = a.split(QLatin1Char('.'));
    const QStringList partsB = b.split(QLatin1Char('.'));
    const int count = qMax(partsA.size(), partsB.size());
    for (int i = 0; i < count; ++i)
    {
        const int valueA = i < partsA.size() ? numericPrefix(partsA[i]) : 0;
        const int valueB = i < partsB.size() ? numericPrefix(partsB[i]) : 0;
        if (valueA != valueB)
            return valueA < valueB ? -1 : 1;
    }
    return 0;
}

int UpdateChecker::numericPrefix(const QString& version)
{
    // Versions may carry non-numeric suffixes ("v1.06-beta", "1.2-rc1").
    // Compare the leading integer run only; QString::toInt() would yield 0
    // for the whole component and misclassify pre-releases.
    int end = 0;
    while (end < version.size() && version.at(end).isDigit())
        ++end;
    return end > 0 ? version.mid(0, end).toInt() : 0;
}

QString UpdateChecker::normalizeVersion(const QString& version)
{
    QString normalized = version.trimmed();
    if (normalized.size() > 1 && (normalized.front() == QLatin1Char('v') || normalized.front() == QLatin1Char('V')))
        normalized.remove(0, 1);
    return normalized;
}
