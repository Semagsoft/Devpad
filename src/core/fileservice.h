/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef FILESERVICE_H
#define FILESERVICE_H

#include <QByteArray>
#include <QString>

struct FileLoadResult
{
    bool ok = false;
    QString text;
    QString encoding; // display name e.g. "UTF-8"
    QString error;
    qint64 size = 0;
};

class FileService
{
public:
    static constexpr qint64 MaxFileSize = 100LL * 1024 * 1024;
    static constexpr qint64 WarningFileSize = 50LL * 1024 * 1024;

    static FileLoadResult load(const QString& filePath, const QString& requestedEncoding = QString());
    static bool save(const QString& filePath, const QString& text, const QString& encoding, QString* error = nullptr);
    static QString detectEncoding(const QByteArray& buffer);

private:
    static QByteArray bomForEncoding(const QString& encodingName);
};

#endif // FILESERVICE_H
