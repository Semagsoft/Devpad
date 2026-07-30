#include "lspconnectionhelper.h"

#include "actionmanager.h"
#include "codeeditor.h"
#include "errorlistpanel.h"
#include "settingsmanager.h"
#include "tabmanager.h"

#include "lspclient.h"
#include "lspservermanager.h"
#include "lsptypes.h"

#include <QCursor>
#include <QJsonObject>
#include <QMenu>
#include <QToolButton>

namespace lsp
{

void setupLspConnections(LspServerManager* manager, TabManager* tabManager, ErrorListPanel* errorPanel, ActionManager* actionManager,
                         std::function<void(const QString& filePath, int line, int column)> navigateToLocation)
{
    // Definition
    QObject::connect(manager, &LspServerManager::definitionReady, manager,
                     [navigateToLocation](const QString&, const Location& location)
                     {
                         QString filePath = pathFromUri(location.uri);
                         navigateToLocation(filePath, location.range.start.line, location.range.start.character);
                     });

    // Completion
    QObject::connect(manager, &LspServerManager::completionReady, manager,
                     [tabManager](const QString& uri, const CompletionList& completions)
                     {
                         Q_UNUSED(uri)
                         CodeEditor* editor = tabManager->currentEditor();
                         if (editor)
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
                     [tabManager](const QString& uri, const QList<QJsonObject>& actions)
                     {
                         if (actions.isEmpty())
                             return;
                         CodeEditor* editor = tabManager->findEditorByFileName(pathFromUri(uri));
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
                     [tabManager](const QString& uri, const QString& contents)
                     {
                         Q_UNUSED(uri)
                         CodeEditor* editor = tabManager->currentEditor();
                         if (editor && !contents.isEmpty())
                         {
                             int pos = editor->cursorPosition();
                             editor->showToolTip(pos, contents);
                         }
                     });

    // Selection Ranges
    QObject::connect(manager, &LspServerManager::selectionRangesReady, manager,
                     [tabManager](const QString& uri, const QJsonArray& ranges)
                     {
                         Q_UNUSED(uri)
                         CodeEditor* editor = tabManager->currentEditor();
                         if (editor)
                             editor->setSelectionRanges(ranges);
                     });

    // Semantic Tokens
    QObject::connect(manager, &LspServerManager::semanticTokensFullReady, manager,
                     [tabManager](const QString& uri, const QJsonArray& tokens)
                     {
                         CodeEditor* editor = tabManager->findEditorByFileName(pathFromUri(uri));
                         if (editor)
                             editor->applySemanticTokens(uri, tokens);
                     });

    // Linked Editing Range
    QObject::connect(manager, &LspServerManager::linkedEditingRangeReady, manager,
                     [tabManager](const QString& uri, const QJsonObject& result)
                     {
                         CodeEditor* editor = tabManager->findEditorByFileName(pathFromUri(uri));
                         if (editor)
                             editor->setLinkedEditingRanges(result);
                     });

    // Formatting
    QObject::connect(manager, &LspServerManager::formattingReady, manager,
                     [tabManager](const QString& uri, const QList<QJsonObject>& edits)
                     {
                         CodeEditor* editor = tabManager->findEditorByFileName(pathFromUri(uri));
                         if (editor)
                             editor->applyFormattingEdits(edits);
                     });

    // Range Formatting
    QObject::connect(manager, &LspServerManager::rangeFormattingReady, manager,
                     [tabManager](const QString& uri, const QList<QJsonObject>& edits)
                     {
                         CodeEditor* editor = tabManager->findEditorByFileName(pathFromUri(uri));
                         if (editor)
                             editor->applyFormattingEdits(edits);
                     });

    // Signature Help
    QObject::connect(manager, &LspServerManager::signatureHelpReady, manager,
                     [tabManager](const QString& uri, const QJsonObject& info)
                     {
                         Q_UNUSED(uri)
                         CodeEditor* editor = tabManager->currentEditor();
                         if (editor)
                             editor->showSignatureHelp(info);
                     });

    // Document Highlight
    QObject::connect(manager, &LspServerManager::documentHighlightReady, manager,
                     [tabManager](const QString& uri, const QJsonArray& highlights)
                     {
                         CodeEditor* editor = tabManager->findEditorByFileName(pathFromUri(uri));
                         if (editor)
                             editor->applyHighlights(highlights);
                     });

    // Diagnostics
    QObject::connect(manager, &LspServerManager::diagnosticsReady, manager,
                     [tabManager, errorPanel](const QString& uri, const QList<Diagnostic>& diagnostics)
                     {
                         CodeEditor* editor = tabManager->findEditorByFileName(pathFromUri(uri));
                         if (editor)
                             editor->applyDiagnostics(uri, diagnostics);
                         errorPanel->updateDiagnostics(uri, diagnostics);
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
