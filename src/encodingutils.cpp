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
#include "encodingutils.h"

namespace
{
QVector<EncodingInfo>& pluginEncodingTable();
} // namespace

BomResult detectBom(const QByteArray& data)
{
    if (data.size() >= 3 && data.first(3) == QByteArray("\xEF\xBB\xBF", 3))
    {
        return {3, "UTF-8-BOM"};
    }
    if (data.size() >= 4 && data.first(4) == QByteArray("\x00\x00\xFE\xFF", 4))
    {
        return {4, "UTF-32BE"};
    }
    if (data.size() >= 4 && data.first(4) == QByteArray("\xFF\xFE\x00\x00", 4))
    {
        return {4, "UTF-32LE"};
    }
    if (data.size() >= 2 && data.first(2) == QByteArray("\xFE\xFF", 2))
    {
        return {2, "UTF-16BE"};
    }
    if (data.size() >= 2 && data.first(2) == QByteArray("\xFF\xFE", 2))
    {
        return {2, "UTF-16LE"};
    }
    return {0, QString()};
}

const QVector<EncodingInfo>& supportedEncodings()
{
    static const QVector<EncodingInfo> encodings = {
        {"UTF-8", QStringConverter::Utf8},       {"UTF-8-BOM", QStringConverter::Utf8},   {"UTF-16", QStringConverter::Utf16},
        {"UTF-16LE", QStringConverter::Utf16LE}, {"UTF-16BE", QStringConverter::Utf16BE}, {"UTF-32", QStringConverter::Utf32},
        {"UTF-32LE", QStringConverter::Utf32LE}, {"UTF-32BE", QStringConverter::Utf32BE}, {"ISO-8859-1", QStringConverter::Latin1},
        {"System", QStringConverter::System},
    };
    return encodings;
}

QStringConverter::Encoding encodingFromName(const QString& name)
{
    for (const auto& info : supportedEncodings())
    {
        if (info.displayName == name)
        {
            return info.encoding;
        }
    }
    // Check plugin-registered encodings.
    auto& pluginEncodings = pluginEncodingTable();
    for (const auto& info : pluginEncodings)
    {
        if (info.displayName == name)
        {
            return info.encoding;
        }
    }
    return QStringConverter::Utf8;
}

QString encodingToDisplayName(QStringConverter::Encoding enc)
{
    for (const auto& info : supportedEncodings())
    {
        if (info.encoding == enc)
        {
            return info.displayName;
        }
    }
    return "UTF-8";
}

// ── Plugin encoding registry ──────────────────────────────────
namespace
{
QVector<EncodingInfo>& pluginEncodingTable()
{
    static QVector<EncodingInfo> encodings;
    return encodings;
}
} // namespace

bool registerPluginEncoding(const QString& displayName, const QString& encodingName)
{
    if (displayName.isEmpty() || encodingName.isEmpty())
        return false;
    auto& table = pluginEncodingTable();
    for (const auto& info : table)
    {
        if (info.displayName == displayName)
            return false;
    }
    QStringConverter::Encoding enc = QStringConverter::Utf8;
    // Try to resolve the encoding name via QStringConverter.
    // QStringConverter doesn't have a name lookup, so we just store the name
    // and use the encoding value. For now, map common names; unknown names
    // default to UTF-8.
    if (encodingName == QLatin1String("UTF-8"))
        enc = QStringConverter::Utf8;
    else if (encodingName == QLatin1String("UTF-16LE"))
        enc = QStringConverter::Utf16LE;
    else if (encodingName == QLatin1String("UTF-16BE"))
        enc = QStringConverter::Utf16BE;
    else if (encodingName == QLatin1String("UTF-32LE"))
        enc = QStringConverter::Utf32LE;
    else if (encodingName == QLatin1String("UTF-32BE"))
        enc = QStringConverter::Utf32BE;
    else if (encodingName == QLatin1String("ISO-8859-1") || encodingName == QLatin1String("Latin1"))
        enc = QStringConverter::Latin1;
    table.append({displayName, enc});
    return true;
}

bool unregisterPluginEncoding(const QString& displayName)
{
    auto& table = pluginEncodingTable();
    for (int i = 0; i < table.size(); ++i)
    {
        if (table[i].displayName == displayName)
        {
            table.removeAt(i);
            return true;
        }
    }
    return false;
}
