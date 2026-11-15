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

#include "lspeditorbridge.h"

#include "codeeditor.h"

#include <utility>

LspEditorBridge::LspEditorBridge(CodeEditor* editor, QObject* parent)
    : IEditorLspBridge(parent ? parent : static_cast<QObject*>(editor)), m_editor(editor)
{
}

QString LspEditorBridge::fileName() const
{
    return m_editor ? m_editor->fileName() : QString();
}

QString LspEditorBridge::documentText() const
{
    return m_editor ? m_editor->text() : QString();
}

QString LspEditorBridge::lineText(int line) const
{
    return m_editor ? m_editor->text(line) : QString();
}

void LspEditorBridge::getCursorPosition(int* line, int* column) const
{
    if (!m_editor)
    {
        if (line)
            *line = 0;
        if (column)
            *column = 0;
        return;
    }
    m_editor->getCursorPosition(line, column);
}

void LspEditorBridge::getSelection(int* lineFrom, int* columnFrom, int* lineTo, int* columnTo) const
{
    if (!m_editor)
    {
        if (lineFrom)
            *lineFrom = 0;
        if (columnFrom)
            *columnFrom = 0;
        if (lineTo)
            *lineTo = 0;
        if (columnTo)
            *columnTo = 0;
        return;
    }
    m_editor->getSelection(lineFrom, columnFrom, lineTo, columnTo);
}

void LspEditorBridge::setSelection(int lineFrom, int columnFrom, int lineTo, int columnTo)
{
    if (m_editor)
        m_editor->setSelection(lineFrom, columnFrom, lineTo, columnTo);
}

int LspEditorBridge::cursorPosition() const
{
    return m_editor ? m_editor->cursorPosition() : 0;
}

void LspEditorBridge::lineIndexFromPosition(int position, int* line, int* column) const
{
    if (!m_editor)
    {
        if (line)
            *line = 0;
        if (column)
            *column = 0;
        return;
    }
    m_editor->lineIndexFromPosition(position, line, column);
}

int LspEditorBridge::wordCharCountBeforeCursor() const
{
    return m_editor ? m_editor->wordCharCountBeforeCursor() : 0;
}

void LspEditorBridge::applyDiagnostics(const QString& uri, const QList<lsp::Diagnostic>& diagnostics)
{
    if (m_editor)
        m_editor->applyDiagnostics(uri, diagnostics);
}

void LspEditorBridge::applyHighlights(const QJsonArray& highlights)
{
    if (m_editor)
        m_editor->applyHighlights(highlights);
}

void LspEditorBridge::applyFormattingEdits(const QList<QJsonObject>& edits)
{
    if (m_editor)
        m_editor->applyFormattingEdits(edits);
}

void LspEditorBridge::applyTextEdits(const QList<QJsonObject>& edits)
{
    if (m_editor)
        m_editor->applyTextEdits(edits);
}

void LspEditorBridge::applySemanticTokens(const QString& uri, const QJsonArray& tokens)
{
    if (m_editor)
        m_editor->applySemanticTokens(uri, tokens);
}

void LspEditorBridge::setLinkedEditingRanges(const QJsonObject& result)
{
    if (m_editor)
        m_editor->setLinkedEditingRanges(result);
}

void LspEditorBridge::showCompletion(const lsp::CompletionList& completions)
{
    if (m_editor)
        m_editor->showCompletion(completions);
}

void LspEditorBridge::showSignatureHelp(const QJsonObject& info)
{
    if (m_editor)
        m_editor->showSignatureHelp(info);
}

void LspEditorBridge::showToolTip(int position, const QString& text)
{
    if (m_editor)
        m_editor->showToolTip(position, text);
}

void LspEditorBridge::navigateToLocation(const QString& filePath, int line, int column)
{
    if (m_navigate)
        m_navigate(filePath, line, column);
}
