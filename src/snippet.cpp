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
#include "snippet.h"

#include <QRegularExpression>
#include <QMap>

Snippet::ExpandedSnippet Snippet::parseBody(const QStringList& bodyLines)
{
    QString joined = bodyLines.join('\n');

    // Handle escape sequences first
    joined.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    joined.replace(QStringLiteral("\\t"), QStringLiteral("\t"));
    joined.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));
    joined.replace(QStringLiteral("\\$"), QStringLiteral("$"));
    joined.replace(QStringLiteral("\\}"), QStringLiteral("}"));

    ExpandedSnippet result;
    QMap<int, TabStop> tabStopMap;
    QList<QPair<int, int>> mirrorRanges; // position, number (for $n without braces)

    // Replace ${n:default} placeholders
    static const QRegularExpression placeholderRe(R"(\$\{(\d+):([^}]*)\})");
    QRegularExpressionMatchIterator it = placeholderRe.globalMatch(joined);
    struct PlaceholderReplace
    {
        int pos;
        int len;
        QString replacement;
        int number;
        QString defaultValue;
    };
    QList<PlaceholderReplace> replacements;

    while (it.hasNext())
    {
        QRegularExpressionMatch m = it.next();
        int number = m.captured(1).toInt();
        QString defaultValue = m.captured(2);

        PlaceholderReplace pr;
        pr.pos = m.capturedStart();
        pr.len = m.capturedLength();
        pr.number = number;
        pr.defaultValue = defaultValue;
        pr.replacement = defaultValue;
        replacements.append(pr);
    }

    // Apply replacements from end to start to preserve positions
    std::sort(replacements.begin(), replacements.end(),
              [](const PlaceholderReplace& a, const PlaceholderReplace& b) { return a.pos > b.pos; });

    for (const auto& pr : replacements)
    {
        joined.replace(pr.pos, pr.len, pr.replacement);
        auto mapIt = tabStopMap.find(pr.number);
        if (mapIt != tabStopMap.end())
        {
            mapIt->positions.append(pr.pos);
        }
        else
        {
            TabStop ts;
            ts.number = pr.number;
            ts.positions.append(pr.pos);
            ts.defaultValue = pr.defaultValue;
            ts.length = pr.defaultValue.length();
            tabStopMap.insert(pr.number, ts);
        }
    }

    // Replace ${n} (braces without default)
    static const QRegularExpression braceStopRe(R"(\$\{(\d+)\})");
    QRegularExpressionMatchIterator braceIt = braceStopRe.globalMatch(joined);
    QList<PlaceholderReplace> braceReplacements;
    while (braceIt.hasNext())
    {
        QRegularExpressionMatch m = braceIt.next();
        int number = m.captured(1).toInt();

        PlaceholderReplace pr;
        pr.pos = m.capturedStart();
        pr.len = m.capturedLength();
        pr.number = number;
        pr.replacement.clear();
        braceReplacements.append(pr);
    }

    std::sort(braceReplacements.begin(), braceReplacements.end(),
              [](const PlaceholderReplace& a, const PlaceholderReplace& b) { return a.pos > b.pos; });

    for (const auto& pr : braceReplacements)
    {
        joined.replace(pr.pos, pr.len, pr.replacement);
        auto mapIt = tabStopMap.find(pr.number);
        if (mapIt != tabStopMap.end())
        {
            mapIt->positions.append(pr.pos);
        }
        else
        {
            TabStop ts;
            ts.number = pr.number;
            ts.positions.append(pr.pos);
            ts.defaultValue.clear();
            ts.length = 0;
            tabStopMap.insert(pr.number, ts);
        }
    }

    // Replace $n (simple tab stops without defaults)
    static const QRegularExpression simpleStopRe(R"(\$(\d+))");
    QRegularExpressionMatchIterator simpleIt = simpleStopRe.globalMatch(joined);
    QList<PlaceholderReplace> simpleReplacements;
    while (simpleIt.hasNext())
    {
        QRegularExpressionMatch m = simpleIt.next();
        int number = m.captured(1).toInt();

        PlaceholderReplace pr;
        pr.pos = m.capturedStart();
        pr.len = m.capturedLength();
        pr.number = number;
        pr.replacement.clear();
        simpleReplacements.append(pr);
    }

    // Apply from end to start
    std::sort(simpleReplacements.begin(), simpleReplacements.end(),
              [](const PlaceholderReplace& a, const PlaceholderReplace& b) { return a.pos > b.pos; });

    for (const auto& pr : simpleReplacements)
    {
        joined.replace(pr.pos, pr.len, pr.replacement);
        auto mapIt = tabStopMap.find(pr.number);
        if (mapIt != tabStopMap.end())
        {
            mapIt->positions.append(pr.pos);
        }
        else
        {
            TabStop ts;
            ts.number = pr.number;
            ts.positions.append(pr.pos);
            ts.defaultValue.clear();
            ts.length = 0;
            tabStopMap.insert(pr.number, ts);
        }
    }

    result.text = joined;

    for (auto it2 = tabStopMap.begin(); it2 != tabStopMap.end(); ++it2)
    {
        result.tabStops.append(it2.value());
    }

    std::sort(result.tabStops.begin(), result.tabStops.end(),
              [](const TabStop& a, const TabStop& b) { return a.number < b.number; });

    return result;
}

Snippet::ExpandedSnippet Snippet::expand() const
{
    return parseBody(body);
}
