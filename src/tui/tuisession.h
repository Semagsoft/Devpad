/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#ifndef TUISESSION_H
#define TUISESSION_H

#include "tuifiletree.h"
#include "tuitabmodel.h"

#include <QDateTime>
#include <QHash>
#include <QString>

class QCommandLineParser;

class TuiSession
{
public:
    // Restore session into tabs/fileTree if applicable. Returns true if restored.
    static bool tryRestore(const QCommandLineParser& parser, const QStringList& positionalFiles, TuiTabModel& tabs, TuiFileTree& fileTree);

    static void save(const QCommandLineParser& parser, const TuiTabModel& tabs, const TuiFileTree& fileTree);

    // Poll external changes for all buffers; updates fileMtimes and statusMsg/cur as needed.
    static void pollExternalChanges(TuiTabModel& tabs, TuiBuffer* cur, QHash<QString, QDateTime>& fileMtimes, QString& statusMsg);

    // Poll directory changes for fileTree (throttled)
    static void pollFileTree(TuiFileTree& fileTree, qint64& lastPollMs, QDateTime& lastDirMTime, QString& statusMsg);
};

#endif // TUISESSION_H
