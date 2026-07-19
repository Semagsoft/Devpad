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
#ifndef LSPEDITORINTEGRATION_H
#define LSPEDITORINTEGRATION_H

#include "lsptypes.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

class CodeEditor;

namespace lsp
{

class LspServerManager;
class LspClient;

class LspEditorIntegration : public QObject
{
    Q_OBJECT

public:
    explicit LspEditorIntegration(CodeEditor* editor, QObject* parent = nullptr);
    ~LspEditorIntegration() override;

    void setServerManager(LspServerManager* manager);
    LspServerManager* serverManager() const
    {
        return m_lspManager;
    }

    void setLanguage(const QString& language);
    const QString& language() const
    {
        return m_lspLanguage;
    }
    bool isActive() const
    {
        return m_lspActive;
    }
    int documentVersion() const
    {
        return m_docVersion;
    }
    void setDocumentVersion(int v)
    {
        m_docVersion = v;
    }

    // LSP actions
    void goToDefinition();
    void goToTypeDefinition();
    void goToDeclaration();
    void formatSelection();
    void findReferences();
    void triggerCompletion();
    void requestRename();
    void requestDocumentHighlight();
    void requestCodeActions();
    void expandSelection();
    void shrinkSelection();
    void requestSelectionRanges();
    void sendDidChange();
    void sendDidOpen();

    // Handlers for LSP responses
    void handleCompletion(const CompletionList& completions);
    void handleHighlights(const QJsonArray& highlights);
    void handleDiagnostics(const QString& uri, const QList<Diagnostic>& diagnostics);
    void handleSelectionRanges(const QJsonArray& ranges);
    void handleSemanticTokens(const QString& uri, const QJsonArray& tokens);
    void handleLinkedEditingRanges(const QJsonObject& result);
    void handleFormatting(const QList<QJsonObject>& edits);
    void handleSignatureHelp(const QJsonObject& info);
    void handleDefinition(const Location& location);
    void handleTypeDefinition(const Location& location);
    void handleDeclaration(const Location& location);
    void handleReferences(const QList<Location>& locations);
    void handleCodeActions(const QList<QJsonObject>& actions);
    void handleHover(const QString& contents);
    void handleRename(const QJsonObject& result);
    void handleSymbols(const QJsonArray& symbols);

    // Timer and trigger helpers
    void startCompletionTimer()
    {
        if (m_lspActive)
            m_completionTimer->start();
    }
    void startDiagnosticsTimer()
    {
        if (m_lspActive)
            m_diagnosticsTimer->start();
    }

    // Completion, linked editing, selection, and token state
    const QList<CompletionItem>& completionItems() const
    {
        return m_lspCompletionItems;
    }
    void setCompletionItems(const QList<CompletionItem>& items)
    {
        m_lspCompletionItems = items;
    }
    const QList<Range>& linkedRanges() const
    {
        return m_linkedRanges;
    }
    void setLinkedRanges(const QList<Range>& ranges)
    {
        m_linkedRanges = ranges;
    }
    bool isApplyingLinkedEdit() const
    {
        return m_isApplyingLinkedEdit;
    }
    void setApplyingLinkedEdit(bool v)
    {
        m_isApplyingLinkedEdit = v;
    }
    const QString& semanticTokensUri() const
    {
        return m_semanticTokensUri;
    }
    void setSemanticTokensUri(const QString& uri)
    {
        m_semanticTokensUri = uri;
    }

    int selectionRangeDepth() const
    {
        return m_selectionRangeDepth;
    }
    void setSelectionRangeDepth(int d)
    {
        m_selectionRangeDepth = d;
    }
    const QList<Range>& selectionRangeStack() const
    {
        return m_selectionRangeStack;
    }
    void setSelectionRangeStack(const QList<Range>& stack)
    {
        m_selectionRangeStack = stack;
    }

    int lastTriggerChar() const
    {
        return m_lastTriggerChar;
    }
    void setLastTriggerChar(int c)
    {
        m_lastTriggerChar = c;
    }
    bool lastCharWasTrigger() const
    {
        return m_lastCharWasTrigger;
    }
    void setLastCharWasTrigger(bool v)
    {
        m_lastCharWasTrigger = v;
    }

    void setSelectionRanges(const QJsonArray& ranges);
    LspClient* clientForCurrentFile() const;

signals:
    void diagnosticsChanged(const QString& uri, const QList<Diagnostic>& diagnostics);
    void navigateToLocation(const QString& filePath, int line, int column);

private:
    CodeEditor* m_editor;
    LspServerManager* m_lspManager = nullptr;
    QString m_lspLanguage;
    bool m_lspActive = false;
    bool m_lspCompletionActive = false;
    int m_docVersion = 0;
    QList<CompletionItem> m_lspCompletionItems;
    QTimer* m_completionTimer = nullptr;
    QTimer* m_diagnosticsTimer = nullptr;
    QTimer* m_highlightTimer = nullptr;
    int m_lastTriggerChar = 0;
    bool m_lastCharWasTrigger = false;

    // Linked editing
    QList<Range> m_linkedRanges;
    bool m_isApplyingLinkedEdit = false;

    // Semantic tokens
    QString m_semanticTokensUri;

    // Selection range stack
    QList<Range> m_selectionRangeStack;
    int m_selectionRangeDepth = 0;
};

} // namespace lsp

#endif // LSPEDITORINTEGRATION_H
