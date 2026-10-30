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
#include "backupmanager.h"

#include "appstrings.h"
#include "logger.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>

QMutex BackupManager::s_mutex;

QString BackupManager::backupDirectory()
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/devpad/backups";
    QDir dir(dirPath);
    if (!dir.exists())
    {
        dir.mkpath(".");
    }
    return dirPath;
}

QString BackupManager::backupPathForFile(const QString& filePath)
{
    QString encoded = QUrl::toPercentEncoding(filePath);
    return QDir(backupDirectory()).filePath(encoded);
}

bool BackupManager::saveBackup(const QString& filePath, const QString& content)
{
    if (filePath.isEmpty() || filePath == Strings::untitled())
    {
        return false;
    }

    QMutexLocker locker(&s_mutex);
    QString backupPath = backupPathForFile(filePath);
    // Write atomically: a crash or power loss mid-write must not leave a
    // truncated backup behind (it would later be restored into the editor).
    QSaveFile file(backupPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        Logger::instance().warning(QString("Failed to create backup: %1").arg(backupPath));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    out.flush();

    if (out.status() != QTextStream::Ok || file.error() != QFileDevice::NoError)
    {
        file.cancelWriting();
        Logger::instance().warning(QString("Failed to write backup: %1").arg(backupPath));
        return false;
    }

    if (!file.commit())
    {
        Logger::instance().warning(QString("Failed to commit backup: %1").arg(backupPath));
        return false;
    }

    return true;
}

bool BackupManager::hasBackup(const QString& filePath)
{
    if (filePath.isEmpty() || filePath == Strings::untitled())
    {
        return false;
    }
    QMutexLocker locker(&s_mutex);
    return QFile::exists(backupPathForFile(filePath));
}

bool BackupManager::isBackupNewer(const QString& filePath)
{
    if (filePath.isEmpty() || filePath == Strings::untitled())
    {
        return false;
    }

    QMutexLocker locker(&s_mutex);
    QString backupPath = backupPathForFile(filePath);
    if (!QFile::exists(backupPath))
    {
        return false;
    }

    QFileInfo backupInfo(backupPath);
    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists())
    {
        return true;
    }

    if (backupInfo.lastModified() != fileInfo.lastModified())
    {
        return backupInfo.lastModified() > fileInfo.lastModified();
    }

    // Same mtime tick (coarse filesystems, network shares): fall back to a
    // size comparison so the backup is offered for restore rather than
    // silently discarded on a coin flip.
    return backupInfo.size() != fileInfo.size();
}

QString BackupManager::getBackupContent(const QString& filePath)
{
    if (filePath.isEmpty() || filePath == Strings::untitled())
    {
        return QString();
    }

    QMutexLocker locker(&s_mutex);
    QString backupPath = backupPathForFile(filePath);
    QFile file(backupPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        Logger::instance().warning(QString("Failed to read backup: %1").arg(backupPath));
        return QString();
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    return in.readAll();
}

bool BackupManager::deleteBackup(const QString& filePath)
{
    if (filePath.isEmpty() || filePath == Strings::untitled())
    {
        return false;
    }

    QMutexLocker locker(&s_mutex);
    QString backupPath = backupPathForFile(filePath);
    if (QFile::exists(backupPath))
    {
        return QFile::remove(backupPath);
    }
    return false;
}

void BackupManager::purgeStaleBackups(int maxAgeDays)
{
    QMutexLocker locker(&s_mutex);

    QDir dir(backupDirectory());
    const QFileInfoList entries = dir.entryInfoList(QDir::Files, QDir::Time);

    const QDateTime cutoff = QDateTime::currentDateTimeUtc().addDays(-maxAgeDays);
    for (const QFileInfo& entry : entries)
    {
        // Backup names are percent-encoded original paths.
        const QString originalPath = QUrl::fromPercentEncoding(entry.fileName().toUtf8());
        const bool originalGone = !QFileInfo::exists(originalPath);
        const bool tooOld = entry.lastModified() < cutoff;

        if (originalGone || tooOld)
        {
            if (!QFile::remove(entry.absoluteFilePath()))
            {
                Logger::instance().warning(QString("Failed to remove stale backup: %1").arg(entry.absoluteFilePath()));
            }
        }
    }
}
