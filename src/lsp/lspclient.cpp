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
#include "lspclient.h"

#include "logger.h"
#include "settingsmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QProcess>

namespace lsp
{

LspClient::LspClient(const QString& language, QObject* parent) : QObject(parent), m_language(language)
{
    m_jsonRpc = new LspJsonRpc(this);

    connect(m_jsonRpc, &LspJsonRpc::notificationReceived, this, &LspClient::handleNotification);
    connect(m_jsonRpc, &LspJsonRpc::responseReceived, this, &LspClient::handleResponse);
    connect(m_jsonRpc, &LspJsonRpc::serverRequestReceived, this, &LspClient::handleServerRequest);
}

LspClient::~LspClient()
{
    // Best-effort graceful path (writes shutdown/exit synchronously to the
    // process pipe), then guarantee that no orphaned server process outlives
    // this object. QProcess's destructor does NOT kill a running child, so a
    // plain stopServer() while destructing would leak a zombie language server.
    stopServer();

    if (m_process)
    {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning)
        {
            m_process->terminate();
            if (!m_process->waitForFinished(KILL_TIMEOUT_MS))
                m_process->kill();
            m_process->waitForFinished(KILL_TIMEOUT_MS);
        }
        delete m_process;
        m_process = nullptr;
    }
}

void LspClient::startServer(const QString& command, const QStringList& args, const QString& rootUri)
{
    if (m_process && m_process->state() != QProcess::NotRunning)
        stopServer();

    m_command = command;
    m_args = args;
    m_rootUri = rootUri;
    m_initialized = false;
    m_starting = true;
    m_shutdownExitSent = false;

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &LspClient::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &LspClient::onReadyReadStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &LspClient::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &LspClient::onProcessError);
    connect(m_process, &QProcess::started, this, &LspClient::onProcessStarted);

    m_process->start(command, args);

    QPointer<QProcess> guard(m_process);
    QTimer::singleShot(START_TIMEOUT_MS, this,
                       [this, guard]()
                       {
                           if (!m_starting || !guard || guard != m_process)
                               return;
                           m_starting = false;
                           emit serverError(tr("Language server timed out starting: %1").arg(m_command));
                           stopServer();
                       });
}

void LspClient::onProcessStarted()
{
    if (!m_process)
        return;
    m_starting = false;
    sendInitialize();
    emit serverStarted();
}

void LspClient::stopServer()
{
    if (!m_process)
        return;

    m_starting = false;
    m_stopping = true;

    if (m_process->state() == QProcess::NotRunning)
    {
        cleanupProcess();
        return;
    }

    if (!m_shutdownExitSent)
    {
        m_shutdownExitSent = true;

        // LSP lifecycle: send "exit" only after the server has responded to
        // "shutdown" (or on timeout as a fallback). Until then, servers are
        // required to ignore requests but stay alive to answer them.
        int id = m_jsonRpc->nextRequestId();
        m_process->write(m_jsonRpc->createRequest(id, "shutdown", QJsonObject()));
        m_jsonRpc->registerPendingRequest(
            id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) { sendExitIfStopping(); }, SHUTDOWN_TIMEOUT_MS);
    }

    // Escalation chain: if there is no shutdown response within
    // SHUTDOWN_TIMEOUT_MS, send exit anyway (the fallback callback above races
    // with this) and terminate; after KILL_TIMEOUT_MS, kill.
    QPointer<QProcess> guard(m_process);
    QTimer::singleShot(SHUTDOWN_TIMEOUT_MS, this,
                       [this, guard]()
                       {
                           if (!guard || guard != m_process || !m_stopping)
                               return;
                           if (m_process->state() != QProcess::NotRunning)
                           {
                               sendExitIfStopping();
                               m_process->terminate();
                               QTimer::singleShot(KILL_TIMEOUT_MS, this,
                                                  [this, guard]()
                                                  {
                                                      if (!guard || guard != m_process || !m_stopping)
                                                          return;
                                                      if (m_process->state() != QProcess::NotRunning)
                                                          m_process->kill();
                                                  });
                           }
                       });
}

void LspClient::sendExitIfStopping()
{
    if (!m_stopping || !m_process)
        return;
    m_process->write(m_jsonRpc->createNotification("exit", QJsonObject()));
}

bool LspClient::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

void LspClient::sendInitialize()
{
    QJsonObject params;
    QJsonObject clientInfo;
    clientInfo["name"] = QStringLiteral("Devpad");
    clientInfo["version"] = QCoreApplication::applicationVersion();
    params["processId"] = QCoreApplication::applicationPid();
    params["clientInfo"] = clientInfo;
    params["rootUri"] = m_rootUri;

    QJsonObject capabilities;
    QJsonObject textDocument;
    QJsonObject sync;
    sync["textDocumentSync"] = 1; // Full document sync
    textDocument["synchronization"] = sync;
    QJsonArray docFormats;
    docFormats.append(QStringLiteral("plaintext"));
    QJsonObject completionItem;
    completionItem["documentationFormat"] = docFormats;
    QJsonObject completion;
    completion["completionItem"] = completionItem;
    textDocument["completion"] = completion;
    textDocument["definition"] = QJsonObject();
    textDocument["references"] = QJsonObject();
    textDocument["hover"] = QJsonObject();
    textDocument["documentSymbol"] = QJsonObject();
    textDocument["signatureHelp"] = QJsonObject();
    textDocument["codeAction"] = QJsonObject();
    textDocument["formatting"] = QJsonObject();
    textDocument["rangeFormatting"] = QJsonObject();
    textDocument["selectionRange"] = QJsonObject();
    textDocument["linkedEditingRange"] = QJsonObject();
    textDocument["callHierarchy"] = QJsonObject();
    textDocument["semanticTokens"] = QJsonObject();

    QJsonObject workspace;
    workspace["symbol"] = QJsonObject();
    capabilities["workspace"] = workspace;
    textDocument["typeDefinition"] = QJsonObject();
    textDocument["declaration"] = QJsonObject();
    capabilities["textDocument"] = textDocument;

    QJsonObject diagnosticOpts;
    diagnosticOpts["relatedDocumentSupport"] = false;
    capabilities["diagnostic"] = diagnosticOpts;

    params["capabilities"] = capabilities;
    params["trace"] = QStringLiteral("off");

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(
        id,
        [this](const QJsonValue& result, const QJsonObject& error)
        {
            if (!error.isEmpty())
            {
                // A failed initialize leaves the session unusable;
                // per the LSP lifecycle the client sends exit and stops.
                emit serverError(QStringLiteral("[%1] %2").arg(QString::number(error["code"].toInt(0)), error["message"].toString()));
                m_starting = false;
                stopServer();
                return;
            }
            m_retryCount = 0;
            m_capabilities = ServerCapabilities::fromJson(result.toObject()["capabilities"].toObject());
            m_initialized = true;
            QByteArray notification = m_jsonRpc->createNotification("initialized", QJsonObject());
            if (m_process)
                m_process->write(notification);
            flushPendingDocOps();
        });

    QByteArray request = m_jsonRpc->createRequest(id, "initialize", params);
    if (m_process)
        m_process->write(request);
}

// ── Document Sync ──────────────────────────────────────────────

void LspClient::openDocument(const QString& uri, const QString& text, int version)
{
    if (!isRunning() || !m_initialized)
    {
        m_pendingDocOps.append({PendingDocOp::Open, uri, text, version});
        return;
    }

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    textDoc["languageId"] = m_language;
    textDoc["version"] = version;
    textDoc["text"] = text;
    params["textDocument"] = textDoc;

    QByteArray notification = m_jsonRpc->createNotification("textDocument/didOpen", params);
    m_process->write(notification);
}

void LspClient::changeDocument(const QString& uri, const QString& text, int version)
{
    if (!isRunning() || !m_initialized)
    {
        m_pendingDocOps.append({PendingDocOp::Change, uri, text, version});
        return;
    }

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    textDoc["version"] = version;

    QJsonArray contentChanges;
    QJsonObject change;
    change["text"] = text;
    contentChanges.append(change);
    params["textDocument"] = textDoc;
    params["contentChanges"] = contentChanges;

    QByteArray notification = m_jsonRpc->createNotification("textDocument/didChange", params);
    m_process->write(notification);
}

void LspClient::closeDocument(const QString& uri)
{
    if (!isRunning() || !m_initialized)
    {
        // Only queue if the doc was actually opened (avoid queuing close for never-opened docs)
        for (const auto& op : m_pendingDocOps)
        {
            if (op.type == PendingDocOp::Open && op.uri == uri)
            {
                m_pendingDocOps.append({PendingDocOp::Close, uri, {}, 0});
                return;
            }
        }
        return;
    }

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;

    QByteArray notification = m_jsonRpc->createNotification("textDocument/didClose", params);
    m_process->write(notification);
}

void LspClient::saveDocument(const QString& uri)
{
    if (!isRunning() || !m_initialized)
    {
        m_pendingDocOps.append({PendingDocOp::Save, uri, {}, 0});
        return;
    }

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    textDoc["text"] = QString(); // optional; server may ignore
    params["textDocument"] = textDoc;

    QByteArray notification = m_jsonRpc->createNotification("textDocument/didSave", params);
    m_process->write(notification);
}

// ── LSP Requests ───────────────────────────────────────────────

void LspClient::requestCompletion(const QString& uri, const Position& pos, int triggerKind)
{
    if (!isRunning() || !m_initialized || !m_capabilities.completionProvider)
        return;

    QJsonObject params = makePositionParam(uri, pos);
    params["context"] = QJsonObject{{"triggerKind", triggerKind}};

    sendRequest("textDocument/completion", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/)
                {
                    CompletionList list;
                    QJsonArray items;
                    QJsonObject obj = result.toObject();
                    if (result.isObject() && obj.contains("items"))
                    {
                        items = obj["items"].toArray();
                        list.isIncomplete = obj["isIncomplete"].toBool();
                    }
                    else if (result.isArray())
                    {
                        items = result.toArray();
                    }
                    for (const auto& item : items)
                        list.items.append(CompletionItem::fromJson(item.toObject()));
                    emit completionReady(uri, list);
                });
}

void LspClient::requestDefinition(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.definitionProvider)
        return;
    requestLocation("textDocument/definition", uri, pos, "definitionReady");
}

void LspClient::requestTypeDefinition(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.typeDefinitionProvider)
        return;
    requestLocation("textDocument/typeDefinition", uri, pos, "typeDefinitionReady");
}

void LspClient::requestDeclaration(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.declarationProvider)
        return;
    requestLocation("textDocument/declaration", uri, pos, "declarationReady");
}

void LspClient::requestCodeAction(const QString& uri, const Range& range, const QList<Diagnostic>& diagnostics)
{
    if (!isRunning() || !m_initialized || !m_capabilities.codeActionProvider)
        return;

    QJsonObject params;
    params["textDocument"] = makeTextDocumentParam(uri);
    params["range"] = range.toJson();
    QJsonArray diags;
    for (const auto& d : diagnostics)
        diags.append(d.toJson());
    params["context"] = QJsonObject{{"diagnostics", diags}};

    sendRequest("textDocument/codeAction", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/)
                {
                    QList<QJsonObject> actions;
                    if (result.isArray())
                        for (const auto& v : result.toArray())
                            actions.append(v.toObject());
                    emit codeActionReady(uri, actions);
                });
}

void LspClient::requestWorkspaceSymbols(const QString& query)
{
    if (!isRunning() || !m_initialized || !m_capabilities.workspaceSymbolProvider)
        return;

    QJsonObject params{{"query", query}};
    sendRequest("workspace/symbol", params,
                [this, query](const QJsonValue& result, const QJsonObject& /*error*/) { emit workspaceSymbolsReady(query, result.toArray()); });
}

void LspClient::requestSelectionRanges(const QString& uri, const QList<Position>& positions)
{
    if (!isRunning() || !m_initialized || !m_capabilities.selectionRangeProvider)
        return;

    QJsonObject params;
    params["textDocument"] = makeTextDocumentParam(uri);
    QJsonArray posArr;
    for (const auto& p : positions)
        posArr.append(p.toJson());
    params["positions"] = posArr;
    sendRequest("textDocument/selectionRange", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/) { emit selectionRangesReady(uri, result.toArray()); });
}

void LspClient::requestLinkedEditingRange(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.linkedEditingRangeProvider)
        return;

    QJsonObject params = makePositionParam(uri, pos);
    sendRequest("textDocument/linkedEditingRange", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/) { emit linkedEditingRangeReady(uri, result.toObject()); });
}

void LspClient::prepareCallHierarchy(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.callHierarchyProvider)
        return;

    QJsonObject params = makePositionParam(uri, pos);
    sendRequest("textDocument/prepareCallHierarchy", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/) { emit callHierarchyReady(uri, result.toArray()); });
}

void LspClient::incomingCalls(const QString& uri, const QString& itemId)
{
    if (!isRunning() || !m_initialized || !m_capabilities.callHierarchyProvider)
        return;

    QJsonObject params{{"item", QJsonObject{{"kind", 12}, {"name", itemId}}}};
    sendRequest("callHierarchy/incomingCalls", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/) { emit incomingCallsReady(uri, result.toArray()); });
}

void LspClient::outgoingCalls(const QString& uri, const QString& itemId)
{
    if (!isRunning() || !m_initialized || !m_capabilities.callHierarchyProvider)
        return;

    QJsonObject params{{"item", QJsonObject{{"kind", 12}, {"name", itemId}}}};
    sendRequest("callHierarchy/outgoingCalls", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/) { emit outgoingCallsReady(uri, result.toArray()); });
}

void LspClient::requestSemanticTokensFull(const QString& uri)
{
    if (!isRunning() || !m_initialized || !m_capabilities.semanticTokensProvider)
        return;

    QJsonObject params{{"textDocument", makeTextDocumentParam(uri)}};
    sendRequest("textDocument/semanticTokens/full", params, [this, uri](const QJsonValue& result, const QJsonObject& /*error*/)
                { emit semanticTokensFullReady(uri, result.toObject()["data"].toArray()); });
}

void LspClient::requestReferences(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.referencesProvider)
        return;

    QJsonObject params = makePositionParam(uri, pos);
    params["context"] = QJsonObject{{"includeDeclaration", true}};

    sendRequest("textDocument/references", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/)
                {
                    QList<Location> locations;
                    if (result.isArray())
                        for (const auto& v : result.toArray())
                            locations.append(Location::fromJson(v.toObject()));
                    emit referencesReady(uri, locations);
                });
}

void LspClient::requestHover(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.hoverProvider)
        return;

    QJsonObject params = makePositionParam(uri, pos);
    sendRequest("textDocument/hover", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/)
                {
                    QString contents;
                    QJsonObject obj = result.toObject();
                    QJsonValue val = obj["contents"];
                    if (val.isString())
                        contents = val.toString();
                    else if (val.isObject())
                        contents = val.toObject()["value"].toString();
                    else if (val.isArray())
                    {
                        QStringList parts;
                        for (const auto& v : val.toArray())
                        {
                            if (v.isString())
                                parts.append(v.toString());
                            else if (v.isObject())
                                parts.append(v.toObject()["value"].toString());
                        }
                        contents = parts.join("\n");
                    }
                    emit hoverReady(uri, contents);
                });
}

void LspClient::requestDocumentSymbols(const QString& uri)
{
    if (!isRunning() || !m_initialized || !m_capabilities.documentSymbolProvider)
        return;

    QJsonObject params{{"textDocument", makeTextDocumentParam(uri)}};
    sendRequest("textDocument/documentSymbol", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/)
                {
                    QJsonArray symbols;
                    if (result.isArray())
                        symbols = result.toArray();
                    else if (result.isObject() && result.toObject().contains("symbols"))
                        symbols = result.toObject()["symbols"].toArray();
                    emit documentSymbolsReady(uri, symbols);
                });
}

void LspClient::requestFormatting(const QString& uri, int tabSize, bool insertSpaces)
{
    if (!isRunning() || !m_initialized || !m_capabilities.formattingProvider)
        return;

    QJsonObject params;
    params["textDocument"] = makeTextDocumentParam(uri);
    params["options"] = QJsonObject{{"tabSize", tabSize}, {"insertSpaces", insertSpaces}};
    sendRequest("textDocument/formatting", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/)
                {
                    QList<QJsonObject> edits;
                    if (result.isArray())
                        for (const auto& v : result.toArray())
                            edits.append(v.toObject());
                    emit formattingReady(uri, edits);
                });
}

void LspClient::requestRangeFormatting(const QString& uri, const Range& range, int tabSize, bool insertSpaces)
{
    if (!isRunning() || !m_initialized || !m_capabilities.formattingProvider)
        return;

    QJsonObject params;
    params["textDocument"] = makeTextDocumentParam(uri);
    params["range"] = range.toJson();
    params["options"] = QJsonObject{{"tabSize", tabSize}, {"insertSpaces", insertSpaces}};
    sendRequest("textDocument/rangeFormatting", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/)
                {
                    QList<QJsonObject> edits;
                    if (result.isArray())
                        for (const auto& v : result.toArray())
                            edits.append(v.toObject());
                    emit rangeFormattingReady(uri, edits);
                });
}

void LspClient::requestSignatureHelp(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.signatureHelpProvider)
        return;

    QJsonObject params = makePositionParam(uri, pos);
    sendRequest("textDocument/signatureHelp", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/) { emit signatureHelpReady(uri, result.toObject()); });
}

void LspClient::requestCompletionAt(const QString& uri, int line, int character, int triggerKind)
{
    Position p;
    p.line = line;
    p.character = character;
    requestCompletion(uri, p, triggerKind);
}

void LspClient::requestDefinitionAt(const QString& uri, int line, int character)
{
    Position p;
    p.line = line;
    p.character = character;
    requestDefinition(uri, p);
}

void LspClient::requestHoverAt(const QString& uri, int line, int character)
{
    Position p;
    p.line = line;
    p.character = character;
    requestHover(uri, p);
}

void LspClient::requestSignatureHelpAt(const QString& uri, int line, int character)
{
    Position p;
    p.line = line;
    p.character = character;
    requestSignatureHelp(uri, p);
}

void LspClient::requestDiagnostics(const QString& uri)
{
    if (!isRunning() || !m_initialized || !m_capabilities.diagnosticProvider)
        return;

    QJsonObject params{{"textDocument", makeTextDocumentParam(uri)}, {"identifier", QStringLiteral("default")}};
    sendRequest("textDocument/diagnostic", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/)
                {
                    QList<Diagnostic> diagnostics;
                    QJsonArray items;
                    if (result.isObject() && result.toObject().contains("items"))
                        items = result.toObject()["items"].toArray();
                    else if (result.isArray())
                        items = result.toArray();
                    for (const auto& v : items)
                        diagnostics.append(Diagnostic::fromJson(v.toObject()));
                    emit diagnosticsReady(uri, diagnostics);
                });
}

void LspClient::requestRename(const QString& uri, const Position& pos, const QString& newName)
{
    if (!isRunning() || !m_initialized || !m_capabilities.renameProvider)
        return;

    QJsonObject params = makePositionParam(uri, pos);
    params["newName"] = newName;
    sendRequest("textDocument/rename", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/) { emit renameReady(uri, result.toObject()); });
}

void LspClient::requestDocumentHighlight(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.documentHighlightProvider)
        return;

    QJsonObject params = makePositionParam(uri, pos);
    sendRequest("textDocument/documentHighlight", params,
                [this, uri](const QJsonValue& result, const QJsonObject& /*error*/) { emit documentHighlightReady(uri, result.toArray()); });
}

void LspClient::sendDidChangeConfiguration(const QJsonObject& settings)
{
    if (!isRunning() || !m_initialized)
        return;

    QJsonObject params;
    params["settings"] = settings;

    QByteArray notification = m_jsonRpc->createNotification("workspace/didChangeConfiguration", params);
    m_process->write(notification);
}

int LspClient::sendRequest(const QString& method, const QJsonObject& params, LspJsonRpc::ResponseCallback callback)
{
    if (!m_process || !isRunning())
        return -1;
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, std::move(callback));
    QByteArray request = m_jsonRpc->createRequest(id, method, params);
    m_process->write(request);
    return id;
}

QJsonObject LspClient::makeTextDocumentParam(const QString& uri) const
{
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    return textDoc;
}

QJsonObject LspClient::makePositionParam(const QString& uri, const Position& pos) const
{
    QJsonObject params;
    params["textDocument"] = makeTextDocumentParam(uri);
    params["position"] = pos.toJson();
    return params;
}

void LspClient::requestLocation(const QString& method, const QString& uri, const Position& pos, const char* /*signalName*/)
{
    QJsonObject params = makePositionParam(uri, pos);
    sendRequest(method, params,
                [this, uri, method](const QJsonValue& result, const QJsonObject& /*error*/)
                {
                    Location loc;
                    if (result.isObject() && result.toObject().contains("range"))
                        loc = Location::fromJson(result.toObject());
                    else if (result.isArray() && !result.toArray().isEmpty())
                        loc = Location::fromJson(result.toArray().first().toObject());
                    if (loc.uri.isEmpty())
                        return;
                    if (method == QLatin1String("textDocument/definition"))
                        emit definitionReady(uri, loc);
                    else if (method == QLatin1String("textDocument/typeDefinition"))
                        emit typeDefinitionReady(uri, loc);
                    else if (method == QLatin1String("textDocument/declaration"))
                        emit declarationReady(uri, loc);
                });
}

// ── Internal Slots ─────────────────────────────────────────────

void LspClient::onReadyReadStdout()
{
    if (!m_process)
        return;
    QByteArray data = m_process->readAllStandardOutput();
    m_jsonRpc->handleData(data);
}

void LspClient::onReadyReadStderr()
{
    if (!m_process)
        return;
    QByteArray data = m_process->readAllStandardError();
    if (!data.isEmpty())
    {
        QString msg = QString::fromUtf8(data).trimmed();
        if (!msg.isEmpty())
        {
#ifdef QT_DEBUG
            qDebug().noquote() << QString("[LSP %1] %2").arg(m_language, msg);
#endif
        }
    }
}

void LspClient::onProcessFinished(int /*exitCode*/, QProcess::ExitStatus status)
{
    m_initialized = false;

    // The server cannot answer anything anymore; resolve every outstanding
    // request with an error instead of letting them wait out their timeout.
    m_jsonRpc->failPendingRequests(-32001, QStringLiteral("Language server stopped"));

    if (m_stopping)
    {
        m_stopping = false;
        cleanupProcess();
        return;
    }

    // Guard against double emission if cleanupProcess was already called via error path
    if (!m_process)
        return;

    emit serverStopped();

    if (status == QProcess::CrashExit && m_retryCount < MAX_RETRIES)
    {
        m_retryCount++;
#ifdef QT_DEBUG
        qDebug().noquote()
            << QString("Language server for %1 crashed, restarting (attempt %2/%3)...").arg(m_language).arg(m_retryCount).arg(MAX_RETRIES);
#endif
        QTimer::singleShot(m_retryCount * 1000, this, &LspClient::tryRestart);
    }
}

void LspClient::cleanupProcess()
{
    bool hadProcess = (m_process != nullptr);
    if (m_process)
    {
        m_process->disconnect();
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_starting = false;
    m_stopping = false;
    if (hadProcess)
        emit serverStopped();
}

void LspClient::flushPendingDocOps()
{
    auto ops = std::move(m_pendingDocOps);
    for (const auto& op : ops)
    {
        switch (op.type)
        {
        case PendingDocOp::Open:
            openDocument(op.uri, op.text, op.version);
            break;
        case PendingDocOp::Change:
            changeDocument(op.uri, op.text, op.version);
            break;
        case PendingDocOp::Close:
            closeDocument(op.uri);
            break;
        case PendingDocOp::Save:
            saveDocument(op.uri);
            break;
        }
    }
}

void LspClient::tryRestart()
{
    if (!m_command.isEmpty())
        startServer(m_command, m_args, m_rootUri);
}

void LspClient::onProcessError(QProcess::ProcessError error)
{
    m_starting = false;

    switch (error)
    {
    case QProcess::FailedToStart:
        emit serverError(tr("Language server failed to start for %1").arg(m_language));
        // onProcessFinished will also fire; let cleanup be handled there to avoid double emission
        if (m_process && m_process->state() == QProcess::NotRunning)
            cleanupProcess();
        break;
    case QProcess::Timedout:
        emit serverError(tr("Language server timed out for %1").arg(m_language));
        break;
    default:
        emit serverError(tr("Language server process error for %1").arg(m_language));
        break;
    }
}

// ── Notification/Response Handling ─────────────────────────────

void LspClient::handleNotification(const QString& method, const QJsonObject& params)
{
    if (method == "textDocument/publishDiagnostics")
    {
        QString uri = params["uri"].toString();
        QList<Diagnostic> diagnostics;
        QJsonArray diags = params["diagnostics"].toArray();
        for (const auto& d : diags)
            diagnostics.append(Diagnostic::fromJson(d.toObject()));
        emit diagnosticsReady(uri, diagnostics);
    }
    else if (method == "window/showMessage")
    {
        int type = params["type"].toInt(0);
        QString msg = params["message"].toString();
        QString prefix = "LSP";
        switch (type)
        {
        case 1:
        case 2:
            Logger::instance().warning(QString("[%1] %2").arg(prefix, msg));
            break;
        case 3:
#ifdef QT_DEBUG
            qDebug().noquote() << QString("[%1] %2").arg(prefix, msg);
#endif
            break;
        default:
#ifdef QT_DEBUG
            qDebug().noquote() << QString("[%1] %2").arg(prefix, msg);
#endif
            break;
        }
    }
    else if (method == "window/logMessage")
    {
        QString msg = params["message"].toString();
#ifdef QT_DEBUG
        qDebug().noquote() << QString("[LSP %1] %2").arg(m_language, msg);
#endif
    }
}

void LspClient::handleResponse(const QVariant& /*id*/, const QJsonValue& /*result*/, const QJsonObject& error)
{
    if (!error.isEmpty())
    {
        QString msg = error["message"].toString();
        QString code = QString::number(error["code"].toInt());
        emit serverError(QStringLiteral("[%1] %2").arg(code, msg));
    }
}

void LspClient::handleServerRequest(const QVariant& id, const QString& method, const QJsonObject& params)
{
    Q_UNUSED(params);
    Q_UNUSED(method);
#ifdef QT_DEBUG
    qDebug().noquote() << QString("[LSP %1] server request not supported: %2").arg(m_language, method);
#endif
    // Per JSON-RPC every server -> client request must be answered. Until
    // dedicated handlers exist, reply "Method not found" so servers do not
    // block waiting for a response (e.g. workspace/applyEdit,
    // client/registerCapability, workspace/configuration).
    if (m_process)
        m_process->write(m_jsonRpc->createErrorResponse(id, -32601, QStringLiteral("Method not implemented by Devpad")));
}

} // namespace lsp
