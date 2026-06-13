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

const QVector<EncodingInfo>& supportedEncodings() {
    static const QVector<EncodingInfo> encodings = {
        {"UTF-8", QStringConverter::Utf8},
        {"UTF-16", QStringConverter::Utf16},
        {"UTF-16LE", QStringConverter::Utf16LE},
        {"UTF-16BE", QStringConverter::Utf16BE},
        {"UTF-32", QStringConverter::Utf32},
        {"UTF-32LE", QStringConverter::Utf32LE},
        {"UTF-32BE", QStringConverter::Utf32BE},
        {"ISO-8859-1", QStringConverter::Latin1},
        {"System", QStringConverter::System},
    };
    return encodings;
}

QStringConverter::Encoding encodingFromName(const QString &name) {
    for (const auto &info : supportedEncodings()) {
        if (info.displayName == name) {
            return info.encoding;
        }
    }
    return QStringConverter::Utf8;
}

QString encodingToDisplayName(QStringConverter::Encoding enc) {
    for (const auto &info : supportedEncodings()) {
        if (info.encoding == enc) {
            return info.displayName;
        }
    }
    return "UTF-8";
}
