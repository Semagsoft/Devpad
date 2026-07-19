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
#ifndef LSPCLIENT_H
#define LSPCLIENT_H

#include "lspjsonrpc.h"
#include "lsptypes.h"

#include <QObject>
#include <QProcess>
#include <QTimer>

namespace lsp
{

class LspClient : public QObject
{
    Q_OBJECT

public:
    explicit LspClient(const QString& language, QObject* parent = nullptr);
    ~LspClient() override;

    void startServer(const QString& command, const QStringList& args, const QString& rootUri);
    void stopServer();
    bool isRunning() const;
    const QString& language() const
    {
        return m_language;
    }
    const ServerCapabilities& capabilities() const
    {
        return m_capabilities;
    }
    QString rootUri() const
    {
        return m_rootUri;
    }

    // Document synchronization
    void openDocument(const QString& uri, const QString& text, int version);
    void changeDocument(const QString& uri, const QString& text, int version);
    void closeDocument(const QString& uri);
    void saveDocument(const QString& uri);

    // LSP requests
    void requestCompletion(const QString& uri, const Position& pos, int triggerKind = 1);
    void requestDefinition(const QString& uri, const Position& pos);
    void requestReferences(const QString& uri, const Position& pos);
    void requestHover(const QString& uri, const Position& pos);
    void requestDocumentSymbols(const QString& uri);
    void requestFormatting(const QString& uri, int tabSize, bool insertSpaces);
    void requestRangeFormatting(const QString& uri, const Range& range, int tabSize, bool insertSpaces);
    void requestSignatureHelp(const QString& uri, const Position& pos);

    // Force re-diagnostics for a document
    void requestDiagnostics(const QString& uri);
    void requestRename(const QString& uri, const Position& pos, const QString& newName);
    void requestDocumentHighlight(const QString& uri, const Position& pos);
    void requestTypeDefinition(const QString& uri, const Position& pos);
    void requestDeclaration(const QString& uri, const Position& pos);
    void requestCodeAction(const QString& uri, const Range& range, const QList<Diagnostic>& diagnostics);
    void requestWorkspaceSymbols(const QString& query);
    void requestSelectionRanges(const QString& uri, const QList<Position>& positions);
    void requestLinkedEditingRange(const QString& uri, const Position& pos);
    void prepareCallHierarchy(const QString& uri, const Position& pos);
    void incomingCalls(const QString& uri, const QString& itemId);
    void outgoingCalls(const QString& uri, const QString& itemId);
    void requestSemanticTokensFull(const QString& uri);
    void sendDidChangeConfiguration(const QJsonObject& settings);

signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString& error);
    void capabilitiesReady(const ServerCapabilities& caps);

    // Results
    void completionReady(const QString& uri, const CompletionList& completions);
    void definitionReady(const QString& uri, const Location& location);
    void referencesReady(const QString& uri, const QList<Location>& locations);
    void hoverReady(const QString& uri, const QString& contents);
    void documentSymbolsReady(const QString& uri, const QJsonArray& symbols);
    void formattingReady(const QString& uri, const QList<QJsonObject>& edits);
    void rangeFormattingReady(const QString& uri, const QList<QJsonObject>& edits);
    void signatureHelpReady(const QString& uri, const QJsonObject& info);

    // Diagnostics
    void diagnosticsReady(const QString& uri, const QList<Diagnostic>& diagnostics);
    void renameReady(const QString& uri, const QJsonObject& result);
    void documentHighlightReady(const QString& uri, const QJsonArray& highlights);
    void typeDefinitionReady(const QString& uri, const Location& location);
    void declarationReady(const QString& uri, const Location& location);
    void codeActionReady(const QString& uri, const QList<QJsonObject>& actions);
    void workspaceSymbolsReady(const QString& query, const QJsonArray& symbols);
    void selectionRangesReady(const QString& uri, const QJsonArray& ranges);
    void linkedEditingRangeReady(const QString& uri, const QJsonObject& result);
    void callHierarchyReady(const QString& uri, const QJsonArray& items);
    void incomingCallsReady(const QString& uri, const QJsonArray& calls);
    void outgoingCallsReady(const QString& uri, const QJsonArray& calls);
    void semanticTokensFullReady(const QString& uri, const QJsonArray& tokens);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    void sendInitialize();
    void handleNotification(const QString& method, const QJsonObject& params);
    void handleResponse(int id, const QJsonValue& result, const QJsonObject& error);
    QString methodName(const QString& method) const;

    struct DocumentState
    {
        int version = 0;
        QString uri;
        QString text;
    };

    void tryRestart();

    QProcess* m_process = nullptr;
    LspJsonRpc* m_jsonRpc = nullptr;
    QString m_language;
    QString m_rootUri;
    QString m_command;
    QStringList m_args;
    ServerCapabilities m_capabilities;
    bool m_initialized = false;
    int m_retryCount = 0;
    static constexpr int MAX_RETRIES = 3;
};

} // namespace lsp

#endif // LSPCLIENT_H
