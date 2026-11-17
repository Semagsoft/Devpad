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

#include "lspconnectionhelper.h"

#include "logger.h"
#include "lspclient.h"
#include "lspservermanager.h"
#include "lspworkspaceedit.h"
#include "plugins/lspinterface.h"

#include <QApplication>
#include <QCursor>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QPointer>

namespace lsp
{
namespace
{
IEditorLspBridge* bridgeForUri(const QList<QPointer<IEditorLspBridge>>* bridges, const QString& uri)
{
    if (!bridges)
        return nullptr;
    const QString filePath = pathFromUri(uri);
    if (filePath.isEmpty())
        return nullptr;
    for (const auto& guard : *bridges)
    {
        IEditorLspBridge* bridge = guard.data();
        if (bridge && bridge->fileName() == filePath)
            return bridge;
    }
    return nullptr;
}

// Apply a LSP WorkspaceEdit to the matching open editors, or to disk when the
// file isn't open. Files that are not open in an editor are edited on disk
// (UTF-8, atomic write). LSP resource operations (create/rename) are
// handled too; delete is refused and logged.
void applyWorkspaceEdit(const QJsonObject& workspaceEdit, const QList<QPointer<IEditorLspBridge>>* bridges)
{
    QHash<QString, QList<QJsonObject>> editsByUri;

    auto collectEdits = [&](const QString& uri, const QJsonArray& edits)
    {
        for (const auto& v : edits)
            editsByUri[uri].append(v.toObject());
    };

    if (workspaceEdit.contains(QStringLiteral("documentChanges")))
    {
        for (const auto& v : workspaceEdit[QStringLiteral("documentChanges")].toArray())
        {
            const QJsonObject dc = v.toObject();
            const QString kind = dc[QStringLiteral("kind")].toString();
            if (!kind.isEmpty())
            {
                // Resource operation (CreateFile/RenameFile/DeleteFile).
                const QString uri = dc[QStringLiteral("uri")].toString();
                const QString filePath = pathFromUri(uri);
                if (kind == QStringLiteral("create"))
                {
                    if (!filePath.isEmpty() && !QFile::exists(filePath))
                    {
                        QFile createFile(filePath);
                        if (!createFile.open(QIODevice::WriteOnly))
                            Logger::instance().warning(QString("Workspace edit: cannot create file %1").arg(filePath));
                        createFile.close();
                    }
                }
                else if (kind == QStringLiteral("rename"))
                {
                    const QString newPath = pathFromUri(dc[QStringLiteral("newUri")].toString());
                    if (!filePath.isEmpty() && !newPath.isEmpty() && filePath != newPath)
                    {
                        if (!QFile::rename(filePath, newPath))
                            Logger::instance().warning(QString("Workspace edit: cannot rename %1 to %2").arg(filePath, newPath));
                    }
                }
                else if (kind == QStringLiteral("delete"))
                {
                    // Never delete files automatically as a side effect of a
                    // server request; the delete needs an explicit UI confirm.
                    Logger::instance().warning(QString("Workspace edit: ignoring delete of %1 (requires confirmation)").arg(filePath));
                }
                continue;
            }
            const QString uri = dc[QStringLiteral("textDocument")].toObject()[QStringLiteral("uri")].toString();
            collectEdits(uri, dc[QStringLiteral("edits")].toArray());
        }
    }
    else if (workspaceEdit.contains(QStringLiteral("changes")))
    {
        const QJsonObject changesMap = workspaceEdit[QStringLiteral("changes")].toObject();
        for (const QString& uri : changesMap.keys())
            collectEdits(uri, changesMap[uri].toArray());
    }

    for (auto it = editsByUri.begin(); it != editsByUri.end(); ++it)
    {
        const QString filePath = pathFromUri(it.key());
        const QList<QJsonObject> edits = it.value();

        if (IEditorLspBridge* bridge = bridgeForUri(bridges, it.key()))
        {
            bridge->applyTextEdits(edits);
            continue;
        }

        // File not open in any editor: edit the file on disk.
        QString error;
        if (!applyTextEditsToFile(filePath, edits, &error))
            Logger::instance().warning(QString("Workspace edit: %1: %2").arg(filePath, error));
    }
}
} // namespace

void setupLspConnections(LspServerManager* manager, const QList<QPointer<IEditorLspBridge>>* bridges, ILspService* service,
                         std::function<void(const QString& filePath, int line, int column)> navigateToLocation)
{
    QPointer<LspServerManager> managerGuard(manager);

    auto resolveBridge = [bridges](const QString& uri) -> IEditorLspBridge* { return bridgeForUri(bridges, uri); };

    // Definition
    QObject::connect(manager, &LspServerManager::definitionReady, manager,
                     [navigateToLocation](const QString&, const Location& location)
                     {
                         const QString filePath = pathFromUri(location.uri);
                         navigateToLocation(filePath, location.range.start.line, location.range.start.character);
                     });

    // Completion
    QObject::connect(manager, &LspServerManager::completionReady, manager,
                     [resolveBridge](const QString& uri, const CompletionList& completions)
                     {
                         if (IEditorLspBridge* bridge = resolveBridge(uri))
                             bridge->showCompletion(completions);
                     });

    // Type Definition
    QObject::connect(manager, &LspServerManager::typeDefinitionReady, manager,
                     [navigateToLocation](const QString&, const Location& location)
                     {
                         const QString filePath = pathFromUri(location.uri);
                         navigateToLocation(filePath, location.range.start.line, location.range.start.character);
                     });

    // Declaration
    QObject::connect(manager, &LspServerManager::declarationReady, manager,
                     [navigateToLocation](const QString&, const Location& location)
                     {
                         const QString filePath = pathFromUri(location.uri);
                         navigateToLocation(filePath, location.range.start.line, location.range.start.character);
                     });

    // Code Action
    QObject::connect(manager, &LspServerManager::codeActionReady, manager,
                     [managerGuard, resolveBridge, bridges](const QString& uri, const QList<QJsonObject>& actions)
                     {
                         if (actions.isEmpty())
                             return;
                         IEditorLspBridge* bridge = resolveBridge(uri);
                         if (!bridge)
                             return;

                         QMenu menu(QApplication::activeWindow());
                         for (const auto& a : actions)
                         {
                             const QString title = a[QStringLiteral("title")].toString();
                             QAction* act = menu.addAction(title);
                             // Store the full action object so we can apply its
                             // edit and/or execute its command when chosen.
                             act->setData(QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact)));
                         }
                         QAction* chosen = menu.exec(QCursor::pos());
                         if (!chosen)
                             return;

                         const QJsonDocument doc = QJsonDocument::fromJson(chosen->data().toString().toUtf8());
                         const QJsonObject action = doc.object();

                         // Apply the workspace edit (e.g. rename, quick-fix edits)
                         // to any matching open editors or to disk.
                         const QJsonObject edit = action[QStringLiteral("edit")].toObject();
                         if (!edit.isEmpty())
                             applyWorkspaceEdit(edit, bridges);

                         // Execute the command (e.g. a server-provided refactoring).
                         const QJsonObject cmd = action[QStringLiteral("command")].toObject();
                         const QString command = cmd[QStringLiteral("command")].toString();
                         if (!command.isEmpty() && managerGuard)
                         {
                             const QJsonArray args = cmd[QStringLiteral("arguments")].toArray();
                             if (auto* client = managerGuard->clientForUri(uri))
                                 client->requestExecuteCommand(command, args);
                         }
                     });

    // References
    QObject::connect(manager, &LspServerManager::referencesReady, manager,
                     [service](const QString& uri, const QList<Location>& locations)
                     {
                         if (service)
                             Q_EMIT service->referencesReady(uri, locations);
                     });

    // Rename: apply the returned workspace edit to open editors or to disk.
    QObject::connect(manager, &LspServerManager::renameReady, manager,
                     [bridges](const QString&, const QJsonObject& result) { applyWorkspaceEdit(result, bridges); });

    // Hover. Tooltips are transient: only show them in the editor the user
    // is currently looking at, at that editor's cursor.
    QObject::connect(manager, &LspServerManager::hoverReady, manager,
                     [resolveBridge](const QString& uri, const QString& contents)
                     {
                         IEditorLspBridge* bridge = resolveBridge(uri);
                         if (bridge && !contents.isEmpty() && bridge->isCurrentEditor())
                             bridge->showToolTip(bridge->cursorPosition(), contents);
                     });

    // Selection Ranges
    QObject::connect(manager, &LspServerManager::selectionRangesReady, manager,
                     [resolveBridge](const QString& uri, const QJsonArray& ranges)
                     {
                         if (IEditorLspBridge* bridge = resolveBridge(uri))
                         {
                             if (ILspEditorSession* session = bridge->session())
                                 session->handleSelectionRanges(ranges);
                         }
                     });

    // Semantic Tokens
    QObject::connect(manager, &LspServerManager::semanticTokensFullReady, manager,
                     [resolveBridge](const QString& uri, const QJsonArray& tokens)
                     {
                         if (IEditorLspBridge* bridge = resolveBridge(uri))
                             bridge->applySemanticTokens(uri, tokens);
                     });

    // Linked Editing Range
    QObject::connect(manager, &LspServerManager::linkedEditingRangeReady, manager,
                     [resolveBridge](const QString& uri, const QJsonObject& result)
                     {
                         if (IEditorLspBridge* bridge = resolveBridge(uri))
                             bridge->setLinkedEditingRanges(result);
                     });

    // Formatting
    QObject::connect(manager, &LspServerManager::formattingReady, manager,
                     [resolveBridge](const QString& uri, const QList<QJsonObject>& edits)
                     {
                         if (IEditorLspBridge* bridge = resolveBridge(uri))
                             bridge->applyFormattingEdits(edits);
                     });

    // Range Formatting
    QObject::connect(manager, &LspServerManager::rangeFormattingReady, manager,
                     [resolveBridge](const QString& uri, const QList<QJsonObject>& edits)
                     {
                         if (IEditorLspBridge* bridge = resolveBridge(uri))
                             bridge->applyFormattingEdits(edits);
                     });

    // Signature Help
    QObject::connect(manager, &LspServerManager::signatureHelpReady, manager,
                     [resolveBridge](const QString& uri, const QJsonObject& info)
                     {
                         if (IEditorLspBridge* bridge = resolveBridge(uri))
                             bridge->showSignatureHelp(info);
                     });

    // Document Highlight
    QObject::connect(manager, &LspServerManager::documentHighlightReady, manager,
                     [resolveBridge](const QString& uri, const QJsonArray& highlights)
                     {
                         if (IEditorLspBridge* bridge = resolveBridge(uri))
                             bridge->applyHighlights(highlights);
                     });

    // Diagnostics. The editor emits its own diagnosticsChanged signal; the
    // host connects that to the error list panel, so applying via the
    // bridge is enough.
    QObject::connect(manager, &LspServerManager::diagnosticsReady, manager,
                     [resolveBridge](const QString& uri, const QList<Diagnostic>& diagnostics)
                     {
                         if (IEditorLspBridge* bridge = resolveBridge(uri))
                             bridge->applyDiagnostics(uri, diagnostics);
                     });
}

} // namespace lsp
