/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuibuffer.h"

#include "appstrings.h"

#include <QFileInfo>

TuiBuffer::TuiBuffer(const QString& filePath, const QString& text, const QString& encoding)
    : m_filePath(filePath), m_encoding(encoding.isEmpty() ? QStringLiteral("UTF-8") : encoding)
{
    setText(text);
}

QString TuiBuffer::text() const
{
    return m_lines.join(QStringLiteral("\n"));
}

void TuiBuffer::setText(const QString& t)
{
    if (t.isEmpty())
        m_lines = QStringList{QStringLiteral("")};
    else
        m_lines = t.split(QStringLiteral("\n"));
    m_cursorLine = 0;
    m_cursorCol = 0;
    m_modified = false;
    m_undoStack.clear();
    m_redoStack.clear();
    clearSelection();
}

TuiBuffer::Snapshot TuiBuffer::snapshot() const
{
    return {m_lines, m_cursorLine, m_cursorCol, m_modified};
}

void TuiBuffer::restore(const Snapshot& s)
{
    m_lines = s.lines;
    m_cursorLine = s.cursorLine;
    m_cursorCol = s.cursorCol;
    m_modified = s.modified;
    ensureCursorValid();
}

void TuiBuffer::pushUndo()
{
    if (m_readOnly)
        return;
    m_undoStack.append(snapshot());
    if (m_undoStack.size() > MaxUndoDepth)
        m_undoStack.removeFirst();
    m_redoStack.clear();
}

bool TuiBuffer::canUndo() const
{
    return !m_undoStack.isEmpty();
}

bool TuiBuffer::canRedo() const
{
    return !m_redoStack.isEmpty();
}

bool TuiBuffer::undo()
{
    if (!canUndo())
        return false;
    m_redoStack.append(snapshot());
    Snapshot s = m_undoStack.takeLast();
    restore(s);
    clearSelection();
    return true;
}

bool TuiBuffer::redo()
{
    if (!canRedo())
        return false;
    m_undoStack.append(snapshot());
    Snapshot s = m_redoStack.takeLast();
    restore(s);
    clearSelection();
    return true;
}

bool TuiBuffer::hasSelection() const
{
    return m_selAnchorLine != -1 && !(m_selAnchorLine == m_cursorLine && m_selAnchorCol == m_cursorCol);
}

void TuiBuffer::setSelectionAnchor(int line, int col)
{
    m_selAnchorLine = line;
    m_selAnchorCol = col;
}

void TuiBuffer::clearSelection()
{
    m_selAnchorLine = -1;
    m_selAnchorCol = -1;
}

QString TuiBuffer::selectedText() const
{
    if (!hasSelection())
        return QString();
    int aLine = m_selAnchorLine;
    int aCol = m_selAnchorCol;
    int cLine = m_cursorLine;
    int cCol = m_cursorCol;
    if (aLine > cLine || (aLine == cLine && aCol > cCol))
    {
        std::swap(aLine, cLine);
        std::swap(aCol, cCol);
    }
    if (aLine == cLine)
        return m_lines[aLine].mid(aCol, cCol - aCol);
    QStringList parts;
    parts.append(m_lines[aLine].mid(aCol));
    for (int l = aLine + 1; l < cLine; ++l)
        parts.append(m_lines[l]);
    parts.append(m_lines[cLine].left(cCol));
    return parts.join(QStringLiteral("\n"));
}

void TuiBuffer::deleteSelection()
{
    if (!hasSelection())
        return;
    pushUndo();
    int aLine = m_selAnchorLine;
    int aCol = m_selAnchorCol;
    int cLine = m_cursorLine;
    int cCol = m_cursorCol;
    if (aLine > cLine || (aLine == cLine && aCol > cCol))
    {
        std::swap(aLine, cLine);
        std::swap(aCol, cCol);
    }
    if (aLine == cLine)
    {
        m_lines[aLine].remove(aCol, cCol - aCol);
        m_cursorLine = aLine;
        m_cursorCol = aCol;
    }
    else
    {
        QString suffix = m_lines[cLine].mid(cCol);
        m_lines[aLine] = m_lines[aLine].left(aCol) + suffix;
        // Remove intermediate lines
        for (int l = cLine; l > aLine; --l)
            m_lines.removeAt(l);
        m_cursorLine = aLine;
        m_cursorCol = aCol;
    }
    clearSelection();
    m_modified = true;
    ensureCursorValid();
}

void TuiBuffer::selectAll()
{
    m_selAnchorLine = 0;
    m_selAnchorCol = 0;
    m_cursorLine = m_lines.size() - 1;
    m_cursorCol = m_lines.last().size();
}

QStringList TuiBuffer::lines() const
{
    return m_lines;
}

int TuiBuffer::lineCount() const
{
    return m_lines.size();
}

void TuiBuffer::setCursor(int line, int col)
{
    m_cursorLine = line;
    m_cursorCol = col;
    ensureCursorValid();
}

void TuiBuffer::moveCursor(int dLine, int dCol)
{
    setCursor(m_cursorLine + dLine, m_cursorCol + dCol);
}

void TuiBuffer::insertText(const QString& s)
{
    if (m_readOnly || s.isEmpty())
        return;
    if (hasSelection())
        deleteSelection();
    else
        pushUndo();
    // Insert s at cursor, handling newlines
    QStringList parts = s.split(QStringLiteral("\n"));
    if (parts.size() == 1)
    {
        QString& line = m_lines[m_cursorLine];
        line.insert(m_cursorCol, s);
        m_cursorCol += s.size();
    }
    else
    {
        QString& curLine = m_lines[m_cursorLine];
        QString after = curLine.mid(m_cursorCol);
        curLine = curLine.left(m_cursorCol) + parts.first();
        int insertPos = m_cursorLine + 1;
        for (int i = 1; i < parts.size() - 1; ++i)
        {
            m_lines.insert(insertPos++, parts[i]);
        }
        m_lines.insert(insertPos, parts.last() + after);
        m_cursorLine += parts.size() - 1;
        m_cursorCol = parts.last().size();
    }
    m_modified = true;
}

void TuiBuffer::insertChar(QChar ch)
{
    if (m_readOnly)
        return;
    if (ch == QLatin1Char('\n') || ch == QLatin1Char('\r'))
    {
        newLine();
        return;
    }
    insertText(QString(ch));
}

void TuiBuffer::backspace()
{
    if (m_readOnly)
        return;
    if (hasSelection())
    {
        deleteSelection();
        return;
    }
    pushUndo();
    if (m_cursorCol > 0)
    {
        QString& line = m_lines[m_cursorLine];
        int col = m_cursorCol;
        line.remove(col - 1, 1);
        m_cursorCol--;
        m_modified = true;
    }
    else if (m_cursorLine > 0)
    {
        int prevLen = m_lines[m_cursorLine - 1].size();
        m_lines[m_cursorLine - 1] += m_lines[m_cursorLine];
        m_lines.removeAt(m_cursorLine);
        m_cursorLine--;
        m_cursorCol = prevLen;
        m_modified = true;
    }
    else
    {
        // No change, pop the undo we just pushed
        if (canUndo())
            m_undoStack.removeLast();
    }
    clearSelection();
}

void TuiBuffer::deleteChar()
{
    if (m_readOnly)
        return;
    if (hasSelection())
    {
        deleteSelection();
        return;
    }
    pushUndo();
    QString& line = m_lines[m_cursorLine];
    bool changed = false;
    if (m_cursorCol < line.size())
    {
        line.remove(m_cursorCol, 1);
        changed = true;
    }
    else if (m_cursorLine + 1 < m_lines.size())
    {
        line += m_lines[m_cursorLine + 1];
        m_lines.removeAt(m_cursorLine + 1);
        changed = true;
    }
    if (changed)
        m_modified = true;
    else if (canUndo())
        m_undoStack.removeLast();
    clearSelection();
}

void TuiBuffer::newLine()
{
    if (m_readOnly)
        return;
    if (hasSelection())
        deleteSelection();
    else
        pushUndo();
    QString& line = m_lines[m_cursorLine];
    QString after = line.mid(m_cursorCol);
    line = line.left(m_cursorCol);
    m_lines.insert(m_cursorLine + 1, after);
    m_cursorLine++;
    m_cursorCol = 0;
    m_modified = true;
    clearSelection();
}

void TuiBuffer::deleteLine(int line)
{
    if (line < 0 || line >= m_lines.size())
        return;
    pushUndo();
    if (m_lines.size() == 1)
    {
        m_lines[0].clear();
        m_cursorLine = 0;
        m_cursorCol = 0;
    }
    else
    {
        m_lines.removeAt(line);
        if (m_cursorLine >= m_lines.size())
            m_cursorLine = m_lines.size() - 1;
        m_cursorCol = qMin(m_cursorCol, m_lines[m_cursorLine].size());
    }
    m_modified = true;
    clearSelection();
}

void TuiBuffer::toggleBookmark(int line)
{
    if (m_bookmarks.contains(line))
        m_bookmarks.removeAll(line);
    else
        m_bookmarks.append(line);
}

bool TuiBuffer::hasBookmark(int line) const
{
    return m_bookmarks.contains(line);
}

QString TuiBuffer::displayName() const
{
    if (m_filePath.isEmpty())
        return Strings::untitled();
    return QFileInfo(m_filePath).fileName();
}

QString TuiBuffer::languageFromExtension() const
{
    if (m_filePath.isEmpty())
        return QString();
    return QFileInfo(m_filePath).suffix().toLower();
}

void TuiBuffer::ensureCursorValid()
{
    if (m_lines.isEmpty())
        m_lines.append(QString());
    m_cursorLine = qBound(0, m_cursorLine, m_lines.size() - 1);
    int lineLen = m_lines[m_cursorLine].size();
    m_cursorCol = qBound(0, m_cursorCol, lineLen);
}
