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
#include <QString>
#include <QUrl>

namespace lsp
{

struct Position
{
    int line = 0;
    int character = 0;

    QJsonObject toJson() const;
    static Position fromJson(const QJsonObject& obj);
};

struct Range
{
    Position start;
    Position end;

    QJsonObject toJson() const;
    static Range fromJson(const QJsonObject& obj);
};

struct Location
{
    QString uri;
    Range range;

    static Location fromJson(const QJsonObject& obj);
};

struct Diagnostic
{
    Range range;
    QString message;
    QString severity;
    int severityLevel = 1;
    QString source;
    QString code;

    static Diagnostic fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
};

struct CompletionItem
{
    QString label;
    QString kind;
    QString detail;
    QString documentation;
    QString insertText;
    int startPos = 0;
    int replaceLen = 0;

    static CompletionItem fromJson(const QJsonObject& obj);
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

} // namespace lsp

#endif // LSPTYPES_H
