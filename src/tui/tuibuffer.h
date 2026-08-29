/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * TUI buffer: headless text model used by terminal UI.
 */

#ifndef TUIBUFFER_H
#define TUIBUFFER_H

#include <QList>
#include <QString>
#include <QStringList>

class TuiBuffer
{
public:
    explicit TuiBuffer(const QString& filePath = QString(), const QString& text = QString(), const QString& encoding = QStringLiteral("UTF-8"));

    const QString& filePath() const { return m_filePath; }
    void setFilePath(const QString& p) { m_filePath = p; }

    const QString& encoding() const { return m_encoding; }
    void setEncoding(const QString& e) { m_encoding = e; }

    QString text() const;
    void setText(const QString& t);

    QStringList lines() const;
    int lineCount() const;

    bool isModified() const { return m_modified; }
    void setModified(bool m) { m_modified = m; }

    bool isReadOnly() const { return m_readOnly; }
    void setReadOnly(bool ro) { m_readOnly = ro; }

    // Cursor is 0-based line/col
    int cursorLine() const { return m_cursorLine; }
    int cursorCol() const { return m_cursorCol; }
    void setCursor(int line, int col);
    void moveCursor(int dLine, int dCol);

    // Editing primitives
    void insertText(const QString& s);
    void insertChar(QChar ch);
    void backspace();
    void deleteChar();
    void newLine();
    void deleteLine(int line);

    // Bookmark support
    QList<int> bookmarks() const { return m_bookmarks; }
    void setBookmarks(const QList<int>& b) { m_bookmarks = b; }
    void toggleBookmark(int line);
    bool hasBookmark(int line) const;

    // Undo/Redo
    void pushUndo();
    bool canUndo() const;
    bool canRedo() const;
    bool undo();
    bool redo();

    // Selection
    bool hasSelection() const;
    void setSelectionAnchor(int line, int col);
    void clearSelection();
    int selectionAnchorLine() const { return m_selAnchorLine; }
    int selectionAnchorCol() const { return m_selAnchorCol; }
    QString selectedText() const;
    void deleteSelection();
    void selectAll();

    QString displayName() const;
    QString languageFromExtension() const;

private:
    void ensureCursorValid();
    struct Snapshot
    {
        QStringList lines;
        int cursorLine = 0;
        int cursorCol = 0;
        bool modified = false;
    };
    Snapshot snapshot() const;
    void restore(const Snapshot& s);

    QList<Snapshot> m_undoStack;
    QList<Snapshot> m_redoStack;
    static constexpr int MaxUndoDepth = 200;

    int m_selAnchorLine = -1;
    int m_selAnchorCol = -1;
    QString m_filePath;
    QString m_encoding;
    QStringList m_lines;
    bool m_modified = false;
    bool m_readOnly = false;
    int m_cursorLine = 0;
    int m_cursorCol = 0;
    QList<int> m_bookmarks;
};

#endif // TUIBUFFER_H
