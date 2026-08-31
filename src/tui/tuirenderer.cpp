/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuirenderer.h"

#include "tuibuffer.h"

#include <algorithm>

QList<WrapInfo> TuiRenderer::buildWrapInfos(const TuiBuffer* cur, int availGlobal, int* outCursorVisualRow)
{
    QList<WrapInfo> wrapInfos;
    int cursorVisualRow = -1;
    if (!cur || availGlobal <= 0)
    {
        if (outCursorVisualRow)
            *outCursorVisualRow = cursorVisualRow;
        return wrapInfos;
    }
    for (int i = 0; i < cur->lineCount(); ++i)
    {
        QString line = cur->lines().at(i);
        if (line.isEmpty())
        {
            wrapInfos.append({i, 0, 0});
            if (i == cur->cursorLine() && cur->cursorCol() == 0)
                cursorVisualRow = wrapInfos.size() - 1;
        }
        else
        {
            for (int off = 0; off < line.size(); off += availGlobal)
            {
                int len = qMin(availGlobal, line.size() - off);
                wrapInfos.append({i, off, len});
                if (i == cur->cursorLine() && cur->cursorCol() >= off && cur->cursorCol() < off + len)
                    cursorVisualRow = wrapInfos.size() - 1;
            }
            if (i == cur->cursorLine() && cur->cursorCol() == line.size())
            {
                for (int v = wrapInfos.size() - 1; v >= 0; --v)
                {
                    if (wrapInfos[v].line == i)
                    {
                        cursorVisualRow = v;
                        break;
                    }
                }
            }
        }
    }
    if (cursorVisualRow == -1 && !wrapInfos.isEmpty())
        cursorVisualRow = 0;
    if (outCursorVisualRow)
        *outCursorVisualRow = cursorVisualRow;
    return wrapInfos;
}

bool TuiRenderer::selectionForLine(const TuiBuffer* cur, int lineIdx, int startCol, int len, int* outStart, int* outEnd)
{
    if (!cur || !cur->hasSelection())
        return false;
    int aLine = cur->selectionAnchorLine();
    int aCol = cur->selectionAnchorCol();
    int cLine = cur->cursorLine();
    int cCol = cur->cursorCol();
    if (aLine > cLine || (aLine == cLine && aCol > cCol))
    {
        std::swap(aLine, cLine);
        std::swap(aCol, cCol);
    }
    if (lineIdx < aLine || lineIdx > cLine)
        return false;
    int selStart = -1;
    int selEnd = -1;
    if (aLine == cLine)
    {
        selStart = aCol - startCol;
        selEnd = cCol - startCol;
    }
    else if (lineIdx == aLine)
    {
        selStart = aCol - startCol;
        selEnd = len;
        if (selStart < 0)
            selStart = 0;
    }
    else if (lineIdx == cLine)
    {
        selStart = 0;
        selEnd = cCol - startCol;
    }
    else
    {
        selStart = 0;
        selEnd = len;
    }
    selStart = qBound(0, selStart, len);
    selEnd = qBound(0, selEnd, len);
    if (selStart == selEnd)
        return false;
    if (outStart)
        *outStart = selStart;
    if (outEnd)
        *outEnd = selEnd;
    return true;
}

bool TuiRenderer::searchForSegment(const SearchResult& lastSearch, int lineIdx, int segStart, int segLen, int* outStart, int* outLen)
{
    if (!lastSearch.found || lastSearch.line != lineIdx)
        return false;
    int s = lastSearch.column;
    int e = s + lastSearch.length;
    int segS = segStart;
    int segE = segStart + segLen;
    if (e <= segS || s >= segE)
        return false;
    if (outStart)
        *outStart = qMax(0, s - segS);
    if (outLen)
        *outLen = qMin(e, segE) - qMax(s, segS);
    return true;
}

#ifdef BUILD_TUI
#include <ncurses.h>

static void drawSyntaxVisible(int y, int x, const QString& visible, int offsetInLine, const QList<HighlightSegment>& segs, bool hasColors,
                              bool syntaxEnabled)
{
    if (!syntaxEnabled || !hasColors || segs.isEmpty())
    {
        mvaddnstr(y, x, visible.toUtf8().constData(), visible.toUtf8().size());
        return;
    }
    for (int i = 0; i < visible.size(); ++i)
    {
        int pos = offsetInLine + i;
        HighlightKind kind = HighlightKind::Normal;
        for (const auto& s : segs)
        {
            if (pos >= s.start && pos < s.start + s.length)
            {
                kind = s.kind;
                break;
            }
        }
        int pair = 0;
        switch (kind)
        {
        case HighlightKind::Keyword: pair = 4; break;
        case HighlightKind::String: pair = 5; break;
        case HighlightKind::Comment: pair = 6; break;
        case HighlightKind::Number: pair = 7; break;
        case HighlightKind::Preprocessor: pair = 8; break;
        default: pair = 0; break;
        }
        if (pair)
            attron(COLOR_PAIR(pair));
        QString chStr = visible.mid(i, 1);
        QByteArray utf = chStr.toUtf8();
        mvaddnstr(y, x + i, utf.constData(), utf.size());
        if (pair)
            attroff(COLOR_PAIR(pair));
    }
}

void TuiRenderer::drawTabBar(const TuiTabModel& tabs, int cols)
{
    attron(A_REVERSE);
    for (int c = 0; c < cols; ++c)
        mvaddch(0, c, ' ');
    int x = 0;
    for (int i = 0; i < tabs.count(); ++i)
    {
        const TuiBuffer* b = tabs.bufferAt(i);
        QString label = b->displayName();
        if (b->isModified())
            label += QStringLiteral("*");
        if (tabs.isPinned(i))
            label.prepend(QString::fromUtf8("\xF0\x9F\x93\x8C "));
        label = QStringLiteral(" %1 ").arg(label);
        if (x + label.size() + 1 >= cols)
            break;
        if (i == tabs.currentIndex())
            attron(A_BOLD);
        mvaddnstr(0, x, label.toUtf8().constData(), label.toUtf8().size());
        if (i == tabs.currentIndex())
            attroff(A_BOLD);
        x += label.size();
        mvaddch(0, x++, '|');
    }
    attroff(A_REVERSE);
}

void TuiRenderer::drawStatusBar(const TuiBuffer* cur, const TuiTabModel& tabs, const QString& statusMsg, const TuiFileTree& fileTree,
                                bool fileTreeVisible, bool fileTreeFocused, bool findMode, const QString& findInput, bool replaceMode,
                                int replacePhase, const QString& replaceFindInput, const QString& replaceInput, bool fileTreeFilterMode,
                                const QString& fileTreeFilterInput, bool saveAsMode, const QString& saveAsInput, bool gotoMode,
                                const QString& gotoInput, bool commandMode, const QString& commandInput, int cols, int rows, bool hasColors)
{
    int statusY = rows - 2;
    int msgY = rows - 1;
    if (hasColors)
        attron(COLOR_PAIR(1));
    else
        attron(A_REVERSE);
    for (int c = 0; c < cols; ++c)
    {
        mvaddch(statusY, c, ' ');
        mvaddch(msgY, c, ' ');
    }
    QString statusLeft = QStringLiteral(" %1  %2:%3  %4  %5 ")
                             .arg(cur && !cur->filePath().isEmpty() ? cur->filePath() : QStringLiteral("(untitled)"))
                             .arg(cur ? cur->cursorLine() + 1 : 1)
                             .arg(cur ? cur->cursorCol() + 1 : 1)
                             .arg(cur ? cur->encoding() : QStringLiteral("UTF-8"))
                             .arg(cur && cur->isModified() ? QStringLiteral("[+]") : QStringLiteral(""));
    QString statusRight = QStringLiteral(" Tab %1/%2 ").arg(tabs.currentIndex() + 1).arg(tabs.count());
    mvaddnstr(statusY, 0, statusLeft.toUtf8().constData(), qMin(statusLeft.toUtf8().size(), cols - statusRight.toUtf8().size() - 1));
    mvaddnstr(statusY, cols - statusRight.toUtf8().size(), statusRight.toUtf8().constData(), statusRight.toUtf8().size());
    QString msg;
    if (findMode)
        msg = QStringLiteral("Find: ") + findInput;
    else if (replaceMode)
        msg = (replacePhase == 0 ? QStringLiteral("Find: ") + replaceFindInput : QStringLiteral("Replace: ") + replaceInput);
    else if (fileTreeFilterMode)
        msg = QStringLiteral("Filter: ") + fileTreeFilterInput;
    else if (saveAsMode)
        msg = QStringLiteral("Save As: ") + saveAsInput;
    else if (gotoMode)
        msg = QStringLiteral("Go to line: ") + gotoInput;
    else if (commandMode)
        msg = QStringLiteral(":") + commandInput;
    else
        msg = statusMsg;
    if (fileTreeVisible && !findMode && !replaceMode && !fileTreeFilterMode && !saveAsMode && !gotoMode && !commandMode)
        msg = QStringLiteral("[Tree %1] ").arg(fileTreeFocused ? QStringLiteral("FOCUS") : QStringLiteral(" ")) + msg;
    mvaddnstr(msgY, 0, msg.toUtf8().constData(), qMin(msg.toUtf8().size(), cols));
    if (findMode)
        move(msgY, 6 + findInput.size());
    else if (replaceMode)
    {
        if (replacePhase == 0)
            move(msgY, 6 + replaceFindInput.size());
        else
            move(msgY, 9 + replaceInput.size());
    }
    else if (fileTreeFilterMode)
        move(msgY, 8 + fileTreeFilterInput.size());
    else if (saveAsMode)
        move(msgY, 9 + saveAsInput.size());
    else if (gotoMode)
        move(msgY, 12 + gotoInput.size());
    else if (commandMode)
        move(msgY, 1 + commandInput.size());
    if (hasColors)
        attroff(COLOR_PAIR(1));
    else
        attroff(A_REVERSE);
    Q_UNUSED(fileTree);
}

void TuiRenderer::drawEditor(TuiBuffer* cur, const TuiTabModel& tabs, TuiFileTree& fileTree, TuiViewState& view, int editorH, int cols,
                             int tabBarH, int treeWidth, bool fileTreeVisible, bool fileTreeFocused, bool hasColors,
                             const SearchResult& lastSearch)
{
    Q_UNUSED(tabs);
    const int editorXOffset = fileTreeVisible ? treeWidth + 1 : 0;
    const int textXGlobal = editorXOffset + 7;
    const int availGlobal = (textXGlobal < cols) ? cols - textXGlobal : 0;
    QList<TuiFileNode> treeNodes;
    if (fileTreeVisible)
    {
        treeNodes = fileTree.visibleNodes();
        view.clampFileTreeScroll(fileTree.cursorIndex(), editorH);
    }
    QList<WrapInfo> wrapInfos;
    int cursorVisualRow = -1;
    if (view.wordWrap && availGlobal > 0)
    {
        wrapInfos = buildWrapInfos(cur, availGlobal, &cursorVisualRow);
        view.ensureCursorVisibleWrap(cursorVisualRow, editorH, wrapInfos.size());
    }
    else
    {
        view.ensureCursorVisibleNoWrap(cur, editorH);
    }

    for (int r = 0; r < editorH; ++r)
    {
        int lineIdx = view.scrollTop + r;
        int y = tabBarH + r;
        for (int c = 0; c < cols; ++c)
            mvaddch(y, c, ' ');

        if (fileTreeVisible)
        {
            int treeRow = view.fileTreeScroll + r;
            if (treeRow < treeNodes.size())
            {
                const TuiFileNode& node = treeNodes[treeRow];
                bool isSelected = (treeRow == fileTree.cursorIndex());
                if (isSelected)
                    attron(A_REVERSE);
                if (fileTreeFocused && isSelected)
                    attron(A_BOLD);
                QString indent = QString(node.depth * 2, QLatin1Char(' '));
                QString prefix = node.isDir ? (node.expanded ? QStringLiteral("- ") : QStringLiteral("+ ")) : QStringLiteral("  ");
                QString label = indent + prefix + node.name + (node.isDir ? QStringLiteral("/") : QString());
                mvaddnstr(y, 0, label.toUtf8().constData(), qMin(label.toUtf8().size(), treeWidth));
                if (isSelected)
                    attroff(A_REVERSE);
                if (fileTreeFocused && isSelected)
                    attroff(A_BOLD);
            }
            mvaddch(y, treeWidth, ACS_VLINE);
        }

        if (view.wordWrap && availGlobal > 0)
        {
            int vIdx = view.wrapScrollTop + r;
            if (vIdx < 0 || vIdx >= wrapInfos.size())
                continue;
            WrapInfo wi = wrapInfos[vIdx];
            int wLineIdx = wi.line;
            QString wLine = cur->lines().at(wLineIdx);
            bool isFirstSeg = (wi.startCol == 0);
            bool isBm = isFirstSeg && cur->hasBookmark(wLineIdx);
            QString gutter = (hasColors && isBm) ? QString::fromUtf8("\xE2\x98\x85 ") : QStringLiteral("  ");
            QString lnStr = isFirstSeg ? QStringLiteral("%1 ").arg(wLineIdx + 1, 4) : QStringLiteral("     ");
            if (hasColors && isFirstSeg)
                attron(COLOR_PAIR(2));
            mvaddnstr(y, editorXOffset, lnStr.toUtf8().constData(), qMin(lnStr.toUtf8().size(), cols - editorXOffset));
            if (hasColors && isFirstSeg)
                attroff(COLOR_PAIR(2));
            int gutterX = editorXOffset + 5;
            if (hasColors && isBm)
                attron(COLOR_PAIR(3));
            mvaddnstr(y, gutterX, gutter.toUtf8().constData(), gutter.size());
            if (hasColors && isBm)
                attroff(COLOR_PAIR(3));
            int textX = editorXOffset + 7;
            QString visible = wLine.mid(wi.startCol, wi.len);
            int selStart = -1, selEnd = -1;
            bool hasSel = selectionForLine(cur, wLineIdx, wi.startCol, wi.len, &selStart, &selEnd);
            bool searchInSeg = false;
            int searchStart = -1, searchLen = 0;
            searchInSeg = searchForSegment(lastSearch, wLineIdx, wi.startCol, wi.len, &searchStart, &searchLen);
            if (hasSel)
            {
                mvaddnstr(y, textX, visible.left(selStart).toUtf8().constData(), selStart);
                attron(A_REVERSE);
                mvaddnstr(y, textX + selStart, visible.mid(selStart, selEnd - selStart).toUtf8().constData(), selEnd - selStart);
                attroff(A_REVERSE);
                if (selEnd < visible.size())
                    mvaddnstr(y, textX + selEnd, visible.mid(selEnd).toUtf8().constData(), visible.size() - selEnd);
            }
            else if (searchInSeg)
            {
                mvaddnstr(y, textX, visible.left(searchStart).toUtf8().constData(), searchStart);
                attron(A_REVERSE);
                mvaddnstr(y, textX + searchStart, visible.mid(searchStart, searchLen).toUtf8().constData(), searchLen);
                attroff(A_REVERSE);
                if (searchStart + searchLen < visible.size())
                    mvaddnstr(y, textX + searchStart + searchLen, visible.mid(searchStart + searchLen).toUtf8().constData(),
                              visible.size() - searchStart - searchLen);
            }
            else
            {
                auto segs = TuiHighlighter::highlightLine(wLine, TuiHighlighter::currentLanguage(cur->filePath()));
                drawSyntaxVisible(y, textX, visible, wi.startCol, segs, hasColors, view.syntaxEnabled);
            }
            if (vIdx == cursorVisualRow && !fileTreeFocused)
            {
                int curX = textX + (cur->cursorCol() - wi.startCol);
                if (curX >= textX && curX < cols)
                    move(y, curX);
            }
            continue;
        }

        if (lineIdx >= cur->lineCount())
        {
            if (lineIdx == cur->cursorLine() && !fileTreeFocused)
            {
                int curX = editorXOffset + 7 + (cur->cursorCol());
                if (curX >= editorXOffset && curX < cols)
                    move(y, curX);
            }
            continue;
        }

        QString line = cur->lines().at(lineIdx);
        bool isBm = cur->hasBookmark(lineIdx);
        QString gutter;
        if (hasColors && isBm)
            gutter = QString::fromUtf8("\xE2\x98\x85 ");
        else
            gutter = QStringLiteral("  ");

        QString lnStr = QStringLiteral("%1 ").arg(lineIdx + 1, 4);
        if (hasColors)
            attron(COLOR_PAIR(2));
        mvaddnstr(y, editorXOffset, lnStr.toUtf8().constData(), qMin(lnStr.toUtf8().size(), cols - editorXOffset));
        if (hasColors)
            attroff(COLOR_PAIR(2));

        int gutterX = editorXOffset + 5;
        if (hasColors && isBm)
            attron(COLOR_PAIR(3));
        mvaddnstr(y, gutterX, gutter.toUtf8().constData(), gutter.size());
        if (hasColors && isBm)
            attroff(COLOR_PAIR(3));

        int textX = gutterX + 2;
        int avail = cols - textX;
        if (avail <= 0)
            continue;

        int hScroll = 0;
        if (cur->cursorLine() == lineIdx && cur->cursorCol() >= avail)
            hScroll = cur->cursorCol() - avail + 1;

        QString visible = line.mid(hScroll, avail);

        int selStart = -1;
        int selEnd = -1;
        bool hasSel = false;
        if (cur->hasSelection())
        {
            int aLine = cur->selectionAnchorLine();
            int aCol = cur->selectionAnchorCol();
            int cLine = cur->cursorLine();
            int cCol = cur->cursorCol();
            if (aLine > cLine || (aLine == cLine && aCol > cCol))
            {
                std::swap(aLine, cLine);
                std::swap(aCol, cCol);
            }
            if (lineIdx >= aLine && lineIdx <= cLine)
            {
                if (aLine == cLine)
                {
                    selStart = aCol - hScroll;
                    selEnd = cCol - hScroll;
                }
                else if (lineIdx == aLine)
                {
                    selStart = aCol - hScroll;
                    selEnd = avail;
                }
                else if (lineIdx == cLine)
                {
                    selStart = 0 - hScroll;
                    if (selStart < 0)
                        selStart = 0;
                    selEnd = cCol - hScroll;
                }
                else
                {
                    selStart = 0;
                    selEnd = avail;
                }
                selStart = qBound(0, selStart, avail);
                selEnd = qBound(0, selEnd, avail);
                if (selStart == selEnd)
                {
                    selStart = -1;
                    selEnd = -1;
                }
                else
                    hasSel = true;
            }
        }

        if (lastSearch.found && lastSearch.line == lineIdx && !hasSel)
        {
            int start = lastSearch.column - hScroll;
            int len = lastSearch.length;
            if (start < 0)
            {
                len += start;
                start = 0;
            }
            if (start < avail && len > 0)
            {
                mvaddnstr(y, textX, visible.left(start).toUtf8().constData(), start);
                attron(A_REVERSE);
                mvaddnstr(y, textX + start, visible.mid(start, len).toUtf8().constData(), len);
                attroff(A_REVERSE);
                if (start + len < visible.size())
                    mvaddnstr(y, textX + start + len, visible.mid(start + len).toUtf8().constData(), visible.size() - start - len);
            }
            else
            {
                mvaddnstr(y, textX, visible.toUtf8().constData(), visible.toUtf8().size());
            }
        }
        else if (hasSel)
        {
            mvaddnstr(y, textX, visible.left(selStart).toUtf8().constData(), selStart);
            attron(A_REVERSE);
            mvaddnstr(y, textX + selStart, visible.mid(selStart, selEnd - selStart).toUtf8().constData(), selEnd - selStart);
            attroff(A_REVERSE);
            if (selEnd < visible.size())
                mvaddnstr(y, textX + selEnd, visible.mid(selEnd).toUtf8().constData(), visible.size() - selEnd);
        }
        else
        {
            auto segs = TuiHighlighter::highlightLine(line, TuiHighlighter::currentLanguage(cur->filePath()));
            drawSyntaxVisible(y, textX, visible, hScroll, segs, hasColors, view.syntaxEnabled);
        }

        if (lineIdx == cur->cursorLine())
        {
            int curX = textX + (cur->cursorCol() - hScroll);
            if (curX >= textX && curX < cols)
                move(y, curX);
        }
    }
}
#endif
