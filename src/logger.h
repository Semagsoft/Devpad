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
#ifndef LOGGER_H
#define LOGGER_H

#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QString>
#include <QDateTime>

class Logger {
public:
    static Logger& instance();

    enum Level { Debug, Info, Warning, Error, Critical };

    void debug(const QString &message) const;
    void info(const QString &message) const;
    void warning(const QString &message) const;
    void error(const QString &message) const;
    void critical(const QString &message) const;

    void log(Level level, const QString &message) const;
    void setLogFile(const QString &filePath);
    void setLevel(Level level);

private:
    Logger();
    ~Logger();
    Q_DISABLE_COPY(Logger)

    void rotateLogFile(const QString &logFilePath);
    QString levelString(Level level) const;

    mutable QFile m_file;
    mutable QTextStream m_stream;
    mutable QMutex m_mutex;
    Level m_level;
};

inline void Logger::debug(const QString &message) const {
    log(Debug, message);
}

inline void Logger::info(const QString &message) const {
    log(Info, message);
}

inline void Logger::warning(const QString &message) const {
    log(Warning, message);
}

inline void Logger::error(const QString &message) const {
    log(Error, message);
}

inline void Logger::critical(const QString &message) const {
    log(Critical, message);
}

#endif // LOGGER_H