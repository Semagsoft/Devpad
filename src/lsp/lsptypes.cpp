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
#include "lsptypes.h"

#include <array>

namespace lsp
{

QJsonObject Position::toJson() const
{
    QJsonObject obj;
    obj["line"] = line;
    obj["character"] = character;
    return obj;
}

Position Position::fromJson(const QJsonObject& obj)
{
    return {obj["line"].toInt(), obj["character"].toInt()};
}

QJsonObject Range::toJson() const
{
    QJsonObject obj;
    obj["start"] = start.toJson();
    obj["end"] = end.toJson();
    return obj;
}

Range Range::fromJson(const QJsonObject& obj)
{
    Range r;
    r.start = Position::fromJson(obj["start"].toObject());
    r.end = Position::fromJson(obj["end"].toObject());
    return r;
}

Location Location::fromJson(const QJsonObject& obj)
{
    Location loc;
    loc.uri = obj["uri"].toString();
    loc.range = Range::fromJson(obj["range"].toObject());
    return loc;
}

Diagnostic Diagnostic::fromJson(const QJsonObject& obj)
{
    Diagnostic d;
    d.range = Range::fromJson(obj["range"].toObject());
    d.message = obj["message"].toString();
    int sev = obj["severity"].toInt(1);
    d.severityLevel = sev;
    switch (sev)
    {
    case 1:
        d.severity = "Error";
        break;
    case 2:
        d.severity = "Warning";
        break;
    case 3:
        d.severity = "Information";
        break;
    case 4:
        d.severity = "Hint";
        break;
    default:
        d.severity = "Error";
    }
    d.source = obj["source"].toString();
    if (obj.contains("code"))
        d.code = QString::number(obj["code"].toVariant().toLongLong());
    return d;
}

QJsonObject Diagnostic::toJson() const
{
    QJsonObject obj;
    obj["range"] = range.toJson();
    obj["message"] = message;
    obj["severity"] = severityLevel;
    if (!source.isEmpty())
        obj["source"] = source;
    if (!code.isEmpty())
        obj["code"] = code;
    return obj;
}

CompletionItem CompletionItem::fromJson(const QJsonObject& obj)
{
    CompletionItem item;
    item.label = obj["label"].toString();
    int k = obj["kind"].toInt(0);
    static constexpr std::array kindNames{"Text",      "Method", "Function", "Constructor",  "Field",  "Variable",   "Class",
                                          "Interface", "Module", "Property", "Unit",         "Value",  "Enum",       "Keyword",
                                          "Snippet",   "Color",  "File",     "Reference",    "Folder", "EnumMember", "Constant",
                                          "Struct",    "Event",  "Operator", "TypeParameter"};
    if (k >= 0 && k <= 25)
        item.kind = QString::fromLatin1(kindNames[k]);
    item.detail = obj["detail"].toString();
    item.documentation = obj["documentation"].toString();
    if (obj.contains("textEdit"))
    {
        auto te = obj["textEdit"].toObject();
        item.insertText = te["newText"].toString();
        item.startPos = -1;
        item.replaceLen = -1;
    }
    else
    {
        item.insertText = obj["insertText"].toString(item.label);
    }
    return item;
}

ServerCapabilities ServerCapabilities::fromJson(const QJsonObject& caps)
{
    ServerCapabilities c;
    // Per LSP spec ServerCapabilities are top-level keys
    auto hasTopLevel = [&caps](const QString& key) { return caps.contains(key); };
    auto hasCompletionTrigger = [&caps](QStringList& out) -> bool
    {
        QJsonValue compVal = caps["completionProvider"];
        if (compVal.isObject())
        {
            QJsonObject compObj = compVal.toObject();
            QJsonArray arr = compObj["triggerCharacters"].toArray();
            for (const auto& ch : arr)
                out.append(ch.toString());
            return true;
        }
        return compVal.isBool() && compVal.toBool();
    };

    c.completionProvider = hasTopLevel("completionProvider");
    c.definitionProvider = hasTopLevel("definitionProvider");
    c.referencesProvider = hasTopLevel("referencesProvider");
    c.hoverProvider = hasTopLevel("hoverProvider");
    c.documentSymbolProvider = hasTopLevel("documentSymbolProvider");
    c.signatureHelpProvider = hasTopLevel("signatureHelpProvider");
    c.formattingProvider = hasTopLevel("documentFormattingProvider") || hasTopLevel("formattingProvider");
    c.codeActionProvider = hasTopLevel("codeActionProvider");
    c.renameProvider = hasTopLevel("renameProvider");
    c.documentHighlightProvider = hasTopLevel("documentHighlightProvider");
    c.typeDefinitionProvider = hasTopLevel("typeDefinitionProvider");
    c.declarationProvider = hasTopLevel("declarationProvider");
    c.selectionRangeProvider = hasTopLevel("selectionRangeProvider");
    c.linkedEditingRangeProvider = hasTopLevel("linkedEditingRangeProvider");
    c.callHierarchyProvider = hasTopLevel("callHierarchyProvider");
    c.semanticTokensProvider = hasTopLevel("semanticTokensProvider");
    c.diagnosticProvider = hasTopLevel("diagnosticProvider") || hasTopLevel("diagnostic");

    // Workspace symbols
    QJsonValue wsVal = caps["workspaceSymbolProvider"];
    if (!wsVal.isUndefined())
    {
        c.workspaceSymbolProvider = wsVal.isBool() ? wsVal.toBool() : true;
    }
    else
    {
        QJsonObject ws = caps["workspace"].toObject();
        c.workspaceSymbolProvider = ws.contains("symbol") || ws.contains("workspaceSymbol");
    }

    // Legacy fallback: some servers embed capabilities under textDocument.* (previous bug)
    if (!c.completionProvider || !c.definitionProvider)
    {
        QJsonObject td = caps["textDocument"].toObject();
        if (!td.isEmpty())
        {
            c.completionProvider = c.completionProvider || td.contains("completion");
            c.definitionProvider = c.definitionProvider || td.contains("definition");
            c.referencesProvider = c.referencesProvider || td.contains("references");
            c.hoverProvider = c.hoverProvider || td.contains("hover");
            c.documentSymbolProvider = c.documentSymbolProvider || td.contains("documentSymbol");
            c.signatureHelpProvider = c.signatureHelpProvider || td.contains("signatureHelp");
            c.formattingProvider = c.formattingProvider || td.contains("formatting");
            c.codeActionProvider = c.codeActionProvider || td.contains("codeAction");
            c.renameProvider = c.renameProvider || td.contains("rename");
            c.documentHighlightProvider = c.documentHighlightProvider || td.contains("documentHighlight");
            c.typeDefinitionProvider = c.typeDefinitionProvider || td.contains("typeDefinition");
            c.declarationProvider = c.declarationProvider || td.contains("declaration");
            c.selectionRangeProvider = c.selectionRangeProvider || td.contains("selectionRange");
            c.linkedEditingRangeProvider = c.linkedEditingRangeProvider || td.contains("linkedEditingRange");
            c.callHierarchyProvider = c.callHierarchyProvider || td.contains("callHierarchy");
            c.semanticTokensProvider = c.semanticTokensProvider || td.contains("semanticTokens");
            QJsonObject ws = caps["workspace"].toObject();
            c.workspaceSymbolProvider = c.workspaceSymbolProvider || ws.contains("symbol");
        }
    }

    if (c.completionProvider)
    {
        QStringList triggers;
        if (hasCompletionTrigger(triggers))
        {
            c.completionTriggerChars = triggers;
        }
        else
        {
            // Legacy path: completionItem.triggerCharacters nested incorrectly
            QJsonObject td = caps["textDocument"].toObject();
            QJsonObject comp = td["completion"].toObject();
            QJsonObject compOpts = comp["completionItem"].toObject();
            for (const auto& ch : compOpts["triggerCharacters"].toArray())
                c.completionTriggerChars.append(ch.toString());
        }
    }
    return c;
}

QString uriFromPath(const QString& path)
{
    if (path.startsWith("file://"))
        return path;
    QUrl url = QUrl::fromLocalFile(path);
    return url.toString();
}

QString pathFromUri(const QString& uri)
{
    return QUrl(uri).toLocalFile();
}

// ── Line column model ────────────────────────────────────────
// Three column units coexist in this codebase. Pick the right
// conversion instead of reusing the wrong one:
//
//   1. LSP positions: column is a count of UTF-16 code units
//      (astral characters count as 2).
//   2. QScintilla line/index APIs: index is a count of *code points*
//      (SCI_POSITIONAFTER steps one code point at a time).
//   3. Raw Scintilla positions: byte offsets in the UTF-8 document.
//
// The four primitive directions of the triangle are implemented from a
// single code-point walker so all helpers agree, including on astral
// characters and out-of-range/clamped inputs.

// Counts of the first `cpLimit` code points of `lineText`.
static void countsAfterNCodePoints(const QString& lineText, int cpLimit, int* utf16Out, int* bytesOut)
{
    int utf16 = 0, bytes = 0, cps = 0;
    for (int i = 0; i < lineText.size() && cps < cpLimit;)
    {
        QChar c = lineText.at(i);
        uint cp = c.unicode();
        int steps = 1;
        if ((c.unicode() >= 0xD800 && c.unicode() < 0xDC00) && i + 1 < lineText.size() && (lineText.at(i + 1).unicode() >= 0xDC00 && lineText.at(i + 1).unicode() < 0xE000))
        {
            cp = 0x10000u + ((static_cast<uint>(c.unicode()) - 0xD800u) << 10) + (static_cast<uint>(lineText.at(i + 1).unicode()) - 0xDC00u);
            steps = 2;
        }
        utf16 += steps;
        bytes += cp <= 0x7F ? 1 : (cp <= 0x7FF ? 2 : (cp <= 0xFFFF ? 3 : 4));
        i += steps;
        ++cps;
    }
    *utf16Out = utf16;
    *bytesOut = bytes;
}

// Code point index of the first code unit at or after utf16Col. A UTF-16
// column that falls inside an astral character is clamped to that
// character's start.
int utf16ColToCharIndex(const QString& lineText, int utf16Col)
{
    if (utf16Col <= 0)
        return 0;
    int cps = 0, units = 0;
    for (int i = 0; i < lineText.size();)
    {
        if (units >= utf16Col)
            break;
        QChar c = lineText.at(i);
        bool astral = (c.unicode() >= 0xD800 && c.unicode() < 0xDC00) && i + 1 < lineText.size() && (lineText.at(i + 1).unicode() >= 0xDC00 && lineText.at(i + 1).unicode() < 0xE000);
        int step = astral ? 2 : 1;
        if (units + step > utf16Col)
            break; // Column lands inside an astral character; take its start.
        units += step;
        i += step;
        ++cps;
    }
    return cps;
}

int charIndexToUtf16Col(const QString& lineText, int charIndex)
{
    if (charIndex <= 0)
        return 0;
    int utf16 = 0, bytes = 0;
    countsAfterNCodePoints(lineText, charIndex, &utf16, &bytes);
    return utf16;
}

int charIndexToByteOffset(const QString& lineText, int charIndex)
{
    if (charIndex <= 0)
        return 0;
    int utf16 = 0, bytes = 0;
    countsAfterNCodePoints(lineText, charIndex, &utf16, &bytes);
    return bytes;
}

int byteOffsetToCharIndex(const QString& lineText, int byteOffset)
{
    if (byteOffset <= 0)
        return 0;
    int cps = 0, bytes = 0;
    for (int i = 0; i < lineText.size();)
    {
        QChar c = lineText.at(i);
        uint cp = c.unicode();
        int steps = 1;
        if ((c.unicode() >= 0xD800 && c.unicode() < 0xDC00) && i + 1 < lineText.size() && (lineText.at(i + 1).unicode() >= 0xDC00 && lineText.at(i + 1).unicode() < 0xE000))
        {
            cp = 0x10000u + ((static_cast<uint>(c.unicode()) - 0xD800u) << 10) + (static_cast<uint>(lineText.at(i + 1).unicode()) - 0xDC00u);
            steps = 2;
        }
        int cpBytes = cp <= 0x7F ? 1 : (cp <= 0x7FF ? 2 : (cp <= 0xFFFF ? 3 : 4));
        if (bytes + cpBytes > byteOffset)
            break;
        bytes += cpBytes;
        i += steps;
        ++cps;
    }
    return cps;
}

int utf16ToByteOffset(const QString& lineText, int utf16Character)
{
    return charIndexToByteOffset(lineText, utf16ColToCharIndex(lineText, utf16Character));
}

int byteOffsetToUtf16(const QString& lineText, int byteOffset)
{
    return charIndexToUtf16Col(lineText, byteOffsetToCharIndex(lineText, byteOffset));
}

} // namespace lsp
