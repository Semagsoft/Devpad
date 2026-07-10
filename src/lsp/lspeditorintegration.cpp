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
#include "lspeditorintegration.h"
#include "lspclient.h"
#include "lspservermanager.h"
#include "lsptypes.h"
#include "lspindicators.h"

#include "../codeeditor.h"
#include "settingsmanager.h"

#include <QFileInfo>
#include <QInputDialog>

namespace lsp {

LspEditorIntegration::LspEditorIntegration(CodeEditor* editor, QObject* parent)
    : QObject(parent), m_editor(editor)
{
    m_completionTimer = new QTimer(this);
    m_completionTimer->setSingleShot(true);
    m_completionTimer->setInterval(200);
    connect(m_completionTimer, &QTimer::timeout, this, [this]() {
        if (m_lspActive && m_lspManager)
            triggerCompletion();
    });

    m_diagnosticsTimer = new QTimer(this);
    m_diagnosticsTimer->setSingleShot(true);
    m_diagnosticsTimer->setInterval(500);
    connect(m_diagnosticsTimer, &QTimer::timeout, this, [this]() {
        if (m_lspActive)
            sendDidChange();
    });

    m_highlightTimer = new QTimer(this);
    m_highlightTimer->setSingleShot(true);
    m_highlightTimer->setInterval(300);
    connect(m_highlightTimer, &QTimer::timeout, this, [this]() {
        requestDocumentHighlight();
    });
}

LspEditorIntegration::~LspEditorIntegration() = default;

void LspEditorIntegration::setServerManager(LspServerManager* manager)
{
    m_lspManager = manager;
}

void LspEditorIntegration::setLanguage(const QString& language)
{
    m_lspLanguage = language;
    m_lspActive = !language.isEmpty();
    m_lspCompletionActive = false;
    m_docVersion = 0;
}

void LspEditorIntegration::sendDidOpen()
{
    if (!m_lspActive || !m_lspManager) return;

    QString filePath = m_editor->fileName();
    if (filePath.isEmpty() || filePath == m_editor->tr("Untitled")) return;

    QString uri = uriFromPath(filePath);
    QString text = m_editor->text();
    m_lspManager->openDocument(m_lspLanguage, uri, text);
}

void LspEditorIntegration::sendDidChange()
{
    if (!m_lspActive || !m_lspManager) return;

    QString filePath = m_editor->fileName();
    if (filePath.isEmpty() || filePath == m_editor->tr("Untitled")) return;

    QString uri = uriFromPath(filePath);
    QString text = m_editor->text();
    m_lspManager->changeDocument(uri, text);
}

void LspEditorIntegration::goToDefinition()
{
    if (!m_lspActive || !m_lspManager) return;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return;
    QString uri = uriFromPath(filePath);
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().definitionProvider) return;
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    client->requestDefinition(uri, {line, col});
}

void LspEditorIntegration::goToTypeDefinition()
{
    if (!m_lspActive || !m_lspManager) return;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return;
    QString uri = uriFromPath(filePath);
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().typeDefinitionProvider) return;
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    client->requestTypeDefinition(uri, {line, col});
}

void LspEditorIntegration::goToDeclaration()
{
    if (!m_lspActive || !m_lspManager) return;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return;
    QString uri = uriFromPath(filePath);
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().declarationProvider) return;
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    client->requestDeclaration(uri, {line, col});
}

void LspEditorIntegration::formatSelection()
{
    if (!m_lspActive || !m_lspManager) return;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return;
    QString uri = uriFromPath(filePath);
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().formattingProvider) return;
    int lineFrom, colFrom, lineTo, colTo;
    m_editor->getSelection(&lineFrom, &colFrom, &lineTo, &colTo);
    Range range{{lineFrom, colFrom}, {lineTo, colTo}};
    int tabSize = SettingsManager::instance().tabWidth();
    bool insertSpaces = true;
    client->requestRangeFormatting(uri, range, tabSize, insertSpaces);
}

void LspEditorIntegration::findReferences()
{
    if (!m_lspActive || !m_lspManager) return;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return;
    QString uri = uriFromPath(filePath);
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().referencesProvider) return;
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    client->requestReferences(uri, {line, col});
}

void LspEditorIntegration::triggerCompletion()
{
    if (!m_lspActive || !m_lspManager) return;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return;
    QString uri = uriFromPath(filePath);
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().completionProvider) return;
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    int triggerKind = m_lastCharWasTrigger ? 2 : 1;
    m_lastCharWasTrigger = false;
    client->requestCompletion(uri, {line, col}, triggerKind);
}

void LspEditorIntegration::requestRename()
{
    if (!m_lspActive || !m_lspManager) return;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return;
    QString uri = uriFromPath(filePath);
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().renameProvider) return;
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    bool ok = false;
    QString newName = QInputDialog::getText(nullptr, tr("Rename Symbol"), tr("New name:"),
                                            QLineEdit::Normal, QString(), &ok);
    if (!ok || newName.isEmpty()) return;
    client->requestRename(uri, {line, col}, newName);
}

void LspEditorIntegration::requestDocumentHighlight()
{
    if (!m_lspActive || !m_lspManager) return;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return;
    QString uri = uriFromPath(filePath);
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().documentHighlightProvider) return;
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    client->requestDocumentHighlight(uri, {line, col});
}

void LspEditorIntegration::requestCodeActions()
{
    if (!m_lspActive || !m_lspManager) return;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return;
    QString uri = uriFromPath(filePath);
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().codeActionProvider) return;
    int lineFrom, colFrom, lineTo, colTo;
    m_editor->getSelection(&lineFrom, &colFrom, &lineTo, &colTo);
    Range range{{lineFrom, colFrom}, {lineTo, colTo}};
    client->requestCodeAction(uri, range, {});
}

void LspEditorIntegration::expandSelection()
{
    if (!m_lspActive || !m_lspManager) return;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return;

    int line, col;
    m_editor->getCursorPosition(&line, &col);

    if (!m_selectionRangeStack.isEmpty() && m_selectionRangeDepth > 0) {
        m_selectionRangeDepth--;
        const auto& r = m_selectionRangeStack[m_selectionRangeDepth];
        m_editor->setSelection(r.start.line, r.start.character, r.end.line, r.end.character);
        return;
    }

    QString uri = uriFromPath(filePath);
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().selectionRangeProvider) return;
    QList<Position> positions{{line, col}};
    client->requestSelectionRanges(uri, positions);
}

void LspEditorIntegration::shrinkSelection()
{
    if (m_selectionRangeStack.isEmpty() || m_selectionRangeDepth >= static_cast<int>(m_selectionRangeStack.size()) - 1)
        return;

    m_selectionRangeDepth++;
    const auto& r = m_selectionRangeStack[m_selectionRangeDepth];
    m_editor->setSelection(r.start.line, r.start.character, r.end.line, r.end.character);
}

void LspEditorIntegration::requestSelectionRanges()
{
    expandSelection();
}

void LspEditorIntegration::handleCompletion(const CompletionList& completions)
{
    m_editor->showCompletion(completions);
}

void LspEditorIntegration::handleHighlights(const QJsonArray& highlights)
{
    m_editor->clearHighlights();
    for (const auto& h : highlights) {
        QJsonObject obj = h.toObject();
        auto range = Range::fromJson(obj["range"].toObject());
        m_editor->fillIndicatorRange(range.start.line, range.start.character,
                                     range.end.line, range.end.character, LSP_INDICATOR_HIGHLIGHT);
    }
}

void LspEditorIntegration::handleDiagnostics(const QString& uri, const QList<Diagnostic>& diagnostics)
{
    m_editor->clearDiagnostics();
    for (const auto& d : diagnostics) {
        int indicator = LSP_INDICATOR_ERROR;
        if (d.severityLevel == 2) indicator = LSP_INDICATOR_WARNING;
        else if (d.severityLevel >= 3) indicator = LSP_INDICATOR_INFO;
        m_editor->fillIndicatorRange(d.range.start.line, d.range.start.character,
                                     d.range.end.line, d.range.end.character, indicator);
    }
    emit diagnosticsChanged(uri, diagnostics);
}

static void flattenSelectionRanges(const QJsonObject& node, QList<Range>& out)
{
    if (node.contains("range")) {
        out.append(Range::fromJson(node["range"].toObject()));
    }
    QJsonArray children = node["children"].toArray();
    for (const auto& child : children) {
        flattenSelectionRanges(child.toObject(), out);
    }
}

void LspEditorIntegration::handleSelectionRanges(const QJsonArray& ranges)
{
    m_selectionRangeStack.clear();
    if (ranges.isEmpty()) return;

    QJsonObject root = ranges.first().toObject();
    flattenSelectionRanges(root, m_selectionRangeStack);

    if (m_selectionRangeStack.isEmpty()) return;

    m_selectionRangeDepth = 0;
    const auto& r = m_selectionRangeStack.first();
    m_editor->setSelection(r.start.line, r.start.character, r.end.line, r.end.character);
}

void LspEditorIntegration::handleSemanticTokens(const QString& uri, const QJsonArray& tokens)
{
    m_semanticTokensUri = uri;
    // Re-apply via editor
    m_editor->applySemanticTokens(uri, tokens);
}

void LspEditorIntegration::handleLinkedEditingRanges(const QJsonObject& result)
{
    m_editor->setLinkedEditingRanges(result);
}

void LspEditorIntegration::handleFormatting(const QList<QJsonObject>& edits)
{
    m_editor->applyFormattingEdits(edits);
}

lsp::LspClient* LspEditorIntegration::clientForCurrentFile() const
{
    if (!m_lspManager) return nullptr;
    QString filePath = m_editor->fileName();
    if (filePath.isEmpty()) return nullptr;
    QString uri = uriFromPath(filePath);
    return m_lspManager->clientForUri(uri);
}

void LspEditorIntegration::handleSignatureHelp(const QJsonObject& info)
{
    m_editor->showSignatureHelp(info);
}

void LspEditorIntegration::handleDefinition(const Location& location)
{
    emit navigateToLocation(pathFromUri(location.uri),
                            location.range.start.line,
                            location.range.start.character);
}

void LspEditorIntegration::handleTypeDefinition(const Location& location)
{
    emit navigateToLocation(pathFromUri(location.uri),
                            location.range.start.line,
                            location.range.start.character);
}

void LspEditorIntegration::handleDeclaration(const Location& location)
{
    emit navigateToLocation(pathFromUri(location.uri),
                            location.range.start.line,
                            location.range.start.character);
}

void LspEditorIntegration::handleReferences(const QList<Location>& locations)
{
    if (locations.isEmpty()) return;
    const auto& loc = locations.first();
    emit navigateToLocation(pathFromUri(loc.uri),
                            loc.range.start.line,
                            loc.range.start.character);
}

void LspEditorIntegration::handleCodeActions(const QList<QJsonObject>& actions)
{
    if (actions.isEmpty()) return;
    // Code actions are currently handled via MainWindow for menu display
    // This just logs them
    for (const auto& a : actions) {
        QString title = a["title"].toString();
#ifdef QT_DEBUG
        qDebug() << "Code action available:" << title;
#endif
    }
}

void LspEditorIntegration::handleHover(const QString& contents)
{
    if (contents.isEmpty()) return;
    int pos = m_editor->cursorPosition();
    m_editor->showToolTip(pos, contents);
}

void LspEditorIntegration::handleRename(const QJsonObject& result)
{
    Q_UNUSED(result)
    // Rename is handled via workspace edit application
}

void LspEditorIntegration::handleSymbols(const QJsonArray& symbols)
{
    Q_UNUSED(symbols)
}

} // namespace lsp
