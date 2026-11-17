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

#ifndef LSPEDITORBRIDGE_H
#define LSPEDITORBRIDGE_H

#include "plugins/lspinterface.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <functional>

class CodeEditor;

/**
 * Host-side IEditorLspBridge implementation. Wraps a CodeEditor and narrows
 * it to exactly the operations the LSP plugin needs, so the plugin never
 * includes host headers or links host code.
 *
 * The bridge is parented to the editor; when the editor is destroyed the
 * bridge dies with it and the plugin drops its session.
 */
class LspEditorBridge : public lsp::IEditorLspBridge
{
    Q_OBJECT

public:
    explicit LspEditorBridge(CodeEditor* editor, QObject* parent = nullptr);
    ~LspEditorBridge() override = default;

    void setNavigateCallback(std::function<void(const QString& filePath, int line, int column)> callback)
    {
        m_navigate = std::move(callback);
    }
    void setIsCurrentCallback(std::function<bool()> callback)
    {
        m_isCurrent = std::move(callback);
    }

    // IEditorLspBridge — document access
    QString fileName() const override;
    QString documentText() const override;
    QString lineText(int line) const override;

    // Cursor / selection
    void getCursorPosition(int* line, int* column) const override;
    void getSelection(int* lineFrom, int* columnFrom, int* lineTo, int* columnTo) const override;
    void setSelection(int lineFrom, int columnFrom, int lineTo, int columnTo) override;
    int cursorPosition() const override;
    void lineIndexFromPosition(int position, int* line, int* column) const override;
    int wordCharCountBeforeCursor() const override;

    // Diagnostics / highlights / formatting / semantic tokens
    void applyDiagnostics(const QString& uri, const QList<lsp::Diagnostic>& diagnostics) override;
    void applyHighlights(const QJsonArray& highlights) override;
    void applyFormattingEdits(const QList<QJsonObject>& edits) override;
    void applyTextEdits(const QList<QJsonObject>& edits) override;
    void applySemanticTokens(const QString& uri, const QJsonArray& tokens) override;
    void setLinkedEditingRanges(const QJsonObject& result) override;

    // Completion / signature help / hover UI
    void showCompletion(const lsp::CompletionList& completions) override;
    void showSignatureHelp(const QJsonObject& info) override;
    void showToolTip(int position, const QString& text) override;

    // Outbound navigation — host decides where to open the file.
    void navigateToLocation(const QString& filePath, int line, int column) override;

    // Session wiring
    lsp::ILspEditorSession* session() const override
    {
        return m_session.data();
    }
    void setSession(lsp::ILspEditorSession* session) override
    {
        m_session = session;
    }
    bool isCurrentEditor() const override
    {
        return m_isCurrent ? m_isCurrent() : false;
    }

private:
    QPointer<CodeEditor> m_editor;
    QPointer<lsp::ILspEditorSession> m_session;
    std::function<void(const QString& filePath, int line, int column)> m_navigate;
    std::function<bool()> m_isCurrent;
};

#endif // LSPEDITORBRIDGE_H
