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
#include "logger.h"
#include <QStandardPaths>

namespace {
constexpr qint64 MaxLogSize = 1 * 1024 * 1024; // 1 MB
constexpr int MaxRotatedLogs = 3;
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::rotateLogFile(const QString &logFilePath) {
    for (int i = MaxRotatedLogs - 1; i >= 1; --i) {
        QString oldPath = QString("%1.%2").arg(logFilePath).arg(i);
        QString newPath = QString("%1.%2").arg(logFilePath).arg(i + 1);
        if (QFile::exists(oldPath)) {
            QFile::remove(newPath);
            if (!QFile::rename(oldPath, newPath)) {
                qWarning("Failed to rotate log: %s -> %s", qPrintable(oldPath), qPrintable(newPath));
            }
        }
    }
    QString rotatedPath = logFilePath + ".1";
    QFile::remove(rotatedPath);
    if (!QFile::rename(logFilePath, rotatedPath)) {
        qWarning("Failed to rotate log: %s -> %s", qPrintable(logFilePath), qPrintable(rotatedPath));
    }
}

Logger::Logger()
    : m_level(Debug)
{
    QMutexLocker locker(&m_mutex);
    QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/devpad";
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
    QString logFile = logDir.filePath("devpad.log");

    QFileInfo fi(logFile);
    if (fi.exists() && fi.size() >= MaxLogSize) {
        m_file.setFileName(logFile);
        if (m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_file.close();
        }
        rotateLogFile(logFile);
    }

    m_file.setFileName(logFile);
    if (m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_file);
    }
}

Logger::~Logger() {
    m_stream.flush();
    m_file.close();
}

void Logger::log(Level level, const QString &message) const {
    QMutexLocker locker(&m_mutex);
    if (level < m_level) return;

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString levelStr = levelString(level);
    QString formatted = QString("[%1] [%2] %3").arg(timestamp, levelStr, message);

    if (m_stream.device()) {
        m_stream << formatted << "\n";
        m_stream.flush();
    }

#ifndef QT_NO_DEBUG
    fprintf(stderr, "%s\n", qPrintable(formatted));
#endif
}

QString Logger::levelString(Level level) const {
    switch (level) {
        case Debug:    return "DEBUG";
        case Info:     return "INFO";
        case Warning:  return "WARN";
        case Error:    return "ERROR";
        case Critical: return "CRITICAL";
        default:       return "UNKNOWN";
    }
}

void Logger::setLogFile(const QString &filePath) {
    QMutexLocker locker(&m_mutex);
    m_stream.flush();
    m_file.close();

    m_file.setFileName(filePath);
    if (m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_file);
    }
}

void Logger::setLevel(Level level) {
    QMutexLocker locker(&m_mutex);
    m_level = level;
}