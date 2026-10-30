#include "lspconnectionhelper.h"

#include "actionmanager.h"
#include "codeeditor.h"
#include "errorlistpanel.h"
#include "lspclient.h"
#include "lspservermanager.h"
#include "lsptypes.h"
#include "settingsmanager.h"
#include "tabmanager.h"

#include <QCursor>
#include <QJsonObject>
#include <QMenu>
#include <QPointer>
#include <QToolButton>

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
} // namespace

void setupLspConnections(LspServerManager* manager, TabManager* tabManager, ErrorListPanel* errorPanel, ActionManager* actionManager,
                         std::function<void(const QString& filePath, int line, int column)> navigateToLocation)
{
    QPointer<TabManager> tabGuard(tabManager);
    QPointer<ErrorListPanel> panelGuard(errorPanel);

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
                     [resolveEditor](const QString& uri, const QList<QJsonObject>& actions)
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
                             QJsonObject cmd = a["command"].toObject();
                             QString command = cmd["command"].toString();
                             act->setData(command);
                         }
                         QAction* chosen = menu.exec(QCursor::pos());
                         if (chosen && !chosen->data().toString().isEmpty())
                         {
#ifdef QT_DEBUG
                             qDebug() << "Code action:" << chosen->data().toString();
#endif
                         }
                     });

    // References
    QObject::connect(manager, &LspServerManager::referencesReady, manager,
                     [navigateToLocation](const QString&, const QList<Location>& locations)
                     {
                         if (locations.isEmpty())
                             return;
                         const auto& loc = locations.first();
                         QString filePath = pathFromUri(loc.uri);
                         navigateToLocation(filePath, loc.range.start.line, loc.range.start.character);
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
