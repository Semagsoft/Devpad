#ifndef LSPTYPES_H
#define LSPTYPES_H

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QUrl>

namespace lsp {

struct Position
{
    int line = 0;
    int character = 0;

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["line"] = line;
        obj["character"] = character;
        return obj;
    }

    static Position fromJson(const QJsonObject& obj)
    {
        return { obj["line"].toInt(), obj["character"].toInt() };
    }
};

struct Range
{
    Position start;
    Position end;

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["start"] = start.toJson();
        obj["end"] = end.toJson();
        return obj;
    }

    static Range fromJson(const QJsonObject& obj)
    {
        Range r;
        r.start = Position::fromJson(obj["start"].toObject());
        r.end = Position::fromJson(obj["end"].toObject());
        return r;
    }
};

struct Location
{
    QString uri;
    Range range;

    static Location fromJson(const QJsonObject& obj)
    {
        Location loc;
        loc.uri = obj["uri"].toString();
        loc.range = Range::fromJson(obj["range"].toObject());
        return loc;
    }
};

struct Diagnostic
{
    Range range;
    QString message;
    QString severity; // "Error", "Warning", "Information", "Hint"
    int severityLevel = 1; // 1=error, 2=warning, 3=info, 4= hint
    QString source;
    QString code;

    static Diagnostic fromJson(const QJsonObject& obj)
    {
        Diagnostic d;
        d.range = Range::fromJson(obj["range"].toObject());
        d.message = obj["message"].toString();
        int sev = obj["severity"].toInt(1);
        d.severityLevel = sev;
        switch (sev) {
        case 1: d.severity = "Error"; break;
        case 2: d.severity = "Warning"; break;
        case 3: d.severity = "Information"; break;
        case 4: d.severity = "Hint"; break;
        default: d.severity = "Error";
        }
        d.source = obj["source"].toString();
        if (obj.contains("code"))
            d.code = QString::number(obj["code"].toVariant().toLongLong());
        return d;
    }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["range"] = range.toJson();
        obj["message"] = message;
        obj["severity"] = severityLevel;
        if (!source.isEmpty()) obj["source"] = source;
        if (!code.isEmpty()) obj["code"] = code;
        return obj;
    }
};

struct CompletionItem
{
    QString label;
    QString kind; // "Text", "Method", "Function", "Constructor", "Field", "Variable", "Class", etc.
    QString detail;
    QString documentation;
    QString insertText;
    int startPos = 0;
    int replaceLen = 0;

    static CompletionItem fromJson(const QJsonObject& obj)
    {
        CompletionItem item;
        item.label = obj["label"].toString();
        int k = obj["kind"].toInt(0);
        static const char* kindNames[] = {
            "Text", "Method", "Function", "Constructor", "Field", "Variable",
            "Class", "Interface", "Module", "Property", "Unit", "Value",
            "Enum", "Keyword", "Snippet", "Color", "File", "Reference",
            "Folder", "EnumMember", "Constant", "Struct", "Event", "Operator", "TypeParameter"
        };
        if (k >= 0 && k <= 25)
            item.kind = QString::fromLatin1(kindNames[k]);
        item.detail = obj["detail"].toString();
        item.documentation = obj["documentation"].toString();
        if (obj.contains("textEdit")) {
            auto te = obj["textEdit"].toObject();
            item.insertText = te["newText"].toString();
            item.startPos = -1;
            item.replaceLen = -1;
        } else {
            item.insertText = obj["insertText"].toString(item.label);
        }
        return item;
    }
};

struct CompletionList
{
    bool isIncomplete = false;
    QList<CompletionItem> items;
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
    bool diagnosticProvider = false; // 3.17+ diagnostic pull model
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

    static ServerCapabilities fromJson(const QJsonObject& caps)
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

        if (c.completionProvider) {
            QJsonObject comp = td["completion"].toObject();
            QJsonObject compOpts = comp["completionItem"].toObject();
            for (const auto& ch : compOpts["triggerCharacters"].toArray())
                c.completionTriggerChars.append(ch.toString());
        }
        return c;
    }
};

inline QString uriFromPath(const QString& path)
{
    if (path.startsWith("file://"))
        return path;
    QUrl url = QUrl::fromLocalFile(path);
    return url.toString();
}

inline QString pathFromUri(const QString& uri)
{
    return QUrl(uri).toLocalFile();
}

} // namespace lsp

#endif // LSPTYPES_H
