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
    static const char* kindNames[] = {"Text",      "Method", "Function", "Constructor",  "Field",  "Variable",   "Class",
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
    QJsonObject td = caps["textDocument"].toObject();
    c.completionProvider = td.contains("completion");
    c.definitionProvider = td.contains("definition");
    c.referencesProvider = td.contains("references");
    c.hoverProvider = td.contains("hover");
    c.documentSymbolProvider = td.contains("documentSymbol");
    c.signatureHelpProvider = td.contains("signatureHelp");
    c.formattingProvider = td.contains("formatting");
    c.codeActionProvider = td.contains("codeAction");
    c.renameProvider = td.contains("rename");
    c.documentHighlightProvider = td.contains("documentHighlight");
    c.typeDefinitionProvider = td.contains("typeDefinition");
    c.declarationProvider = td.contains("declaration");
    c.selectionRangeProvider = td.contains("selectionRange");
    c.linkedEditingRangeProvider = td.contains("linkedEditingRange");
    c.callHierarchyProvider = td.contains("callHierarchy");
    c.semanticTokensProvider = td.contains("semanticTokens");
    c.diagnosticProvider = caps.contains("diagnostic");

    QJsonObject ws = caps["workspace"].toObject();
    c.workspaceSymbolProvider = ws.contains("symbol");

    if (c.completionProvider)
    {
        QJsonObject comp = td["completion"].toObject();
        QJsonObject compOpts = comp["completionItem"].toObject();
        for (const auto& ch : compOpts["triggerCharacters"].toArray())
            c.completionTriggerChars.append(ch.toString());
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

} // namespace lsp
