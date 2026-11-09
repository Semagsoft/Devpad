#include "lspconnectionhelper.h"

#include "actionmanager.h"
#include "codeeditor.h"
#include "errorlistpanel.h"
#include "lspclient.h"
#include "lspservermanager.h"
#include "lsptypes.h"
#include "referencespanel.h"
#include "settingsmanager.h"
#include "tabmanager.h"

#include <QCursor>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QPointer>
#include <QToolButton>
#include <algorithm>

namespace lsp
{
namespace
{
// Resolve the editor that owns a notification/response URI. Several LSP
// responses arrive asynchronously; applying them to whichever editor is
// "current" at arrival time would put results into the wrong document.
CodeEditor* editorForUri(TabManager* tabManager, const QString& uri)
{
    return tabManager->findEditorByFileName(pathFromUri(uri));
}

// Apply a LSP WorkspaceEdit to the matching open editors. Edits within a
// single document are applied in reverse order so that positions remain valid.
void applyWorkspaceEdit(const QJsonObject& workspaceEdit, TabManager* tabManager)
{
    QHash<QString, QList<QJsonObject>> editsByUri;

    auto collectEdits = [&](const QString& uri, const QJsonArray& edits)
    {
        for (const auto& v : edits)
            editsByUri[uri].append(v.toObject());
    };

    if (workspaceEdit.contains("documentChanges"))
    {
        for (const auto& v : workspaceEdit["documentChanges"].toArray())
        {
            QJsonObject dc = v.toObject();
            QString uri = dc["textDocument"].toObject()["uri"].toString();
            collectEdits(uri, dc["edits"].toArray());
        }
    }
    else if (workspaceEdit.contains("changes"))
    {
        QJsonObject changesMap = workspaceEdit["changes"].toObject();
        for (const QString& uri : changesMap.keys())
            collectEdits(uri, changesMap[uri].toArray());
    }

    auto rangeKey = [](const QJsonObject& edit)
    {
        auto range = lsp::Range::fromJson(edit["range"].toObject());
        return qMakePair(range.start.line, range.start.character);
    };

    for (auto it = editsByUri.begin(); it != editsByUri.end(); ++it)
    {
        QString filePath = lsp::pathFromUri(it.key());
        CodeEditor* editor = tabManager ? tabManager->findEditorByFileName(filePath) : nullptr;
        if (!editor)
            continue;

        QList<QJsonObject> edits = it.value();
        // Descending position order: applying the last (bottom-most) edit first
        // keeps the ranges of earlier edits valid.
        std::sort(edits.begin(), edits.end(), [&](const QJsonObject& a, const QJsonObject& b) { return rangeKey(a) > rangeKey(b); });

        editor->applyTextEdits(edits);
    }
}
} // namespace

void setupLspConnections(LspServerManager* manager, TabManager* tabManager, ErrorListPanel* errorPanel, ReferencesPanel* referencesPanel,
                         ActionManager* actionManager, std::function<void(const QString& filePath, int line, int column)> navigateToLocation)
{
    QPointer<TabManager> tabGuard(tabManager);
    QPointer<ErrorListPanel> panelGuard(errorPanel);
    QPointer<ReferencesPanel> referencesGuard(referencesPanel);

    auto resolveEditor = [tabGuard](const QString& uri) -> CodeEditor* { return tabGuard ? editorForUri(tabGuard.data(), uri) : nullptr; };

    // Definition
    QObject::connect(manager, &LspServerManager::definitionReady, manager,
                     [navigateToLocation](const QString&, const Location& location)
                     {
                         QString filePath = pathFromUri(location.uri);
                         navigateToLocation(filePath, location.range.start.line, location.range.start.character);
                     });

    // Completion
    QObject::connect(manager, &LspServerManager::completionReady, manager,
                     [resolveEditor](const QString& uri, const CompletionList& completions)
                     {
                         if (CodeEditor* editor = resolveEditor(uri))
                             editor->showCompletion(completions);
                     });

    // Type Definition
    QObject::connect(manager, &LspServerManager::typeDefinitionReady, manager,
                     [navigateToLocation](const QString&, const Location& location)
                     {
                         QString filePath = pathFromUri(location.uri);
                         navigateToLocation(filePath, location.range.start.line, location.range.start.character);
                     });

    // Declaration
    QObject::connect(manager, &LspServerManager::declarationReady, manager,
                     [navigateToLocation](const QString&, const Location& location)
                     {
                         QString filePath = pathFromUri(location.uri);
                         navigateToLocation(filePath, location.range.start.line, location.range.start.character);
                     });

    // Code Action
    QObject::connect(manager, &LspServerManager::codeActionReady, manager,
                     [manager, resolveEditor, tabGuard](const QString& uri, const QList<QJsonObject>& actions)
                     {
                         if (actions.isEmpty())
                             return;
                         CodeEditor* editor = resolveEditor(uri);
                         if (!editor)
                             return;

                         QMenu menu(editor);
                         for (const auto& a : actions)
                         {
                             QString title = a["title"].toString();
                             QAction* act = menu.addAction(title);
                             // Store the full action object so we can apply its
                             // edit and/or execute its command when chosen.
                             act->setData(QString(QString::fromUtf8(QJsonDocument(a).toJson(QJsonDocument::Compact))));
                         }
                         QAction* chosen = menu.exec(QCursor::pos());
                         if (!chosen)
                             return;

                         QJsonDocument doc = QJsonDocument::fromJson(chosen->data().toString().toUtf8());
                         QJsonObject action = doc.object();

                         // Apply the workspace edit (e.g. rename, quick-fix edits)
                         // to any matching open editors.
                         QJsonObject edit = action["edit"].toObject();
                         if (!edit.isEmpty() && tabGuard)
                             applyWorkspaceEdit(edit, tabGuard.data());

                         // Execute the command (e.g. a server-provided refactoring).
                         QJsonObject cmd = action["command"].toObject();
                         QString command = cmd["command"].toString();
                         if (!command.isEmpty() && manager)
                         {
                             QJsonArray args = cmd["arguments"].toArray();
                             if (auto* client = manager->clientForUri(uri))
                                 client->requestExecuteCommand(command, args);
                         }
                     });

    // References
    QObject::connect(manager, &LspServerManager::referencesReady, manager,
                     [referencesGuard](const QString&, const QList<Location>& locations)
                     {
                         if (referencesGuard)
                             referencesGuard->setReferences(locations);
                     });

    // Rename: apply the returned workspace edit to open editors.
    QObject::connect(manager, &LspServerManager::renameReady, manager,
                     [tabGuard](const QString&, const QJsonObject& result)
                     {
                         if (tabGuard)
                             applyWorkspaceEdit(result, tabGuard.data());
                     });

    // Hover
    QObject::connect(manager, &LspServerManager::hoverReady, manager,
                     [resolveEditor, tabGuard](const QString& uri, const QString& contents)
                     {
                         if (CodeEditor* editor = resolveEditor(uri); editor && !contents.isEmpty())
                         {
                             // Tooltips are transient: only show them in the editor the
                             // user is currently looking at, at that editor's cursor.
                             if (!tabGuard || editor != tabGuard->currentEditor())
                                 return;
                             int pos = editor->cursorPosition();
                             editor->showToolTip(pos, contents);
                         }
                     });

    // Selection Ranges
    QObject::connect(manager, &LspServerManager::selectionRangesReady, manager,
                     [resolveEditor](const QString& uri, const QJsonArray& ranges)
                     {
                         if (CodeEditor* editor = resolveEditor(uri))
                             editor->setSelectionRanges(ranges);
                     });

    // Semantic Tokens
    QObject::connect(manager, &LspServerManager::semanticTokensFullReady, manager,
                     [resolveEditor](const QString& uri, const QJsonArray& tokens)
                     {
                         if (CodeEditor* editor = resolveEditor(uri))
                             editor->applySemanticTokens(uri, tokens);
                     });

    // Linked Editing Range
    QObject::connect(manager, &LspServerManager::linkedEditingRangeReady, manager,
                     [resolveEditor](const QString& uri, const QJsonObject& result)
                     {
                         if (CodeEditor* editor = resolveEditor(uri))
                             editor->setLinkedEditingRanges(result);
                     });

    // Formatting
    QObject::connect(manager, &LspServerManager::formattingReady, manager,
                     [resolveEditor](const QString& uri, const QList<QJsonObject>& edits)
                     {
                         if (CodeEditor* editor = resolveEditor(uri))
                             editor->applyFormattingEdits(edits);
                     });

    // Range Formatting
    QObject::connect(manager, &LspServerManager::rangeFormattingReady, manager,
                     [resolveEditor](const QString& uri, const QList<QJsonObject>& edits)
                     {
                         if (CodeEditor* editor = resolveEditor(uri))
                             editor->applyFormattingEdits(edits);
                     });

    // Signature Help
    QObject::connect(manager, &LspServerManager::signatureHelpReady, manager,
                     [resolveEditor](const QString& uri, const QJsonObject& info)
                     {
                         if (CodeEditor* editor = resolveEditor(uri))
                             editor->showSignatureHelp(info);
                     });

    // Document Highlight
    QObject::connect(manager, &LspServerManager::documentHighlightReady, manager,
                     [resolveEditor](const QString& uri, const QJsonArray& highlights)
                     {
                         if (CodeEditor* editor = resolveEditor(uri))
                             editor->applyHighlights(highlights);
                     });

    // Diagnostics
    QObject::connect(manager, &LspServerManager::diagnosticsReady, manager,
                     [resolveEditor, panelGuard](const QString& uri, const QList<Diagnostic>& diagnostics)
                     {
                         if (CodeEditor* editor = resolveEditor(uri))
                             editor->applyDiagnostics(uri, diagnostics);
                         if (panelGuard)
                             panelGuard->updateDiagnostics(uri, diagnostics);
                     });

    // Error panel navigation
    QObject::connect(errorPanel, &ErrorListPanel::navigateToLocation, errorPanel, navigateToLocation);

    // Error panel visibility sync
    if (actionManager->errorListPanelAct())
        QObject::connect(errorPanel, &QDockWidget::visibilityChanged, actionManager->errorListPanelAct(), &QAction::setChecked);
    QObject::connect(errorPanel, &QDockWidget::visibilityChanged, actionManager->errorListPanelButton(), &QToolButton::setChecked);

    // Re-send configuration on settings change
    QObject::connect(&SettingsManager::instance(), &SettingsManager::settingsChanged, manager,
                     [manager]()
                     {
                         for (const QString& lang : manager->languages())
                         {
                             auto* client = manager->clientForLanguage(lang);
                             if (client)
                                 client->sendDidChangeConfiguration(QJsonObject());
                         }
                     });
}

} // namespace lsp
