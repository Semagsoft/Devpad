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
#ifndef LSPTYPES_H
#define LSPTYPES_H

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

namespace lsp
{

struct Position
{
    Q_GADGET
    Q_PROPERTY(int line MEMBER line)
    Q_PROPERTY(int character MEMBER character)

public:
    int line = 0;
    int character = 0;

    bool operator==(const Position& other) const
    {
        return line == other.line && character == other.character;
    }

    QJsonObject toJson() const;
    static Position fromJson(const QJsonObject& obj);
};

struct Range
{
    Q_GADGET
    Q_PROPERTY(Position start MEMBER start)
    Q_PROPERTY(Position end MEMBER end)

public:
    Position start;
    Position end;

    bool operator==(const Range& other) const
    {
        return start == other.start && end == other.end;
    }

    QJsonObject toJson() const;
    static Range fromJson(const QJsonObject& obj);
};

struct Location
{
    Q_GADGET
    Q_PROPERTY(QString uri MEMBER uri)
    Q_PROPERTY(Range range MEMBER range)

public:
    QString uri;
    Range range;

    bool operator==(const Location& other) const
    {
        return uri == other.uri && range == other.range;
    }

    static Location fromJson(const QJsonObject& obj);
};

struct Diagnostic
{
    Q_GADGET
    Q_PROPERTY(Range range MEMBER range)
    Q_PROPERTY(QString message MEMBER message)
    Q_PROPERTY(QString severity MEMBER severity)
    Q_PROPERTY(int severityLevel MEMBER severityLevel)
    Q_PROPERTY(QString source MEMBER source)
    Q_PROPERTY(QString code MEMBER code)

public:
    Range range;
    QString message;
    QString severity;
    int severityLevel = 1;
    QString source;
    QString code;

    bool operator==(const Diagnostic& other) const
    {
        return range == other.range && message == other.message && severity == other.severity && severityLevel == other.severityLevel &&
               source == other.source && code == other.code;
    }

    static Diagnostic fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
};

struct CompletionItem
{
    Q_GADGET
    Q_PROPERTY(QString label MEMBER label)
    Q_PROPERTY(QString kind MEMBER kind)
    Q_PROPERTY(QString detail MEMBER detail)
    Q_PROPERTY(QString documentation MEMBER documentation)
    Q_PROPERTY(QString insertText MEMBER insertText)
    Q_PROPERTY(int startPos MEMBER startPos)
    Q_PROPERTY(int replaceLen MEMBER replaceLen)

public:
    QString label;
    QString kind;
    QString detail;
    QString documentation;
    QString insertText;
    int startPos = 0;
    int replaceLen = 0;

    bool operator==(const CompletionItem& other) const
    {
        return label == other.label && kind == other.kind && detail == other.detail && documentation == other.documentation &&
               insertText == other.insertText && startPos == other.startPos && replaceLen == other.replaceLen;
    }

    static CompletionItem fromJson(const QJsonObject& obj);
};

struct CompletionList
{
    Q_GADGET
    Q_PROPERTY(bool isIncomplete MEMBER isIncomplete)
    Q_PROPERTY(QList<CompletionItem> items MEMBER items)

public:
    bool isIncomplete = false;
    QList<CompletionItem> items;

    bool operator==(const CompletionList& other) const
    {
        return isIncomplete == other.isIncomplete && items == other.items;
    }
};

struct ServerCapabilities
{
    bool completionProvider = false;
    bool definitionProvider = false;
    bool referencesProvider = false;
    bool hoverProvider = false;
    bool documentSymbolProvider = false;
    bool signatureHelpProvider = false;
    bool formattingProvider = false;
    bool codeActionProvider = false;
    bool diagnosticProvider = false;
    bool renameProvider = false;
    bool documentHighlightProvider = false;
    bool typeDefinitionProvider = false;
    bool declarationProvider = false;
    bool workspaceSymbolProvider = false;
    bool selectionRangeProvider = false;
    bool linkedEditingRangeProvider = false;
    bool callHierarchyProvider = false;
    bool semanticTokensProvider = false;
    QList<QString> completionTriggerChars;

    static ServerCapabilities fromJson(const QJsonObject& caps);
};

QString uriFromPath(const QString& path);
QString pathFromUri(const QString& uri);

// Convert between QScintilla byte offsets within a line and LSP UTF-16
// character counts. QScintilla's getCursorPosition()/getSelection() report
// byte offsets (QScintilla stores documents as UTF-8), while the LSP spec
// requires Position.character to be a UTF-16 code unit count.
int utf16ToByteOffset(const QString& lineText, int utf16Character);
int byteOffsetToUtf16(const QString& lineText, int byteOffset);

} // namespace lsp

#endif // LSPTYPES_H
