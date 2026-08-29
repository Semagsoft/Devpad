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
}

LspClient::~LspClient()
{
    stopServer();
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

    if (m_process->state() != QProcess::NotRunning)
    {
        QByteArray shutdown = m_jsonRpc->createRequest(m_jsonRpc->nextRequestId(), "shutdown", QJsonObject());
        m_process->write(shutdown);

        QByteArray exit = m_jsonRpc->createNotification("exit", QJsonObject());
        m_process->write(exit);

        QPointer<QProcess> guard(m_process);
        QTimer::singleShot(SHUTDOWN_TIMEOUT_MS, this,
                           [this, guard]()
                           {
                               if (!guard || guard != m_process || !m_stopping)
                                   return;
                               if (m_process->state() != QProcess::NotRunning)
                               {
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
    else
    {
        cleanupProcess();
    }
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
    params["processId"] = static_cast<qint64>(QCoreApplication::applicationPid());
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
    m_jsonRpc->registerPendingRequest(id,
                                      [this](const QJsonValue& result)
                                      {
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

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    params["position"] = pos.toJson();

    QJsonObject context;
    context["triggerKind"] = triggerKind;
    params["context"] = context;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, uri](const QJsonValue& result)
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
                                          {
                                              list.items.append(CompletionItem::fromJson(item.toObject()));
                                          }
                                          emit completionReady(uri, list);
                                      });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/completion", params);
    m_process->write(request);
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
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    params["range"] = range.toJson();

    QJsonObject ctx;
    QJsonArray diags;
    for (const auto& d : diagnostics)
        diags.append(d.toJson());
    ctx["diagnostics"] = diags;
    params["context"] = ctx;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, uri](const QJsonValue& result)
                                      {
                                          QList<QJsonObject> actions;
                                          if (result.isArray())
                                          {
                                              for (const auto& v : result.toArray())
                                                  actions.append(v.toObject());
                                          }
                                          emit codeActionReady(uri, actions);
                                      });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/codeAction", params);
    m_process->write(request);
}

void LspClient::requestWorkspaceSymbols(const QString& query)
{
    if (!isRunning() || !m_initialized || !m_capabilities.workspaceSymbolProvider)
        return;

    QJsonObject params;
    params["query"] = query;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this, query](const QJsonValue& result) { emit workspaceSymbolsReady(query, result.toArray()); });

    QByteArray request = m_jsonRpc->createRequest(id, "workspace/symbol", params);
    m_process->write(request);
}

void LspClient::requestSelectionRanges(const QString& uri, const QList<Position>& positions)
{
    if (!isRunning() || !m_initialized || !m_capabilities.selectionRangeProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    QJsonArray posArr;
    for (const auto& p : positions)
        posArr.append(p.toJson());
    params["positions"] = posArr;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this, uri](const QJsonValue& result) { emit selectionRangesReady(uri, result.toArray()); });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/selectionRange", params);
    m_process->write(request);
}

void LspClient::requestLinkedEditingRange(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.linkedEditingRangeProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    params["position"] = pos.toJson();

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this, uri](const QJsonValue& result) { emit linkedEditingRangeReady(uri, result.toObject()); });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/linkedEditingRange", params);
    m_process->write(request);
}

void LspClient::prepareCallHierarchy(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.callHierarchyProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    params["position"] = pos.toJson();

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this, uri](const QJsonValue& result) { emit callHierarchyReady(uri, result.toArray()); });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/prepareCallHierarchy", params);
    m_process->write(request);
}

void LspClient::incomingCalls(const QString& uri, const QString& itemId)
{
    if (!isRunning() || !m_initialized || !m_capabilities.callHierarchyProvider)
        return;

    QJsonObject params;
    QJsonObject item;
    item["kind"] = 12; // Function
    item["name"] = itemId;
    params["item"] = item;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this, uri](const QJsonValue& result) { emit incomingCallsReady(uri, result.toArray()); });

    QByteArray request = m_jsonRpc->createRequest(id, "callHierarchy/incomingCalls", params);
    m_process->write(request);
}

void LspClient::outgoingCalls(const QString& uri, const QString& itemId)
{
    if (!isRunning() || !m_initialized || !m_capabilities.callHierarchyProvider)
        return;

    QJsonObject params;
    QJsonObject item;
    item["kind"] = 12;
    item["name"] = itemId;
    params["item"] = item;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this, uri](const QJsonValue& result) { emit outgoingCallsReady(uri, result.toArray()); });

    QByteArray request = m_jsonRpc->createRequest(id, "callHierarchy/outgoingCalls", params);
    m_process->write(request);
}

void LspClient::requestSemanticTokensFull(const QString& uri)
{
    if (!isRunning() || !m_initialized || !m_capabilities.semanticTokensProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, uri](const QJsonValue& result)
                                      {
                                          QJsonObject obj = result.toObject();
                                          emit semanticTokensFullReady(uri, obj["data"].toArray());
                                      });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/semanticTokens/full", params);
    m_process->write(request);
}

void LspClient::requestReferences(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.referencesProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    params["position"] = pos.toJson();
    params["context"] = QJsonObject{{"includeDeclaration", true}};

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, uri](const QJsonValue& result)
                                      {
                                          QList<Location> locations;
                                          if (result.isArray())
                                          {
                                              for (const auto& v : result.toArray())
                                                  locations.append(Location::fromJson(v.toObject()));
                                          }
                                          emit referencesReady(uri, locations);
                                      });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/references", params);
    m_process->write(request);
}

void LspClient::requestHover(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.hoverProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    params["position"] = pos.toJson();

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, uri](const QJsonValue& result)
                                      {
                                          QString contents;
                                          QJsonObject obj = result.toObject();
                                          QJsonValue val = obj["contents"];
                                          if (val.isString())
                                          {
                                              contents = val.toString();
                                          }
                                          else if (val.isObject())
                                          {
                                              contents = val.toObject()["value"].toString();
                                          }
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

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/hover", params);
    m_process->write(request);
}

void LspClient::requestDocumentSymbols(const QString& uri)
{
    if (!isRunning() || !m_initialized || !m_capabilities.documentSymbolProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, uri](const QJsonValue& result)
                                      {
                                          QJsonArray symbols;
                                          if (result.isArray())
                                              symbols = result.toArray();
                                          else if (result.isObject())
                                          {
                                              QJsonObject obj = result.toObject();
                                              if (obj.contains("symbols"))
                                                  symbols = obj["symbols"].toArray();
                                          }
                                          emit documentSymbolsReady(uri, symbols);
                                      });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/documentSymbol", params);
    m_process->write(request);
}

void LspClient::requestFormatting(const QString& uri, int tabSize, bool insertSpaces)
{
    if (!isRunning() || !m_initialized || !m_capabilities.formattingProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    QJsonObject options;
    options["tabSize"] = tabSize;
    options["insertSpaces"] = insertSpaces;
    params["options"] = options;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, uri](const QJsonValue& result)
                                      {
                                          QList<QJsonObject> edits;
                                          if (result.isArray())
                                          {
                                              for (const auto& v : result.toArray())
                                                  edits.append(v.toObject());
                                          }
                                          emit formattingReady(uri, edits);
                                      });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/formatting", params);
    m_process->write(request);
}

void LspClient::requestRangeFormatting(const QString& uri, const Range& range, int tabSize, bool insertSpaces)
{
    if (!isRunning() || !m_initialized || !m_capabilities.formattingProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    params["range"] = range.toJson();
    QJsonObject options;
    options["tabSize"] = tabSize;
    options["insertSpaces"] = insertSpaces;
    params["options"] = options;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, uri](const QJsonValue& result)
                                      {
                                          QList<QJsonObject> edits;
                                          if (result.isArray())
                                          {
                                              for (const auto& v : result.toArray())
                                                  edits.append(v.toObject());
                                          }
                                          emit rangeFormattingReady(uri, edits);
                                      });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/rangeFormatting", params);
    m_process->write(request);
}

void LspClient::requestSignatureHelp(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.signatureHelpProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    params["position"] = pos.toJson();

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this, uri](const QJsonValue& result) { emit signatureHelpReady(uri, result.toObject()); });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/signatureHelp", params);
    m_process->write(request);
}

void LspClient::requestDiagnostics(const QString& uri)
{
    if (!isRunning() || !m_initialized)
        return;

    // For servers that support the pull model (3.17+), request diagnostics
    if (m_capabilities.diagnosticProvider)
    {
        QJsonObject params;
        QJsonObject textDoc;
        textDoc["uri"] = uri;
        params["textDocument"] = textDoc;
        params["identifier"] = QStringLiteral("default");

        int id = m_jsonRpc->nextRequestId();
        m_jsonRpc->registerPendingRequest(id,
                                          [this, uri](const QJsonValue& result)
                                          {
                                              QList<Diagnostic> diagnostics;
                                              QJsonArray items;
                                              if (result.isObject())
                                              {
                                                  QJsonObject obj = result.toObject();
                                                  if (obj.contains("items"))
                                                      items = obj["items"].toArray();
                                              }
                                              else if (result.isArray())
                                              {
                                                  items = result.toArray();
                                              }
                                              for (const auto& v : items)
                                                  diagnostics.append(Diagnostic::fromJson(v.toObject()));
                                              emit diagnosticsReady(uri, diagnostics);
                                          });

        QByteArray request = m_jsonRpc->createRequest(id, "textDocument/diagnostic", params);
        m_process->write(request);
    }
}

void LspClient::requestRename(const QString& uri, const Position& pos, const QString& newName)
{
    if (!isRunning() || !m_initialized || !m_capabilities.renameProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    params["position"] = pos.toJson();
    params["newName"] = newName;

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this, uri](const QJsonValue& result) { emit renameReady(uri, result.toObject()); });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/rename", params);
    m_process->write(request);
}

void LspClient::requestDocumentHighlight(const QString& uri, const Position& pos)
{
    if (!isRunning() || !m_initialized || !m_capabilities.documentHighlightProvider)
        return;

    QJsonObject params;
    QJsonObject textDoc;
    textDoc["uri"] = uri;
    params["textDocument"] = textDoc;
    params["position"] = pos.toJson();

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this, uri](const QJsonValue& result) { emit documentHighlightReady(uri, result.toArray()); });

    QByteArray request = m_jsonRpc->createRequest(id, "textDocument/documentHighlight", params);
    m_process->write(request);
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
                [this, uri, method](const QJsonValue& result)
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

    if (m_stopping)
    {
        m_stopping = false;
        cleanupProcess();
        return;
    }

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
    if (m_process)
    {
        m_process->disconnect();
        m_process->deleteLater();
        m_process = nullptr;
    }
    m_starting = false;
    m_stopping = false;
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

void LspClient::handleResponse(int /*id*/, const QJsonValue& /*result*/, const QJsonObject& error)
{
    if (!error.isEmpty())
    {
        QString msg = error["message"].toString();
        QString code = QString::number(error["code"].toInt());
        emit serverError(QStringLiteral("[%1] %2").arg(code, msg));
    }
}

} // namespace lsp
