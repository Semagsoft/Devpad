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
#include "filemanager.h"

#include "codeeditor.h"
#include "encodingutils.h"
#include "logger.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringConverter>
#include <QTextStream>

namespace
{

constexpr qint64 MaxFileSize = static_cast<qint64>(100) * 1024 * 1024;
constexpr qint64 WarningFileSize = static_cast<qint64>(50) * 1024 * 1024;
} // namespace

FileManager::FileManager(QObject* parent) : QObject(parent)
{
}

bool FileManager::loadFile(const QString& fileName, CodeEditor* editor, const QString& encoding)
{
    if (!editor)
    {
        m_lastError = tr("Editor pointer is null");
        Logger::instance().error(m_lastError);
        emit error(m_lastError);
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        m_lastError = tr("Cannot open file: %1").arg(fileName);
        Logger::instance().error(m_lastError);
        emit error(m_lastError);
        return false;
    }

    qint64 fileSize = file.size();
    if (fileSize > MaxFileSize)
    {
        m_lastError = tr("File too large (%1 MB). Maximum size is %2 MB.")
                          .arg(fileSize / static_cast<qint64>(1024 * 1024))
                          .arg(MaxFileSize / static_cast<qint64>(1024 * 1024));
        Logger::instance().error(m_lastError);
        emit error(m_lastError);
        file.close();
        return false;
    }

    if (fileSize > WarningFileSize)
    {
        QString warnMsg = tr("File is large (%1 MB). Opening may take a moment.").arg(fileSize / static_cast<qint64>(1024 * 1024));
        Logger::instance().warning(warnMsg);
    }

    QByteArray rawData = file.readAll();
    file.close();

    BomResult bom = detectBom(rawData);

    QStringConverter::Encoding enc;
    if (encoding.isEmpty())
    {
        QString detectedEnc = detectEncoding(rawData);
        auto encOpt = QStringConverter::encodingForName(detectedEnc.toUtf8());
        enc = encOpt.value_or(QStringConverter::Utf8);
    }
    else
    {
        auto encOpt = QStringConverter::encodingForName(encoding.toUtf8());
        enc = encOpt.value_or(QStringConverter::Utf8);
    }
    QStringDecoder decoder(enc);
    QString content;
    if (bom.size > 0)
    {
        content = decoder(rawData.mid(bom.size));
    }
    else
    {
        content = decoder(rawData);
    }

    editor->setText(content);

    editor->setFileName(fileName);
    editor->setModified(false);
    editor->setEncoding(encoding.isEmpty() ? encodingToDisplayName(enc) : encoding);

    Logger::instance().info(QString("Opened file: %1").arg(fileName));
    emit fileLoaded(fileName);
    return true;
}

static QByteArray bomForEncoding(const QString& encodingName)
{
    if (encodingName == "UTF-8")
        return QByteArray("\xEF\xBB\xBF", 3);
    if (encodingName == "UTF-16BE")
        return QByteArray("\xFE\xFF", 2);
    if (encodingName == "UTF-16LE")
        return QByteArray("\xFF\xFE", 2);
    if (encodingName == "UTF-32BE")
        return QByteArray("\x00\x00\xFE\xFF", 4);
    if (encodingName == "UTF-32LE")
        return QByteArray("\xFF\xFE\x00\x00", 4);
    if (encodingName == "UTF-16")
        return QByteArray("\xFF\xFE", 2);
    if (encodingName == "UTF-32")
        return QByteArray("\xFF\xFE\x00\x00", 4);
    return QByteArray();
}

bool FileManager::saveFile(const QString& fileName, CodeEditor* editor, const QString& encoding)
{
    if (!editor)
    {
        m_lastError = tr("Editor pointer is null");
        Logger::instance().error(m_lastError);
        emit error(m_lastError);
        return false;
    }

    QSaveFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        m_lastError = tr("Cannot save file: %1").arg(fileName);
        Logger::instance().error(m_lastError);
        emit error(m_lastError);
        return false;
    }

    QString useEncoding = encoding.isEmpty() ? editor->encoding() : encoding;
    auto encOpt = QStringConverter::encodingForName(useEncoding.toUtf8());

    QByteArray bom = bomForEncoding(useEncoding);
    if (!bom.isEmpty())
    {
        file.write(bom);
    }

    QTextStream out(&file);
    if (encOpt.has_value())
    {
        out.setEncoding(encOpt.value());
    }
    else
    {
        out.setEncoding(QStringConverter::Utf8);
    }

    out << editor->text();
    out.flush();

    if (out.status() != QTextStream::Ok)
    {
        m_lastError = tr("Error writing to file: %1").arg(fileName);
        Logger::instance().error(m_lastError);
        emit error(m_lastError);
        return false;
    }

    if (!file.commit())
    {
        m_lastError = tr("Error saving file: %1 - %2").arg(fileName, file.errorString());
        Logger::instance().error(m_lastError);
        emit error(m_lastError);
        return false;
    }

    editor->setModified(false);

    Logger::instance().info(QString("Saved file: %1").arg(fileName));
    emit fileSaved(fileName);
    return true;
}

QString FileManager::detectEncoding(const QByteArray& buffer)
{
    if (buffer.isEmpty())
    {
        return "UTF-8";
    }

    BomResult bom = detectBom(buffer);
    if (bom.size > 0)
    {
        return bom.encodingName;
    }

    int nullCount = 0;
    int nullAtEven = 0;
    int nullAtOdd = 0;
    int nullAt0or1 = 0;
    int nullAt2or3 = 0;
    bool hasHighBytes = false;

    for (int i = 0; i < buffer.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(buffer[i]);

        if (c == 0)
        {
            nullCount++;
            if (i % 2 == 0)
                nullAtEven++;
            else
                nullAtOdd++;
            if (i % 4 < 2)
                nullAt0or1++;
            else
                nullAt2or3++;
        }

        if (c & 0x80)
        {
            hasHighBytes = true;
        }
    }

    if (nullCount > 0)
    {
        double nullRatio = static_cast<double>(nullCount) / buffer.size();
        if (nullRatio > 0.5)
        {
            if (nullAt0or1 > nullAt2or3)
            {
                return "UTF-32BE";
            }
            else
            {
                return "UTF-32LE";
            }
        }
        else
        {
            if (nullAtEven > nullAtOdd)
            {
                return "UTF-16BE";
            }
            else
            {
                return "UTF-16LE";
            }
        }
    }

    // Validate UTF-8 encoding
    bool validUtf8 = true;
    for (int i = 0; i < buffer.size() && validUtf8;)
    {
        unsigned char c = static_cast<unsigned char>(buffer[i]);
        int expectedBytes = 0;
        if ((c & 0x80) == 0)
        {
            expectedBytes = 1;
        }
        else if ((c & 0xE0) == 0xC0)
        {
            expectedBytes = 2;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            expectedBytes = 3;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            expectedBytes = 4;
        }
        else
        {
            validUtf8 = false;
            break;
        }
        for (int j = 1; j < expectedBytes; ++j)
        {
            if (i + j >= buffer.size() || (static_cast<unsigned char>(buffer[i + j]) & 0xC0) != 0x80)
            {
                validUtf8 = false;
                break;
            }
        }
        i += expectedBytes;
    }

    if (validUtf8)
    {
        return "UTF-8";
    }

    if (hasHighBytes)
    {
        return "ISO-8859-1";
    }

    return "UTF-8";
}