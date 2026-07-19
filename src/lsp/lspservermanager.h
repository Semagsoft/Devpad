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
#ifndef LSPSERVERMANAGER_H
#define LSPSERVERMANAGER_H

#include "lspclient.h"
#include "lsptypes.h"

#include <QHash>
#include <QMap>
#include <QObject>

namespace lsp
{

class LspServerManager : public QObject
{
    Q_OBJECT

public:
    explicit LspServerManager(QObject* parent = nullptr);
    ~LspServerManager() override;

    LspClient* clientForLanguage(const QString& language);
    LspClient* clientForUri(const QString& uri);

    void openDocument(const QString& language, const QString& uri, const QString& text);
    void changeDocument(const QString& uri, const QString& text);
    void closeDocument(const QString& uri);
    void saveDocument(const QString& uri);

    bool hasCapability(const QString& uri, const QString& capability) const;
    QString rootUriForFile(const QString& filePath) const;

    QList<QString> languages() const
    {
        return m_clients.keys();
    }

    static QHash<QString, QString> defaultServerCommands();
    static QStringList defaultServerArgs(const QString& language);

signals:
    void diagnosticsReady(const QString& uri, const QList<Diagnostic>& diagnostics);
    void completionReady(const QString& uri, const CompletionList& completions);
    void definitionReady(const QString& uri, const Location& location);
    void referencesReady(const QString& uri, const QList<Location>& locations);
    void hoverReady(const QString& uri, const QString& contents);
    void codeActionReady(const QString& uri, const QList<QJsonObject>& actions);
    void documentSymbolsReady(const QString& uri, const QJsonArray& symbols);
    void formattingReady(const QString& uri, const QList<QJsonObject>& edits);
    void rangeFormattingReady(const QString& uri, const QList<QJsonObject>& edits);
    void signatureHelpReady(const QString& uri, const QJsonObject& info);
    void renameReady(const QString& uri, const QJsonObject& result);
    void documentHighlightReady(const QString& uri, const QJsonArray& highlights);
    void typeDefinitionReady(const QString& uri, const Location& location);
    void declarationReady(const QString& uri, const Location& location);
    void workspaceSymbolsReady(const QString& query, const QJsonArray& symbols);
    void selectionRangesReady(const QString& uri, const QJsonArray& ranges);
    void linkedEditingRangeReady(const QString& uri, const QJsonObject& result);
    void callHierarchyReady(const QString& uri, const QJsonArray& items);
    void incomingCallsReady(const QString& uri, const QJsonArray& calls);
    void outgoingCallsReady(const QString& uri, const QJsonArray& calls);
    void semanticTokensFullReady(const QString& uri, const QJsonArray& tokens);

private:
    QHash<QString, LspClient*> m_clients;       // language -> client
    QMap<QString, QString> m_documentLanguages; // uri -> language
    QMap<QString, int> m_documentVersions;      // uri -> version
};

} // namespace lsp

#endif // LSPSERVERMANAGER_H
