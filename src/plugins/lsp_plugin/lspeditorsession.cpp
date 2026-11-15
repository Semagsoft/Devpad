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

#include "lspeditorsession.h"

#include "appstrings.h"
#include "lsp/lsptypes.h"
#include "lspclient.h"
#include "lspservermanager.h"
#include "managers/settingsmanager.h"
#include "plugins/lspinterface.h"

#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <functional>

namespace lsp
{

LspEditorSession::LspEditorSession(IEditorLspBridge* bridge, QObject* parent) : ILspEditorSession(parent), m_bridge(bridge)
{

    m_completionTimer = new QTimer(this);
    m_completionTimer->setSingleShot(true);
    m_completionTimer->setInterval(200);
    connect(m_completionTimer, &QTimer::timeout, this,
            [this]()
            {
                if (m_lspActive && m_lspManager)
                    triggerCompletion();
            });

    m_diagnosticsTimer = new QTimer(this);
    m_diagnosticsTimer->setSingleShot(true);
    m_diagnosticsTimer->setInterval(500);
    connect(m_diagnosticsTimer, &QTimer::timeout, this,
            [this]()
            {
                if (m_lspActive)
                    sendDidChange();
            });

    m_highlightTimer = new QTimer(this);
    m_highlightTimer->setSingleShot(true);
    m_highlightTimer->setInterval(300);
    connect(m_highlightTimer, &QTimer::timeout, this, [this]() { requestDocumentHighlight(); });

    // If the editor (and its bridge) dies, drop straight into an inactive
    // state; the plugin deletes the session when the destroyed bridge is
    // observed.
    if (m_bridge)
        connect(m_bridge, &QObject::destroyed, this, QOverload<>::of(&QObject::deleteLater));
}

void LspEditorSession::setLanguage(const QString& language)
{
    m_lspLanguage = language;
    m_lspActive = !language.isEmpty();
    m_lspCompletionActive = false;
    m_docVersion = 0;
}

void LspEditorSession::setServerManager(LspServerManager* manager)
{
    m_lspManager = manager;
}

QString LspEditorSession::currentUri() const
{
    if (!m_bridge)
        return QString();
    const QString filePath = m_bridge->fileName();
    if (filePath.isEmpty() || filePath == Strings::untitled())
        return QString();
    return uriFromPath(filePath);
}

LspClient* LspEditorSession::clientForCurrentFile() const
{
    if (!m_lspManager)
        return nullptr;
    const QString uri = currentUri();
    if (uri.isEmpty())
        return nullptr;
    return m_lspManager->clientForUri(uri);
}

void LspEditorSession::sendDidOpen()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;

    const QString uri = currentUri();
    if (uri.isEmpty())
        return;

    m_lspManager->openDocument(m_lspLanguage, uri, m_bridge->documentText());
}

void LspEditorSession::sendDidChange()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;

    const QString uri = currentUri();
    if (uri.isEmpty())
        return;

    m_lspManager->changeDocument(uri, m_bridge->documentText());
}

void LspEditorSession::onCharAdded(int charAdded)
{
    if (!m_lspActive || !m_bridge)
        return;

    QChar ch(charAdded);
    m_lastTriggerChar = charAdded;
    m_lastCharWasTrigger = false;

    LspClient* client = clientForCurrentFile();
    if (client && client->capabilities().completionProvider)
    {
        for (const auto& tc : client->capabilities().completionTriggerChars)
        {
            if (!tc.isEmpty() && QChar(tc[0]) == ch)
            {
                m_lastCharWasTrigger = true;
                break;
            }
        }
    }

    if (m_lastCharWasTrigger || ch.isLetterOrNumber() || ch == QLatin1Char('.') || ch == QLatin1Char('>') || ch == QLatin1Char(':') ||
        ch == QLatin1Char('/') || ch == QLatin1Char('-') || ch == QLatin1Char('_'))
    {
        const int threshold = SettingsManager::instance().lspCompletionTriggerChars();
        const int count = m_bridge->wordCharCountBeforeCursor();
        if (m_lastCharWasTrigger || count >= threshold)
            startCompletionTimer();
    }
    else if (ch == QLatin1Char('('))
    {
        int line, col;
        m_bridge->getCursorPosition(&line, &col);
        const QString lineText = m_bridge->lineText(line);
        const Position pos{line, qMax(0, charIndexToUtf16Col(lineText, col - 1))};
        if (client)
        {
            const QString uri = currentUri();
            if (!uri.isEmpty())
                client->requestSignatureHelp(uri, pos);
        }
    }

    startDiagnosticsTimer();
}

void LspEditorSession::requestHover(int position)
{
    if (!m_lspActive || !m_bridge)
        return;

    const QString uri = currentUri();
    if (uri.isEmpty())
        return;

    LspClient* client = clientForCurrentFile();
    if (!client || !client->capabilities().hoverProvider)
        return;

    int line, col;
    m_bridge->lineIndexFromPosition(position, &line, &col);
    const QString lineText = m_bridge->lineText(line);
    const Position pos{line, charIndexToUtf16Col(lineText, col)};
    client->requestHover(uri, pos);
}

void LspEditorSession::goToDefinition()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;
    const QString uri = currentUri();
    if (uri.isEmpty())
        return;
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().definitionProvider)
        return;
    int line, col;
    m_bridge->getCursorPosition(&line, &col);
    const QString lineText = m_bridge->lineText(line);
    client->requestDefinition(uri, {line, charIndexToUtf16Col(lineText, col)});
}

void LspEditorSession::goToTypeDefinition()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;
    const QString uri = currentUri();
    if (uri.isEmpty())
        return;
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().typeDefinitionProvider)
        return;
    int line, col;
    m_bridge->getCursorPosition(&line, &col);
    const QString lineText = m_bridge->lineText(line);
    client->requestTypeDefinition(uri, {line, charIndexToUtf16Col(lineText, col)});
}

void LspEditorSession::goToDeclaration()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;
    const QString uri = currentUri();
    if (uri.isEmpty())
        return;
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().declarationProvider)
        return;
    int line, col;
    m_bridge->getCursorPosition(&line, &col);
    const QString lineText = m_bridge->lineText(line);
    client->requestDeclaration(uri, {line, charIndexToUtf16Col(lineText, col)});
}

void LspEditorSession::formatSelection()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;
    const QString uri = currentUri();
    if (uri.isEmpty())
        return;
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().formattingProvider)
        return;
    int lineFrom, colFrom, lineTo, colTo;
    m_bridge->getSelection(&lineFrom, &colFrom, &lineTo, &colTo);
    const QString fromLineText = m_bridge->lineText(lineFrom);
    const QString toLineText = m_bridge->lineText(lineTo);
    Range range{{lineFrom, charIndexToUtf16Col(fromLineText, colFrom)}, {lineTo, charIndexToUtf16Col(toLineText, colTo)}};
    const int tabSize = SettingsManager::instance().tabWidth();
    const bool insertSpaces = true;
    client->requestRangeFormatting(uri, range, tabSize, insertSpaces);
}

void LspEditorSession::findReferences()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;
    const QString uri = currentUri();
    if (uri.isEmpty())
        return;
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().referencesProvider)
        return;
    int line, col;
    m_bridge->getCursorPosition(&line, &col);
    const QString lineText = m_bridge->lineText(line);
    client->requestReferences(uri, {line, charIndexToUtf16Col(lineText, col)});
}

void LspEditorSession::triggerCompletion()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;
    const QString uri = currentUri();
    if (uri.isEmpty())
        return;
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().completionProvider)
        return;
    int line, col;
    m_bridge->getCursorPosition(&line, &col);
    const QString lineText = m_bridge->lineText(line);
    const int triggerKind = m_lastCharWasTrigger ? 2 : 1;
    m_lastCharWasTrigger = false;
    client->requestCompletion(uri, {line, charIndexToUtf16Col(lineText, col)}, triggerKind);
}

void LspEditorSession::requestRename()
{
    if (!m_lspActive || !m_bridge)
        return;
    const QString uri = currentUri();
    if (uri.isEmpty())
        return;

    LspClient* client = clientForCurrentFile();
    if (!client || !client->capabilities().renameProvider)
        return;

    bool ok = false;
    const QString newName = QInputDialog::getText(nullptr, QObject::tr("Rename Symbol"), QObject::tr("New name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || newName.isEmpty())
        return;

    int line, col;
    m_bridge->getCursorPosition(&line, &col);
    const QString lineText = m_bridge->lineText(line);
    client->requestRename(uri, {line, charIndexToUtf16Col(lineText, col)}, newName);
}

void LspEditorSession::requestDocumentHighlight()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;
    const QString uri = currentUri();
    if (uri.isEmpty())
        return;
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().documentHighlightProvider)
        return;
    int line, col;
    m_bridge->getCursorPosition(&line, &col);
    const QString lineText = m_bridge->lineText(line);
    client->requestDocumentHighlight(uri, {line, charIndexToUtf16Col(lineText, col)});
}

void LspEditorSession::requestCodeActions()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;
    const QString uri = currentUri();
    if (uri.isEmpty())
        return;
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().codeActionProvider)
        return;
    int lineFrom, colFrom, lineTo, colTo;
    m_bridge->getSelection(&lineFrom, &colFrom, &lineTo, &colTo);
    const QString fromLineText = m_bridge->lineText(lineFrom);
    const QString toLineText = m_bridge->lineText(lineTo);
    Range range{{lineFrom, charIndexToUtf16Col(fromLineText, colFrom)}, {lineTo, charIndexToUtf16Col(toLineText, colTo)}};
    client->requestCodeAction(uri, range, {});
}

void LspEditorSession::expandSelection()
{
    if (!m_lspActive || !m_lspManager || !m_bridge)
        return;
    const QString filePath = m_bridge->fileName();
    if (filePath.isEmpty())
        return;

    int line, col;
    m_bridge->getCursorPosition(&line, &col);

    if (!m_selectionRangeStack.isEmpty() && m_selectionRangeDepth > 0)
    {
        m_selectionRangeDepth--;
        const auto& r = m_selectionRangeStack[m_selectionRangeDepth];
        const QString startLineText = m_bridge->lineText(r.start.line);
        const QString endLineText = m_bridge->lineText(r.end.line);
        m_bridge->setSelection(r.start.line, utf16ColToCharIndex(startLineText, r.start.character), r.end.line,
                               utf16ColToCharIndex(endLineText, r.end.character));
        return;
    }

    const QString uri = currentUri();
    if (uri.isEmpty())
        return;
    LspClient* client = m_lspManager->clientForUri(uri);
    if (!client || !client->capabilities().selectionRangeProvider)
        return;
    const QString lineText = m_bridge->lineText(line);
    QList<Position> positions{{line, charIndexToUtf16Col(lineText, col)}};
    client->requestSelectionRanges(uri, positions);
}

void LspEditorSession::shrinkSelection()
{
    if (!m_bridge)
        return;
    if (m_selectionRangeStack.isEmpty() || m_selectionRangeDepth >= static_cast<int>(m_selectionRangeStack.size()) - 1)
        return;

    m_selectionRangeDepth++;
    const auto& r = m_selectionRangeStack[m_selectionRangeDepth];
    const QString startLineText = m_bridge->lineText(r.start.line);
    const QString endLineText = m_bridge->lineText(r.end.line);
    m_bridge->setSelection(r.start.line, utf16ColToCharIndex(startLineText, r.start.character), r.end.line,
                           utf16ColToCharIndex(endLineText, r.end.character));
}

void LspEditorSession::handleSelectionRanges(const QJsonArray& ranges)
{
    m_selectionRangeStack.clear();
    m_selectionRangeDepth = 0;

    if (!m_bridge)
        return;

    std::function<void(const QJsonObject&, QList<Range>&)> flattenSelectionRanges = [&](const QJsonObject& node, QList<Range>& out)
    {
        if (node.contains(QStringLiteral("range")))
            out.append(Range::fromJson(node[QStringLiteral("range")].toObject()));
        const QJsonArray children = node[QStringLiteral("children")].toArray();
        for (const auto& child : children)
            flattenSelectionRanges(child.toObject(), out);
    };

    if (ranges.isEmpty())
        return;

    QJsonObject root = ranges.first().toObject();
    flattenSelectionRanges(root, m_selectionRangeStack);

    if (m_selectionRangeStack.isEmpty())
        return;

    const auto& r = m_selectionRangeStack.first();
    const QString startLineText = m_bridge->lineText(r.start.line);
    const QString endLineText = m_bridge->lineText(r.end.line);
    m_bridge->setSelection(r.start.line, utf16ColToCharIndex(startLineText, r.start.character), r.end.line,
                           utf16ColToCharIndex(endLineText, r.end.character));
}

void LspEditorSession::startCompletionTimer()
{
    if (m_lspActive && m_completionTimer)
        m_completionTimer->start();
}

void LspEditorSession::startDiagnosticsTimer()
{
    if (m_lspActive && m_diagnosticsTimer)
        m_diagnosticsTimer->start();
}

} // namespace lsp
