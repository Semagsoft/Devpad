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
#ifndef SNIPPET_H
#define SNIPPET_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QPair>
#include <QMap>

struct Snippet
{
    QString name;
    QString prefix;
    QString description;
    QStringList body; // each element is a line
    QStringList scope;

    struct TabStop
    {
        int number = 0;
        QList<int> positions; // positions in expanded text
        QString defaultValue;
        int length = 0;
    };

    struct ExpandedSnippet
    {
        QString text;
        QList<TabStop> tabStops;
    };

    ExpandedSnippet expand() const;

    // Parse a VS Code-format snippet body string into our representation
    static ExpandedSnippet parseBody(const QStringList& bodyLines);
};

#endif // SNIPPET_H
