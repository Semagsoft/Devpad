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

    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReply(reply); });
}

void UpdateChecker::handleReply(QNetworkReply* reply)
{
    m_checkInProgress = false;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        emit checkFailed(reply->errorString());
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    const QJsonObject obj = doc.object();
    const QString tag = obj.value(QStringLiteral("tag_name")).toString();
    const QString releaseUrl = obj.value(QStringLiteral("html_url")).toString();
    if (tag.isEmpty() || releaseUrl.isEmpty())
    {
        emit checkFailed(tr("The update server returned an invalid response."));
        return;
    }

    const QString latest = normalizeVersion(tag);
    if (compareVersions(latest, QCoreApplication::applicationVersion()) > 0)
        emit updateAvailable(latest, releaseUrl);
    else
        emit upToDate(latest);
}

int UpdateChecker::compareVersions(const QString& a, const QString& b)
{
    const QStringList partsA = a.split(QLatin1Char('.'));
    const QStringList partsB = b.split(QLatin1Char('.'));
    const int count = qMax(partsA.size(), partsB.size());
    for (int i = 0; i < count; ++i)
    {
        const int valueA = i < partsA.size() ? partsA[i].toInt() : 0;
        const int valueB = i < partsB.size() ? partsB[i].toInt() : 0;
        if (valueA != valueB)
            return valueA < valueB ? -1 : 1;
    }
    return 0;
}

QString UpdateChecker::normalizeVersion(const QString& version)
{
    QString normalized = version.trimmed();
    if (normalized.size() > 1 && (normalized.front() == QLatin1Char('v') || normalized.front() == QLatin1Char('V')))
        normalized.remove(0, 1);
    return normalized;
}
