/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * PrimoEditor: QQuickItem-based high-perf editor (QSG).
 * GPU-accelerated via QSGTextNode / QSGRectangleNode.
 * Keeps parallel to TUI, not replacing MainWindow (Widget) path.
 */

#ifndef PRIMOEDITOR_H
#define PRIMOEDITOR_H

#include "primodocument.h"
#include "primohighlighter.h"
#include "theme.h"

#include <QFont>
#include <QQuickItem>
#include <QSGNode>
#include <QSet>
#include <QTextLayout>

class PrimoEditor : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString encoding READ encoding WRITE setEncoding NOTIFY encodingChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
    Q_PROPERTY(int tabWidth READ tabWidth WRITE setTabWidth NOTIFY tabWidthChanged)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap NOTIFY wordWrapChanged)
    Q_PROPERTY(int cursorLine READ cursorLine WRITE setCursorLine NOTIFY cursorChanged)
    Q_PROPERTY(int cursorColumn READ cursorColumn WRITE setCursorColumn NOTIFY cursorChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(qreal lineHeight READ lineHeight NOTIFY fontChanged)
    Q_PROPERTY(qreal hScroll READ hScroll NOTIFY hScrollChanged)
    Q_PROPERTY(qreal wrapScrollTop READ wrapScrollTop NOTIFY wrapScrollTopChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QColor foregroundColor READ foregroundColor WRITE setForegroundColor NOTIFY foregroundColorChanged)
    Q_PROPERTY(bool gutterVisible READ gutterVisible WRITE setGutterVisible NOTIFY gutterVisibleChanged)
    Q_PROPERTY(bool showLineNumbers READ showLineNumbers WRITE setShowLineNumbers NOTIFY showLineNumbersChanged)
    Q_PROPERTY(bool relativeNumbers READ relativeNumbers WRITE setRelativeNumbers NOTIFY relativeNumbersChanged)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(bool minimapVisible READ minimapVisible WRITE setMinimapVisible NOTIFY minimapVisibleChanged)
    Q_PROPERTY(qreal gutterWidth READ gutterWidth NOTIFY gutterWidthChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)
    Q_PROPERTY(bool undoDisabled READ isUndoDisabled NOTIFY undoDisabledChanged)

public:
    explicit PrimoEditor(QQuickItem* parent = nullptr);
    ~PrimoEditor() override = default;

    QString filePath() const;
    void setFilePath(const QString& path);

    QString text() const;
    void setText(const QString& t);

    QString language() const;
    void setLanguage(const QString& lang);

    QString encoding() const;
    void setEncoding(const QString& enc);

    QFont font() const;
    void setFont(const QFont& f);

    int tabWidth() const;
    void setTabWidth(int w);

    bool wordWrap() const;
    void setWordWrap(bool w);

    int cursorLine() const;
    void setCursorLine(int line);
    int cursorColumn() const;
    void setCursorColumn(int col);

    qreal hScroll() const;
    void setHScroll(qreal h);
    qreal wrapScrollTop() const;
    void setWrapScrollTop(qreal v);

    qreal contentHeight() const;
    qreal lineHeight() const;

    QColor backgroundColor() const;
    void setBackgroundColor(const QColor& c);
    QColor foregroundColor() const;
    void setForegroundColor(const QColor& c);

    PrimoDocument* document() const;

    bool gutterVisible() const;
    void setGutterVisible(bool v);
    bool showLineNumbers() const;
    void setShowLineNumbers(bool v);
    bool relativeNumbers() const;
    void setRelativeNumbers(bool v);
    bool isReadOnly() const;
    void setReadOnly(bool ro);
    bool minimapVisible() const;
    void setMinimapVisible(bool v);
    qreal gutterWidth() const;

    Q_INVOKABLE void loadFile(const QString& path);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveAs(const QString& path);
    Q_INVOKABLE void setCursorPosition(int line, int column);
    Q_INVOKABLE void insertAtCursor(const QString& s);
    Q_INVOKABLE void backspaceAtCursor();

    // Bookmarks (like CodeEditor BOOKMARK_MARGIN)
    Q_INVOKABLE void toggleBookmark(int line);
    Q_INVOKABLE void toggleBookmarkCurrent();
    Q_INVOKABLE bool hasBookmark(int line) const;
    Q_INVOKABLE QList<int> bookmarkLines() const;
    Q_INVOKABLE void clearBookmarks();
    Q_INVOKABLE void nextBookmark();
    Q_INVOKABLE void prevBookmark();
    Q_INVOKABLE void findNext(const QString& pattern, bool caseSensitive = false, bool wholeWord = false, bool regex = false);
    Q_INVOKABLE void findPrevious(const QString& pattern, bool caseSensitive = false, bool wholeWord = false, bool regex = false);

    // Search match highlighting
    Q_INVOKABLE void setSearchMatches(const QList<int>& positions);
    Q_INVOKABLE void clearSearchMatches();

    // Auto-completion / snippets
    Q_INVOKABLE QStringList completionList(const QString& prefix) const;
    Q_INVOKABLE void insertSnippet(const QString& trigger);
    Q_INVOKABLE QString currentWord() const;

    // Selection model
    Q_INVOKABLE bool hasSelection() const;
    Q_INVOKABLE QString selectedText() const;
    Q_INVOKABLE void setSelectionAnchor(int line, int col);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void deleteSelection();
    Q_INVOKABLE int selectionAnchorLine() const;
    Q_INVOKABLE int selectionAnchorCol() const;

    // Diagnostics for minimap
    Q_INVOKABLE void setDiagnostics(const QList<int>& linesWithError);
    Q_INVOKABLE void clearDiagnostics();

    // Undo/Clipboard (incremental disabled >50MB)
    bool canUndo() const;
    bool canRedo() const;
    bool isUndoDisabled() const;
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void cut();
    Q_INVOKABLE void paste();
    Q_INVOKABLE void selectAll();

    // Theme support (full ThemeColors for custom themes)
    Q_INVOKABLE void applyThemeColors(const ThemeColors& colors);
    Q_INVOKABLE void setCustomTheme(const QString& name);
    Q_INVOKABLE QString customTheme() const;

signals:
    void filePathChanged();
    void textChanged();
    void languageChanged();
    void encodingChanged();
    void fontChanged();
    void tabWidthChanged();
    void wordWrapChanged();
    void cursorChanged();
    void contentHeightChanged();
    void hScrollChanged();
    void wrapScrollTopChanged();
    void backgroundColorChanged();
    void foregroundColorChanged();
    void gutterVisibleChanged();
    void showLineNumbersChanged();
    void relativeNumbersChanged();
    void readOnlyChanged();
    void minimapVisibleChanged();
    void gutterWidthChanged();
    void bookmarksChanged();
    void diagnosticsChanged();
    void canUndoChanged();
    void canRedoChanged();
    void undoDisabledChanged();
    void fileLoaded(bool ok, const QString& error);
    void fileSaved(bool ok, const QString& error);

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    void ensureMetrics();
    void schedulePolish();
    QString expandedTab(const QString& line) const;
    void scheduleHighlight(int firstLine = -1, int lastLine = -1);
    void applyHighlightResult(int version, int firstLine, const QVector<QVector<QTextLayout::FormatRange>>& fmts);
    void applySettings();
    void ensureCursorVisibleHorizontally();
    // Word wrap support
    struct WrapInfo
    {
        int line = 0;
        int startCol = 0;
        int len = 0;
    };
    QList<WrapInfo> buildWrapInfos(int availGlobal, int* outCursorVisualRow = nullptr);

    PrimoDocument* m_doc = nullptr;
    QString m_encoding = QStringLiteral("UTF-8");
    QFont m_font = QFont(QStringLiteral("Monospace"), 11);
    int m_tabWidth = 4;
    bool m_wordWrap = false;
    int m_cursorLine = 0;
    int m_cursorColumn = 0;
    QColor m_bg = QColor(0x1e, 0x1e, 0x2e);
    QColor m_fg = QColor(0xcd, 0xd6, 0xf4);
    qreal m_lineHeight = 18;
    qreal m_charWidth = 8;
    qreal m_contentHeight = 0;
    QStringList m_linesCache;
    bool m_linesDirty = true;

    // Gutter / bookmarks / readOnly
    bool m_gutterVisible = true;
    bool m_showLineNumbers = true;
    bool m_relativeNumbers = false;
    bool m_readOnly = false;
    bool m_minimapVisible = true;
    qreal m_gutterWidth = 48;
    qreal m_hScroll = 0;
    qreal m_wrapScrollTop = 0;
    QSet<int> m_bookmarks;
    QSet<int> m_diagLines;
    QList<QPair<int, int>> m_searchMatches;

    // Selection model
    int m_selAnchorLine = -1;
    int m_selAnchorCol = -1;

    // Highlight
    ThemeColors m_theme = getThemeColors(ThemeId::Dark);
    PrimoHighlighterController* m_highlightController = nullptr;
    int m_highlightVersion = 0;
    QVector<QVector<QTextLayout::FormatRange>> m_highlightCache;
    bool m_highlightDirty = true;
    QString m_customTheme;
};

#endif // PRIMOEDITOR_H
