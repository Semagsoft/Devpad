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
#include "lspworkspaceedit.h"

#include "lsptypes.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QVector>

#include <algorithm>

namespace lsp
{

namespace
{
// Byte offsets of the start of every line.
QVector<int> lineStartsOf(const QByteArray& utf8)
{
    QVector<int> starts;
    starts.append(0);
    for (int i = 0; i < utf8.size(); ++i)
        if (utf8.at(i) == '\n')
            starts.append(i + 1);
    return starts;
}
} // namespace

QString applyTextEditsToText(const QString& text, const QList<QJsonObject>& editsIn, bool* ok, QString* error)
{
    if (ok)
        *ok = false;

    const QByteArray full = text.toUtf8();

    struct PreparedEdit
    {
        int from = 0;
        int to = 0;
        QByteArray newText;
    };

    QList<PreparedEdit> prepared;
    prepared.reserve(editsIn.size());
    const QVector<int> lineStarts = lineStartsOf(full);
    const QStringList lines = text.split(QLatin1Char('\n'));

    for (const auto& edit : editsIn)
    {
        QJsonObject rangeObj = edit.value(QStringLiteral("range")).toObject();
        Range range = Range::fromJson(rangeObj);
        const int startLine = range.start.line;
        const int endLine = range.end.line;
        if (startLine < 0 || endLine < 0 || startLine >= lines.size() || endLine >= lines.size())
        {
            if (error)
                *error = QStringLiteral("Edit range is outside the document");
            return text;
        }

        const QString& startLineText = lines[startLine];
        const QString& endLineText = lines[endLine];
        const int startCol = utf16ToByteOffset(startLineText, range.start.character);
        const int endCol = utf16ToByteOffset(endLineText, range.end.character);

        PreparedEdit p;
        p.from = lineStarts[startLine] + startCol;
        p.to = lineStarts[endLine] + endCol;
        p.newText = edit.value(QStringLiteral("newText")).toString().toUtf8();
        prepared.append(p);
    }

    // Apply bottom-most first so earlier ranges stay valid against the
    // original document.
    std::sort(prepared.begin(), prepared.end(),
              [](const PreparedEdit& a, const PreparedEdit& b)
              {
                  if (a.from != b.from)
                      return a.from > b.from;
                  return a.to > b.to;
              });

    QByteArray result = full;
    for (const auto& p : prepared)
    {
        if (p.from > p.to || p.from < 0 || p.to > result.size())
        {
            if (error)
                *error = QStringLiteral("Edit range is invalid");
            return text;
        }
        result.replace(p.from, p.to - p.from, p.newText);
    }

    if (ok)
        *ok = true;
    return QString::fromUtf8(result);
}

bool applyTextEditsToFile(const QString& filePath, const QList<QJsonObject>& edits, QString* error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (error)
            *error = QStringLiteral("Cannot open file: %1").arg(filePath);
        return false;
    }
    const QString text = QString::fromUtf8(file.readAll());
    file.close();

    bool ok = false;
    const QString updated = applyTextEditsToText(text, edits, &ok, error);
    if (!ok)
        return false;

    QSaveFile out(filePath);
    if (!out.open(QIODevice::WriteOnly))
    {
        if (error)
            *error = QStringLiteral("Cannot write file: %1").arg(filePath);
        return false;
    }
    out.write(updated.toUtf8());
    if (!out.commit())
    {
        if (error)
            *error = QStringLiteral("Cannot write file: %1").arg(out.errorString());
        return false;
    }
    return true;
}

} // namespace lsp
