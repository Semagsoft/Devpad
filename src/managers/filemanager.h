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
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QObject>
#include <QString>

class CodeEditor;
class QWidget;

class FileManager : public QObject
{
    Q_OBJECT

public:
    explicit FileManager(QObject* parent = nullptr);
    ~FileManager() override = default;

    bool loadFile(const QString& fileName, CodeEditor* editor, const QString& encoding = QString());
    bool saveFile(const QString& fileName, CodeEditor* editor, const QString& encoding = QString());

    const QString& lastError() const
    {
        return m_lastError;
    }

    static QString detectEncoding(const QByteArray& buffer);

signals:
    void fileLoaded(const QString& fileName);
    void fileSaved(const QString& fileName);
    void error(const QString& message);

private:
    QString m_lastError;
};

#endif // FILEMANAGER_H