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

} // namespace lsp
