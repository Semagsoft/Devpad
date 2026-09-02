/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "fileservice.h"

#include "encodingutils.h"
#include "logger.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringConverter>
#include <QTextStream>

FileLoadResult FileService::load(const QString& filePath, const QString& requestedEncoding)
{
    FileLoadResult result;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        result.error = QStringLiteral("Cannot open file: %1").arg(filePath);
        Logger::instance().error(result.error);
        return result;
    }

    qint64 fileSize = file.size();
    result.size = fileSize;
    if (fileSize > MaxFileSize)
    {
        result.error =
            QStringLiteral("File too large (%1 MB). Maximum size is %2 MB.").arg(fileSize / (1024LL * 1024)).arg(MaxFileSize / (1024LL * 1024));
        Logger::instance().error(result.error);
        return result;
    }
    if (fileSize > WarningFileSize)
    {
        Logger::instance().warning(QStringLiteral("File is large (%1 MB). Opening may take a moment.").arg(fileSize / (1024LL * 1024)));
    }

    QByteArray rawData = file.readAll();
    file.close();

    BomResult bom = detectBom(rawData);

    QStringConverter::Encoding enc;
    QString encDisplayName;
    if (requestedEncoding.isEmpty())
    {
        QString detectedEnc = detectEncoding(rawData);
        auto encOpt = QStringConverter::encodingForName(detectedEnc.toUtf8());
        enc = encOpt.value_or(QStringConverter::Utf8);
        encDisplayName = detectedEnc;
        // Normalize display name via encoding utils when possible
        if (encDisplayName.isEmpty())
            encDisplayName = encodingToDisplayName(enc);
    }
    else
    {
        auto encOpt = QStringConverter::encodingForName(requestedEncoding.toUtf8());
        enc = encOpt.value_or(QStringConverter::Utf8);
        encDisplayName = requestedEncoding;
    }

    QStringDecoder decoder(enc);
    QString content;
    if (bom.size > 0)
        content = decoder(rawData.mid(bom.size));
    else
        content = decoder(rawData);

    result.ok = true;
    result.text = content;
    result.encoding = encDisplayName.isEmpty() ? encodingToDisplayName(enc) : encDisplayName;
    Logger::instance().info(QStringLiteral("Loaded file: %1 (%2)").arg(filePath, result.encoding));
    return result;
}

bool FileService::save(const QString& filePath, const QString& text, const QString& encoding, QString* error)
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
    {
        QString msg = QStringLiteral("Cannot save file: %1").arg(filePath);
        Logger::instance().error(msg);
        if (error)
            *error = msg;
        return false;
    }

    QString useEncoding = encoding.isEmpty() ? QStringLiteral("UTF-8") : encoding;
    auto encOpt = QStringConverter::encodingForName(useEncoding.toUtf8());

    QByteArray bom = bomForEncoding(useEncoding);
    if (!bom.isEmpty())
        file.write(bom);

    QTextStream out(&file);
    if (encOpt.has_value())
        out.setEncoding(encOpt.value());
    else
        out.setEncoding(QStringConverter::Utf8);

    out << text;
    out.flush();

    if (out.status() != QTextStream::Ok)
    {
        QString msg = QStringLiteral("Error writing to file: %1").arg(filePath);
        Logger::instance().error(msg);
        if (error)
            *error = msg;
        return false;
    }

    if (!file.commit())
    {
        QString msg = QStringLiteral("Error saving file: %1 - %2").arg(filePath, file.errorString());
        Logger::instance().error(msg);
        if (error)
            *error = msg;
        return false;
    }

    Logger::instance().info(QStringLiteral("Saved file: %1").arg(filePath));
    return true;
}

QString FileService::detectEncoding(const QByteArray& buffer)
{
    if (buffer.isEmpty())
        return QStringLiteral("UTF-8");

    BomResult bom = detectBom(buffer);
    if (bom.size > 0)
        return bom.encodingName;

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
            hasHighBytes = true;
    }

    if (nullCount > 0)
    {
        double nullRatio = static_cast<double>(nullCount) / buffer.size();
        if (nullRatio > 0.5)
        {
            if (nullAt0or1 > nullAt2or3)
                return QStringLiteral("UTF-32BE");
            return QStringLiteral("UTF-32LE");
        }
        if (nullAtEven > nullAtOdd)
            return QStringLiteral("UTF-16BE");
        return QStringLiteral("UTF-16LE");
    }

    bool validUtf8 = true;
    for (int i = 0; i < buffer.size() && validUtf8;)
    {
        unsigned char c = static_cast<unsigned char>(buffer[i]);
        int expectedBytes = 0;
        if ((c & 0x80) == 0)
            expectedBytes = 1;
        else if ((c & 0xE0) == 0xC0)
            expectedBytes = 2;
        else if ((c & 0xF0) == 0xE0)
            expectedBytes = 3;
        else if ((c & 0xF8) == 0xF0)
            expectedBytes = 4;
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
        return QStringLiteral("UTF-8");

    if (hasHighBytes)
        return QStringLiteral("ISO-8859-1");

    return QStringLiteral("UTF-8");
}

QByteArray FileService::bomForEncoding(const QString& encodingName)
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
