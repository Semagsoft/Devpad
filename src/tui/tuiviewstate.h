/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#ifndef TUIVIEWSTATE_H
#define TUIVIEWSTATE_H

#include <QList>

class TuiBuffer;

struct WrapInfo
{
    int line = 0;
    int startCol = 0;
    int len = 0;
};

struct TuiViewState
{
    int scrollTop = 0;
    int wrapScrollTop = 0;
    int fileTreeScroll = 0;
    bool wordWrap = false;
    bool syntaxEnabled = true;

    // Non-wrap: keep cursor line visible
    void ensureCursorVisibleNoWrap(const TuiBuffer* cur, int editorH);

    // Wrap: buildInfos needed to clamp wrapScrollTop
    void ensureCursorVisibleWrap(int cursorVisualRow, int editorH, int totalWrapLines);

    void clampFileTreeScroll(int cursorIndex, int editorH);
};

#endif // TUIVIEWSTATE_H
