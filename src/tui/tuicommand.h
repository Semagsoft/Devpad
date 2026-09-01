/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#ifndef TUICOMMAND_H
#define TUICOMMAND_H

#include "tuibuffer.h"
#include "tuifiletree.h"
#include "tuitabmodel.h"
#include "tuiviewstate.h"

#include <QDateTime>
#include <QHash>

class TuiCommand
{
public:
    enum class Result
    {
        Continue,
        Quit
    };

    // Dispatch a single : command string (without leading ':'). Returns Quit if app should exit.
    // May mutate tabs/cur/fileTree/view/statusMsg/lastSearch/findOpts/fileMtimes.
    // Flags for saveAsMode/commandMode handling are output via outSaveAsRequest.
    static Result dispatch(const QString& rawCmd, TuiTabModel& tabs, TuiBuffer* cur, TuiFileTree& fileTree, TuiViewState& view, QString& statusMsg,
                           SearchResult& lastSearch, SearchOptions& findOpts, QHash<QString, QDateTime>& fileMtimes, bool& outSaveAsRequest);

    // Tab completion for command input. Mutates commandInput and statusMsg.
    static void complete(QString& commandInput, QString& statusMsg);
};

#endif // TUICOMMAND_H
