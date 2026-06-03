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
#include <QMutexLocker>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QUrl>
#include <QDateTime>
#include <QStandardPaths>
#include "logger.h"
#include "appstrings.h"

QMutex BackupManager::s_mutex;

QString BackupManager::backupDirectory() {
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/devpad/backups";
    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dirPath;
}

QString BackupManager::backupPathForFile(const QString &filePath) {
    QString encoded = QUrl::toPercentEncoding(filePath);
    return QDir(backupDirectory()).filePath(encoded);
}

bool BackupManager::saveBackup(const QString &filePath, const QString &content) {
    if (filePath.isEmpty() || filePath == Strings::untitled()) {
        return false;
    }

    QMutexLocker locker(&s_mutex);
    QString backupPath = backupPathForFile(filePath);
    QFile file(backupPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Logger::instance().warning(QString("Failed to create backup: %1").arg(backupPath));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    out.flush();

    if (file.error() != QFileDevice::NoError) {
        Logger::instance().warning(QString("Failed to write backup: %1").arg(backupPath));
        return false;
    }

    return true;
}

bool BackupManager::hasBackup(const QString &filePath) {
    if (filePath.isEmpty() || filePath == Strings::untitled()) {
        return false;
    }
    QMutexLocker locker(&s_mutex);
    return QFile::exists(backupPathForFile(filePath));
}

bool BackupManager::isBackupNewer(const QString &filePath) {
    if (filePath.isEmpty() || filePath == Strings::untitled()) {
        return false;
    }

    QMutexLocker locker(&s_mutex);
    QString backupPath = backupPathForFile(filePath);
    if (!QFile::exists(backupPath)) {
        return false;
    }

    QFileInfo backupInfo(backupPath);
    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        return true;
    }

    return backupInfo.lastModified() > fileInfo.lastModified();
}

QString BackupManager::getBackupContent(const QString &filePath) {
    if (filePath.isEmpty() || filePath == Strings::untitled()) {
        return QString();
    }

    QMutexLocker locker(&s_mutex);
    QString backupPath = backupPathForFile(filePath);
    QFile file(backupPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Logger::instance().warning(QString("Failed to read backup: %1").arg(backupPath));
        return QString();
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    return in.readAll();
}

bool BackupManager::deleteBackup(const QString &filePath) {
    if (filePath.isEmpty() || filePath == Strings::untitled()) {
        return false;
    }

    QMutexLocker locker(&s_mutex);
    QString backupPath = backupPathForFile(filePath);
    if (QFile::exists(backupPath)) {
        return QFile::remove(backupPath);
    }
    return false;
}
