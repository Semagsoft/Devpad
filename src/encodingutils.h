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
#ifndef ENCODINGUTILS_H
#define ENCODINGUTILS_H

#include <QByteArray>
#include <QString>
#include <QStringConverter>
#include <QVector>

struct EncodingInfo {
    QString displayName;
    QStringConverter::Encoding encoding;
};

struct BomResult {
    int size = 0;
    QString encodingName;
};

BomResult detectBom(const QByteArray &data);

const QVector<EncodingInfo>& supportedEncodings();
QStringConverter::Encoding encodingFromName(const QString &name);
QString encodingToDisplayName(QStringConverter::Encoding enc);

#endif
