/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuiviewstate.h"

#include "tuibuffer.h"

void TuiViewState::ensureCursorVisibleNoWrap(const TuiBuffer* cur, int editorH)
{
    if (!cur)
        return;
    if (cur->cursorLine() < scrollTop)
        scrollTop = cur->cursorLine();
    if (cur->cursorLine() >= scrollTop + editorH)
        scrollTop = cur->cursorLine() - editorH + 1;
}

void TuiViewState::ensureCursorVisibleWrap(int cursorVisualRow, int editorH, int totalWrapLines)
{
    if (cursorVisualRow < wrapScrollTop)
        wrapScrollTop = cursorVisualRow;
    if (cursorVisualRow >= wrapScrollTop + editorH)
        wrapScrollTop = cursorVisualRow - editorH + 1;
    if (wrapScrollTop < 0)
        wrapScrollTop = 0;
    if (wrapScrollTop >= totalWrapLines)
        wrapScrollTop = qMax(0, totalWrapLines - editorH);
}

void TuiViewState::clampFileTreeScroll(int cursorIndex, int editorH)
{
    if (cursorIndex < fileTreeScroll)
        fileTreeScroll = cursorIndex;
    if (cursorIndex >= fileTreeScroll + editorH)
        fileTreeScroll = cursorIndex - editorH + 1;
    if (fileTreeScroll < 0)
        fileTreeScroll = 0;
}
