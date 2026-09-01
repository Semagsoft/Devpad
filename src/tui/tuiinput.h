/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#ifndef TUIINPUT_H
#define TUIINPUT_H

#include "tuibuffer.h"
#include "tuifiletree.h"
#include "tuisearchengine.h"
#include "tuitabmodel.h"
#include "tuiviewstate.h"

#include <QDateTime>
#include <QHash>
#include <QString>

// Holds all transient input-mode state (extracted from tuiapp.cpp locals)
struct TuiInputState
{
    // find
    QString findQuery;
    SearchOptions findOpts;
    SearchResult lastSearch;
    bool findMode = false;
    QString findInput;

    // replace
    bool replaceMode = false;
    int replacePhase = 0; // 0=find, 1=replace
    QString replaceFindInput;
    QString replaceInput;

    // saveAs/goto/command
    bool saveAsMode = false;
    QString saveAsInput;
    bool gotoMode = false;
    QString gotoInput;
    bool commandMode = false;
    QString commandInput;

    // file tree
    bool fileTreeFilterMode = false;
    QString fileTreeFilterInput;

    // other
    QString statusMsg = QStringLiteral("Ctrl+Q quit  Ctrl+S save  Ctrl+E tree  Ctrl+F find  F1 help");
    QString clipboard;
    QHash<QString, QDateTime> fileMtimes;
    bool fileTreeVisible = false;
    bool fileTreeFocused = false;

    TuiViewState view;
    bool wordWrapInitDone = false;
};

class TuiInput
{
public:
    // Handle ch when in any input mode (find/replace/saveAs/goto/command/filter). Returns true if handled (caller should continue loop).
    static bool handleInputModes(int ch, TuiTabModel& tabs, TuiBuffer* cur, TuiFileTree& fileTree, TuiInputState& st,
                                 QHash<QString, QDateTime>& fileMtimes);

    // Handle file-tree focused navigation. Returns true if handled.
    static bool handleFileTreeInput(int ch, TuiFileTree& fileTree, TuiInputState& st, TuiTabModel& tabs, QHash<QString, QDateTime>& fileMtimes);

    // Handle global shortcuts + editing. Returns true if should quit app.
    static bool handleGlobalInput(int ch, TuiTabModel& tabs, TuiBuffer* cur, TuiFileTree& fileTree, TuiInputState& st,
                                  QHash<QString, QDateTime>& fileMtimes, int editorH);

    // Named key constants for readability (was magic numbers)
    enum Key
    {
        CtrlQ = 17,
        CtrlS = 19,
        CtrlE = 5,
        CtrlK = 11,
        CtrlO = 15,
        CtrlG = 7,
        CtrlH = 8,
        CtrlZ = 26,
        CtrlY = 25,
        CtrlR = 18,
        CtrlA = 1,
        CtrlC = 3,
        CtrlX = 24,
        CtrlV = 22,
        CtrlW = 23,
        CtrlN = 14,
        CtrlF = 6,
        CtrlB = 2,
        Colon = 58,
        TabKey = 9
    };
};

#endif // TUIINPUT_H
