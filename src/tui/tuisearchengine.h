/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#ifndef TUISEARCHENGINE_H
#define TUISEARCHENGINE_H

#include <QRegularExpression>
#include <QString>

struct SearchOptions
{
    bool caseSensitive = false;
    bool wholeWords = false;
    bool regex = false;
};

struct SearchResult
{
    bool found = false;
    int line = -1;
    int column = -1;
    int length = 0;
};

class TuiSearchEngine
{
public:
    static SearchResult findNext(const QStringList& lines, const QString& query, const SearchOptions& opts, int startLine, int startCol,
                                 bool forward = true, bool wrap = true);
    static QList<SearchResult> findAll(const QStringList& lines, const QString& query, const SearchOptions& opts);

private:
    static QRegularExpression buildPattern(const QString& query, const SearchOptions& opts);
};

#endif // TUISEARCHENGINE_H
