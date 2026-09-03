/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#ifndef TUIFINDINFILES_H
#define TUIFINDINFILES_H

#include "tuisearchengine.h"

#include <QList>
#include <QString>
#include <QStringList>

struct FindInFilesResult
{
    QString filePath;
    int line = -1; // 0-based
    int column = -1;
    int length = 0;
    QString lineText;
};

class TuiFindInFiles
{
public:
    static QList<FindInFilesResult> search(const QString& rootPath, const QString& pattern, const SearchOptions& opts,
                                           const QString& fileGlob = QString(), const QStringList& excludeDirs = QStringList());

private:
    static bool matchesGlob(const QString& fileName, const QString& glob);
    static QStringList parseGlob(const QString& glob);
};

#endif // TUIFINDINFILES_H
