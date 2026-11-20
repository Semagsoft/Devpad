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

#include <QMutex>
#include <QMutexLocker>

namespace
{
QVector<EncodingInfo>& pluginEncodingTable();
QMutex& encodingRegistryMutex();
int& pluginEncodingGeneration();
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
        // Ambiguous: this is both the UTF-32LE BOM and a UTF-16LE BOM followed
        // by U+0000. A real UTF-32LE document's next code point has zero high
        // bytes; if bytes 5-7 are non-zero, prefer UTF-16LE so we do not strip
        // the first character of a UTF-16LE file that starts with a NUL.
        if (data.size() >= 8 && (data.at(5) != 0 || data.at(6) != 0 || data.at(7) != 0))
            return {2, "UTF-16LE"};
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
    // Built-ins first, then plugin-registered encodings (which is also the
    // precedence used by encodingToDisplayName and the encoding menus).
    // Build only when the plugin table actually changes, under a lock, instead
    // of reassigning a shared static on every call from multiple threads.
    static QVector<EncodingInfo> merged;
    static int builtGeneration = -1;
    QMutexLocker locker(&encodingRegistryMutex());
    if (builtGeneration != pluginEncodingGeneration())
    {
        merged = encodings;
        const auto& plugins = pluginEncodingTable();
        merged.reserve(encodings.size() + plugins.size());
        for (const auto& plugin : plugins)
            merged.append(plugin);
        builtGeneration = pluginEncodingGeneration();
    }
    return merged;
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

QMutex& encodingRegistryMutex()
{
    static QMutex mutex;
    return mutex;
}

int& pluginEncodingGeneration()
{
    static int generation = 0;
    return generation;
}
} // namespace

bool registerPluginEncoding(const QString& displayName, const QString& encodingName)
{
    if (displayName.isEmpty() || encodingName.isEmpty())
        return false;
    QMutexLocker locker(&encodingRegistryMutex());
    auto& table = pluginEncodingTable();
    for (const auto& info : table)
    {
        if (info.displayName == displayName)
            return false;
    }
    // Resolve the encoding through Qt's name lookup, the same route used by
    // the built-in table. Unknown names are rejected rather than silently
    // degrading to UTF-8.
    const auto encOpt = QStringConverter::encodingForName(encodingName.toUtf8());
    if (!encOpt.has_value())
        return false;
    table.append({displayName, encOpt.value()});
    ++pluginEncodingGeneration();
    return true;
}

bool unregisterPluginEncoding(const QString& displayName)
{
    QMutexLocker locker(&encodingRegistryMutex());
    auto& table = pluginEncodingTable();
    for (int i = 0; i < table.size(); ++i)
    {
        if (table[i].displayName == displayName)
        {
            table.removeAt(i);
            ++pluginEncodingGeneration();
            return true;
        }
    }
    return false;
}
