/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#ifndef TUIRENDERER_H
#define TUIRENDERER_H

#include "tuifiletree.h"
#include "tuihighlighter.h"
#include "tuisearchengine.h"
#include "tuitabmodel.h"
#include "tuiviewstate.h"

#include <QHash>
#include <QString>

class TuiRenderer
{
public:
    // Pure helpers (testable without ncurses)
    static QList<WrapInfo> buildWrapInfos(const TuiBuffer* cur, int availGlobal, int* outCursorVisualRow = nullptr);
    // Returns selection range for a given line in columns relative to visible segment.
    // Returns true if selection overlaps this segment; outStart/outEnd are clamped to [0,len].
    static bool selectionForLine(const TuiBuffer* cur, int lineIdx, int startCol, int len, int* outStart, int* outEnd);
    static bool searchForSegment(const SearchResult& lastSearch, int lineIdx, int segStart, int segLen, int* outStart, int* outLen);

    // ncurses drawing (requires initscr). Kept separate so logic is testable.
#ifdef BUILD_TUI
    static void drawTabBar(const TuiTabModel& tabs, int cols);
    static void drawStatusBar(const TuiBuffer* cur, const TuiTabModel& tabs, const QString& statusMsg, const TuiFileTree& fileTree,
                              bool fileTreeVisible, bool fileTreeFocused, bool findMode, const QString& findInput, bool replaceMode,
                              int replacePhase, const QString& replaceFindInput, const QString& replaceInput, bool fileTreeFilterMode,
                              const QString& fileTreeFilterInput, bool saveAsMode, const QString& saveAsInput, bool gotoMode,
                              const QString& gotoInput, bool commandMode, const QString& commandInput, int cols, int rows, bool hasColors);
    static void drawEditor(TuiBuffer* cur, const TuiTabModel& tabs, TuiFileTree& fileTree, TuiViewState& view, int editorH, int cols,
                           int tabBarH, int treeWidth, bool fileTreeVisible, bool fileTreeFocused, bool hasColors,
                           const SearchResult& lastSearch);
#endif
};

#endif // TUIRENDERER_H
