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
    return {m_lines, m_bookmarks, m_cursorLine, m_cursorCol, m_modified};
}

void TuiBuffer::restore(const Snapshot& s)
{
    m_lines = s.lines;
    m_bookmarks = s.bookmarks;
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
    return m_selAnchorLine != -1 && (m_selAnchorLine != m_cursorLine || m_selAnchorCol != m_cursorCol);
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
        // Check if any bookmark in deleted range should be preserved on aLine
        bool hadBookmarkInDeleted = false;
        for (int b : m_bookmarks)
        {
            if (b > aLine && b <= cLine)
                hadBookmarkInDeleted = true;
        }
        QString suffix = m_lines[cLine].mid(cCol);
        m_lines[aLine] = m_lines[aLine].left(aCol) + suffix;
        // Remove intermediate lines
        shiftBookmarksOnDelete(aLine + 1, cLine - aLine);
        if (hadBookmarkInDeleted && !m_bookmarks.contains(aLine))
        {
            m_bookmarks.append(aLine);
            std::sort(m_bookmarks.begin(), m_bookmarks.end());
        }
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

bool TuiBuffer::selectionRangeForLine(int lineIdx, int hScroll, int avail, int* outStart, int* outEnd) const
{
    if (!hasSelection())
        return false;
    int aLine = m_selAnchorLine;
    int aCol = m_selAnchorCol;
    int cLine = m_cursorLine;
    int cCol = m_cursorCol;
    if (aLine > cLine || (aLine == cLine && aCol > cCol))
        std::swap(aLine, cLine), std::swap(aCol, cCol);
    if (lineIdx < aLine || lineIdx > cLine)
        return false;
    int s = -1, e = -1;
    if (aLine == cLine)
    {
        s = aCol - hScroll;
        e = cCol - hScroll;
    }
    else if (lineIdx == aLine)
    {
        s = aCol - hScroll;
        e = avail;
    }
    else if (lineIdx == cLine)
    {
        s = 0 - hScroll;
        if (s < 0)
            s = 0;
        e = cCol - hScroll;
    }
    else
    {
        s = 0;
        e = avail;
    }
    s = qBound(0, s, avail);
    e = qBound(0, e, avail);
    if (s == e)
        return false;
    if (outStart)
        *outStart = s;
    if (outEnd)
        *outEnd = e;
    return true;
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
        int origLine = m_cursorLine;
        int inserted = static_cast<int>(parts.size()) - 1;
        shiftBookmarksOnInsert(origLine + 1, inserted);
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
    // Bracket auto-close and skip
    if (m_autoCloseBrackets)
    {
        // Skip over closing bracket if next char is same
        if (ch == QLatin1Char(')') || ch == QLatin1Char(']') || ch == QLatin1Char('}') || ch == QLatin1Char('"') || ch == QLatin1Char('\''))
        {
            QString line = m_lines[m_cursorLine];
            if (m_cursorCol < line.size() && line[m_cursorCol] == ch)
            {
                // Simple check: if we are at closing char, skip
                setCursor(m_cursorLine, m_cursorCol + 1);
                return;
            }
        }
        // Auto-close pairs
        QChar closing;
        bool doPair = false;
        if (ch == QLatin1Char('('))
        {
            closing = QLatin1Char(')');
            doPair = true;
        }
        else if (ch == QLatin1Char('['))
        {
            closing = QLatin1Char(']');
            doPair = true;
        }
        else if (ch == QLatin1Char('{'))
        {
            closing = QLatin1Char('}');
            doPair = true;
        }
        else if (ch == QLatin1Char('"'))
        {
            closing = QLatin1Char('"');
            doPair = true;
        }
        else if (ch == QLatin1Char('\''))
        {
            closing = QLatin1Char('\'');
            doPair = true;
        }
        if (doPair)
        {
            // Insert opening, then closing, and place cursor between
            insertText(QString(ch));
            // Insert closing at current cursor (which is after opening)
            QString& line = m_lines[m_cursorLine];
            line.insert(m_cursorCol, QString(closing));
            m_modified = true;
            return;
        }
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
        int removedLine = m_cursorLine;
        int prevLen = m_lines[m_cursorLine - 1].size();
        // Preserve bookmark: if removed line had bookmark, move it to previous line if not already
        bool hadMark = m_bookmarks.contains(removedLine);
        shiftBookmarksOnDelete(removedLine, 1);
        if (hadMark && !m_bookmarks.contains(removedLine - 1))
        {
            m_bookmarks.append(removedLine - 1);
            std::sort(m_bookmarks.begin(), m_bookmarks.end());
        }
        m_lines[m_cursorLine - 1] += m_lines[removedLine];
        m_lines.removeAt(removedLine);
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
        int removedLine = m_cursorLine + 1;
        bool hadMark = m_bookmarks.contains(removedLine);
        shiftBookmarksOnDelete(removedLine, 1);
        if (hadMark && !m_bookmarks.contains(m_cursorLine))
        {
            m_bookmarks.append(m_cursorLine);
            std::sort(m_bookmarks.begin(), m_bookmarks.end());
        }
        line += m_lines[removedLine];
        m_lines.removeAt(removedLine);
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
    shiftBookmarksOnInsert(m_cursorLine + 1, 1);
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
        m_bookmarks.clear();
        m_cursorLine = 0;
        m_cursorCol = 0;
    }
    else
    {
        shiftBookmarksOnDelete(line, 1);
        m_lines.removeAt(line);
        if (m_cursorLine >= m_lines.size())
            m_cursorLine = m_lines.size() - 1;
        m_cursorCol = qMin(m_cursorCol, m_lines[m_cursorLine].size());
    }
    m_modified = true;
    clearSelection();
}

void TuiBuffer::setBookmarks(const QList<int>& b)
{
    m_bookmarks = b;
    std::sort(m_bookmarks.begin(), m_bookmarks.end());
    m_bookmarks.erase(std::unique(m_bookmarks.begin(), m_bookmarks.end()), m_bookmarks.end());
    // Clamp to valid range
    for (int i = m_bookmarks.size() - 1; i >= 0; --i)
    {
        if (m_bookmarks[i] < 0 || m_bookmarks[i] >= m_lines.size())
            m_bookmarks.removeAt(i);
    }
}

void TuiBuffer::toggleBookmark(int line)
{
    if (m_bookmarks.contains(line))
        m_bookmarks.removeAll(line);
    else
    {
        m_bookmarks.append(line);
        std::sort(m_bookmarks.begin(), m_bookmarks.end());
    }
}

bool TuiBuffer::hasBookmark(int line) const
{
    return m_bookmarks.contains(line);
}

bool TuiBuffer::nextBookmark(int currentLine, int* outLine) const
{
    if (m_bookmarks.isEmpty())
        return false;
    QList<int> sorted = m_bookmarks;
    std::sort(sorted.begin(), sorted.end());
    for (int b : sorted)
    {
        if (b > currentLine)
        {
            if (outLine)
                *outLine = b;
            return true;
        }
    }
    // wrap
    if (outLine)
        *outLine = sorted.first();
    return true;
}

bool TuiBuffer::prevBookmark(int currentLine, int* outLine) const
{
    if (m_bookmarks.isEmpty())
        return false;
    QList<int> sorted = m_bookmarks;
    std::sort(sorted.begin(), sorted.end());
    for (int i = sorted.size() - 1; i >= 0; --i)
    {
        if (sorted[i] < currentLine)
        {
            if (outLine)
                *outLine = sorted[i];
            return true;
        }
    }
    if (outLine)
        *outLine = sorted.last();
    return true;
}

void TuiBuffer::clearBookmarks()
{
    m_bookmarks.clear();
}

void TuiBuffer::shiftBookmarksOnInsert(int atLine, int count)
{
    if (count <= 0 || m_bookmarks.isEmpty())
        return;
    for (int& b : m_bookmarks)
    {
        if (b >= atLine)
            b += count;
    }
}

void TuiBuffer::shiftBookmarksOnDelete(int atLine, int count)
{
    if (count <= 0 || m_bookmarks.isEmpty())
        return;
    QList<int> newMarks;
    for (int b : m_bookmarks)
    {
        if (b < atLine)
            newMarks.append(b);
        else if (b >= atLine + count)
            newMarks.append(b - count);
        // else b in deleted range -> drop
    }
    m_bookmarks = newMarks;
}

TuiBuffer::ReplaceResult TuiBuffer::replaceNext(const QString& find, const QString& replace, const SearchOptions& opts, bool wrap)
{
    if (m_readOnly || find.isEmpty())
        return {};
    QStringList copy = m_lines;
    SearchResult rr = TuiSearchEngine::replaceNext(copy, find, replace, opts, m_cursorLine, m_cursorCol, wrap);
    if (!rr.found)
        return {};
    pushUndo();
    m_lines = copy;
    m_cursorLine = rr.line;
    m_cursorCol = rr.column + rr.length;
    m_modified = true;
    clearSelection();
    ensureCursorValid();
    return {true, rr.line, rr.column, rr.length};
}

int TuiBuffer::replaceAll(const QString& find, const QString& replace, const SearchOptions& opts)
{
    if (m_readOnly || find.isEmpty())
        return 0;
    QStringList copy = m_lines;
    int count = TuiSearchEngine::replaceAll(copy, find, replace, opts);
    if (count == 0)
        return 0;
    pushUndo();
    m_lines = copy;
    m_modified = true;
    clearSelection();
    // Keep cursor valid
    ensureCursorValid();
    return count;
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
