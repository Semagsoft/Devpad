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

#include <QMap>
#include <QRegularExpression>

namespace
{
// Records a text replacement performed after tab-stop positions were
// captured: everything at position > shift.pos moves by shift.delta.
struct ShiftEntry
{
    int pos;
    int delta;
};

// Replace every occurrence of `from` with `to`, scanning right-to-left so the
// recorded positions refer to the ORIGINAL text coordinates.
void replaceTracked(QString& text, const QString& from, const QString& to, QList<ShiftEntry>& shifts)
{
    const int delta = to.length() - from.length();
    int searchStart = text.length();
    while (true)
    {
        int idx = text.lastIndexOf(from, searchStart - 1);
        if (idx < 0)
            break;
        text.replace(idx, from.length(), to);
        shifts.append({idx, delta});
        searchStart = idx;
    }
}

// Correct a position recorded before the tracked unescape passes.
int adjustPosition(int pos, const QList<ShiftEntry>& shifts)
{
    int adjusted = pos;
    for (const auto& s : shifts)
    {
        if (pos > s.pos)
            adjusted += s.delta;
    }
    return adjusted;
}

// Apply the final escape decodings to a value (no position tracking). Order
// mirrors the tracked passes in parseBody.
QString decodeEscapes(QString value)
{
    value.replace(QStringLiteral("\\n"), QStringLiteral("\n"));
    value.replace(QStringLiteral("\\t"), QStringLiteral("\t"));
    value.replace(QStringLiteral("\\}"), QStringLiteral("}"));
    static const QString escDollar = QStringLiteral("\x01"
                                                    "ESC_DOLLAR\x01");
    static const QString escBackslash = QStringLiteral("\x01"
                                                       "ESC_BS\x01");
    value.replace(escDollar, QStringLiteral("$"));
    value.replace(escBackslash, QStringLiteral("\\"));
    return value;
}
} // namespace

Snippet::ExpandedSnippet Snippet::parseBody(const QStringList& bodyLines)
{
    QString joined = bodyLines.join('\n');

    // Protect escaped dollar-brace sequences before placeholder parsing:
    //  \$  -> sentinel so it is not treated as tab stop
    //  \\  -> sentinel so \\n is not prematurely converted
    static const QString escDollar = QStringLiteral("\x01"
                                                    "ESC_DOLLAR\x01");
    static const QString escBackslash = QStringLiteral("\x01"
                                                       "ESC_BS\x01");
    joined.replace(QStringLiteral("\\$"), escDollar);
    joined.replace(QStringLiteral("\\\\"), escBackslash);

    ExpandedSnippet result;
    QMap<int, TabStop> tabStopMap;
    QList<ShiftEntry> shifts;

    // Replace ${n:default} placeholders. The default may contain escaped
    // braces (\}) and other escapes — the (?:\\.|[^}])* pattern consumes
    // \-escapes atomically so a literal } inside the default cannot end the
    // placeholder early.
    static const QRegularExpression placeholderRe(R"(\$\{(\d+):((?:\\.|[^}])*)\})");
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
    std::sort(replacements.begin(), replacements.end(), [](const PlaceholderReplace& a, const PlaceholderReplace& b) { return a.pos > b.pos; });

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

    // Restore escaped sequences and handle remaining escapes (\n, \t, \}, etc.).
    // These replacements change string lengths, so they are tracked and the
    // recorded tab-stop positions are corrected afterwards — otherwise any
    // tab stop after an escape would point at the wrong offset.
    replaceTracked(joined, QStringLiteral("\\n"), QStringLiteral("\n"), shifts);
    replaceTracked(joined, QStringLiteral("\\t"), QStringLiteral("\t"), shifts);
    replaceTracked(joined, QStringLiteral("\\}"), QStringLiteral("}"), shifts);
    // Sentinel restoration LAST: restoring "\\" before the \n pass would turn
    // a protected "\\n" into a real newline.
    replaceTracked(joined, escDollar, QStringLiteral("$"), shifts);
    replaceTracked(joined, escBackslash, QStringLiteral("\\"), shifts);

    result.text = joined;

    for (auto it2 = tabStopMap.begin(); it2 != tabStopMap.end(); ++it2)
    {
        TabStop ts = it2.value();
        // The default value text also goes through the final unescape passes,
        // so its real length differs from the raw captured one.
        ts.defaultValue = decodeEscapes(ts.defaultValue);
        ts.length = ts.defaultValue.length();
        for (int& pos : ts.positions)
        {
            pos = adjustPosition(pos, shifts);
        }
        std::sort(ts.positions.begin(), ts.positions.end());
        result.tabStops.append(ts);
    }

    std::sort(result.tabStops.begin(), result.tabStops.end(), [](const TabStop& a, const TabStop& b) { return a.number < b.number; });

    return result;
}

Snippet::ExpandedSnippet Snippet::expand() const
{
    return parseBody(body);
}
