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
#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSettings>
#include <QString>

class QNetworkReply;

class UpdateChecker : public QObject
{
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);

    // Returns true if a network check was actually started, false if a check
    // is already in progress or the minimum interval since the last check has
    // not elapsed (to avoid hammering the update server).
    bool checkForUpdates();

    static int compareVersions(const QString& a, const QString& b);
    static QString normalizeVersion(const QString& version);

signals:
    void updateAvailable(const QString& latestVersion, const QString& releaseUrl);
    void upToDate(const QString& latestVersion);
    void checkFailed(const QString& friendlyMessage, const QString& details);

private:
    void handleReply(QNetworkReply* reply, const QByteArray& payload);
    static QString friendlyFailureMessage(const QNetworkReply& reply);
    static int numericPrefix(const QString& version);

    // Minimum time between update checks (GitHub unauthenticated rate limit
    // is 60 requests/hour; one hour is a safe, user-friendly interval).
    static constexpr int MinCheckIntervalSecs = 3600;

    void loadLastCheckTime();
    void saveLastCheckTime();

    QNetworkAccessManager m_networkManager;
    QSettings m_settings;
    QDateTime m_lastCheckTime;
    bool m_checkInProgress = false;
};

#endif // UPDATECHECKER_H
