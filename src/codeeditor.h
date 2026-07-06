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
#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include "settingsmanager.h"
#include "snippetengine.h"
#include "snippet.h"
#include "theme.h"

#include <QScopedPointer>
#include <QTimer>

#include <Qsci/qsciapis.h>
#include <Qsci/qsciglobal.h>
#include <Qsci/qscilexer.h>
#include <Qsci/qsciscintilla.h>

#include "lsp/lsptypes.h"
#include "lsp/lspeditorintegration.h"

class CodeEditor : public QsciScintilla
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget* parent = nullptr);
    ~CodeEditor() override;

    static const int BOOKMARK_MARGIN = 1;
    static const int MARKER_BOOKMARK = 0;

    const QString& fileName() const
    {
        return m_fileName;
    }
    void setFileName(const QString& name)
    {
        m_fileName = name;
    }

    void setSyntax(const QString& language);
    const QString& syntax() const
    {
        return m_syntax;
    }
    void setEncoding(const QString& encoding)
    {
        m_encoding = encoding;
    }
    const QString& encoding() const
    {
        return m_encoding;
    }

    void setLineNumbersVisible(bool visible);
    void applyTheme(ThemeId themeId);
    void applyFont(const QFont& font);
    void setScrollPastEnd(bool enabled);
    void setCodeFolding(bool enabled);
    void setWhitespaceVisible(bool visible);
    void setupAutoCompletion(bool enabled, int threshold, bool caseSensitive);
    void setCursorStyle(CursorStyle style);
    void setCursorBlinking(bool enabled);
    void setHighlightCurrentLine(bool enabled);
    void setVerticalEdge(bool enabled, int column);
    void setAutoCloseBrackets(bool enabled);
    void setReadOnlyMode(bool enabled);
    bool isReadOnlyMode() const;

    void toggleComment();
    void toggleBookmark();
    void toggleBookmark(int line);
    bool hasBookmark(int line) const;
    void nextBookmark();
    void prevBookmark();
    void clearBookmarks();
    QList<int> bookmarkLines() const;
    void setBookmarks(const QList<int>& lines);

    // Snippet support
    void insertSnippet(const Snippet& snippet);
    bool isSnippetMode() const;
    void exitSnippetMode();
    void registerSnippetAutoCompletion(const QList<Snippet>& snippets);
    SnippetEngine* snippetEngine() const { return m_snippetEngine; }

    // LSP support
    void setLspServerManager(lsp::LspServerManager* manager);
    void setLspLanguage(const QString& language);
    lsp::LspServerManager* lspServerManager() const;
    bool lspActive() const;
    void goToDefinition();
    void goToTypeDefinition();
    void goToDeclaration();
    void formatSelection();
    void findReferences();
    void triggerCompletion();
    void requestRename();
    void requestDocumentHighlight();
    void applyHighlights(const QJsonArray& highlights);
    void clearHighlights();
    void applyDiagnostics(const QString& uri, const QList<lsp::Diagnostic>& diagnostics);
    void clearDiagnostics();
    void showCompletion(const lsp::CompletionList& completions);
    void requestCodeActions();
    void expandSelection();
    void shrinkSelection();
    void requestSelectionRanges();
    void sendDidChange();
    void sendDidOpen();
    int documentVersion() const;
    void setDocumentVersion(int v);

    lsp::LspEditorIntegration* lspIntegration() const { return m_lspIntegration; }

    ThemeId themeId() const { return m_themeId; }

    void replaceSelectedText(const QString &text);
    int lineFromPosition(int pos) const;
    int cursorPosition() const;
    void showToolTip(int pos, const QString& text);
    void applyFormattingEdits(const QList<QJsonObject>& edits);
    void showSignatureHelp(const QJsonObject& info);
    void setLinkedEditingRanges(const QJsonObject& result);
    void clearLinkedRanges();
    void applySemanticTokens(const QString& uri, const QJsonArray& tokenData);
    void setSelectionRanges(const QJsonArray& ranges);

signals:
    void fileDropped(const QString& filePath);
    void bookmarksChanged();
    void snippetModeChanged(bool active);
    void findRequested();
    void replaceRequested();
    void goToLineRequested();
    void insertSnippetRequested();
    void navigateToLocation(const QString& filePath, int line, int column);
    void diagnosticsChanged(const QString& uri, const QList<lsp::Diagnostic>& diagnostics);

public slots:
    void forceModified();
    void zoomIn() override;
    void zoomOut() override;
    void zoomReset();

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QString m_fileName;
    QString m_syntax;
    QString m_encoding;
    QFont m_defaultFont;
    ThemeId m_themeId;
    QScopedPointer<QsciLexer> m_lexer;
    QScopedPointer<QsciAPIs> m_apis;
    bool m_lineNumbersVisible = true;
    bool m_autoCloseBrackets = true;

    struct BracketContext
    {
        bool inString = false;
        bool inCharLiteral = false;
        bool inComment = false;
        bool inBlockComment = false;
    };

    BracketContext contextAtPosition(int pos) const;
    bool handleAutoClose(QChar ch, int pos);
    bool handleBracketSkip(QChar ch, int pos);

    void setupEditor();
    void applyLexerTheme();
    void updateLineNumberWidth();
    void onCharAdded(int charadded);
    void setupLspIndicators();

    bool hasLineMarker(int line, int marker) const;

    SnippetEngine* m_snippetEngine = nullptr;

    // LSP integration
    lsp::LspEditorIntegration* m_lspIntegration = nullptr;
};

#endif // CODEEDITOR_H
