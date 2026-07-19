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
#ifndef APPSTRINGS_H
#define APPSTRINGS_H

#include <QObject>
#include <QString>

namespace Strings
{
inline const QString& AppName()
{
    static const QString s = QStringLiteral("Devpad");
    return s;
}
inline QString untitled()
{
    return QObject::tr("Untitled");
}
inline QString fileFilter()
{
    return QObject::tr(
        "All Files (*.*);;C/C++ Files (*.c *.cpp *.h *.hpp);;Python Files (*.py *.pyw);;JavaScript Files (*.js *.mjs *.cjs);;HTML Files (*.html "
        "*.htm);;CSS Files (*.css);;XML Files (*.xml);;Java Files (*.java);;C# Files (*.cs);;SQL Files (*.sql);;Text Files (*.txt)");
}
} // namespace Strings

#endif
