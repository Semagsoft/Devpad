/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuisearchengine.h"

QRegularExpression TuiSearchEngine::buildPattern(const QString& query, const SearchOptions& opts)
{
    QString pattern = query;
    if (!opts.regex)
        pattern = QRegularExpression::escape(pattern);
    if (opts.wholeWords)
        pattern = QStringLiteral("\\b") + pattern + QStringLiteral("\\b");
    QRegularExpression::PatternOptions flags = QRegularExpression::NoPatternOption;
    if (!opts.caseSensitive)
        flags |= QRegularExpression::CaseInsensitiveOption;
    return QRegularExpression(pattern, flags);
}

SearchResult TuiSearchEngine::findNext(const QStringList& lines, const QString& query, const SearchOptions& opts, int startLine, int startCol, bool forward,
                                       bool wrap)
{
    if (query.isEmpty() || lines.isEmpty())
        return {};

    QRegularExpression re = buildPattern(query, opts);
    if (!re.isValid())
        return {};

    auto searchInLine = [&](int lineIdx, int fromCol, bool isFirstLine) -> SearchResult {
        const QString& line = lines[lineIdx];
        int offset = isFirstLine ? fromCol : 0;
        if (!forward)
        {
            // For backward, search all matches before offset
            QRegularExpressionMatchIterator it = re.globalMatch(line);
            SearchResult best;
            while (it.hasNext())
            {
                QRegularExpressionMatch m = it.next();
                int col = m.capturedStart();
                int len = m.capturedLength();
                if (isFirstLine && col + len > fromCol)
                    continue;
                if (col + len <= offset || !isFirstLine || col < fromCol)
                {
                    if (!best.found || col > best.column)
                    {
                        best.found = true;
                        best.line = lineIdx;
                        best.column = col;
                        best.length = len;
                    }
                }
            }
            return best;
        }
        else
        {
            QRegularExpressionMatch m = re.match(line, offset);
            if (m.hasMatch())
            {
                return {true, lineIdx, static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength())};
            }
            return {};
        }
    };

    if (forward)
    {
        for (int l = startLine; l < lines.size(); ++l)
        {
            int fromCol = (l == startLine) ? startCol : 0;
            SearchResult r = searchInLine(l, fromCol, l == startLine);
            if (r.found)
                return r;
        }
        if (wrap)
        {
            for (int l = 0; l <= startLine; ++l)
            {
                int fromCol = 0;
                int endCol = (l == startLine) ? startCol : lines[l].size() + 1;
                // Avoid re-finding same match at start position: for first wrap line we search from 0
                Q_UNUSED(endCol);
                SearchResult r = searchInLine(l, fromCol, false);
                if (r.found)
                {
                    // Ensure we don't return a match beyond start position on the start line when wrapping
                    if (l == startLine && r.column >= startCol)
                        continue;
                    return r;
                }
            }
        }
    }
    else
    {
        for (int l = startLine; l >= 0; --l)
        {
            int fromCol = (l == startLine) ? startCol : lines[l].size();
            SearchResult r = searchInLine(l, fromCol, l == startLine);
            if (r.found)
                return r;
        }
        if (wrap)
        {
            for (int l = lines.size() - 1; l >= startLine; --l)
            {
                SearchResult r = searchInLine(l, lines[l].size(), false);
                if (r.found)
                {
                    if (l == startLine && r.column >= startCol)
                        continue;
                    return r;
                }
            }
        }
    }
    return {};
}

QList<SearchResult> TuiSearchEngine::findAll(const QStringList& lines, const QString& query, const SearchOptions& opts)
{
    QList<SearchResult> results;
    if (query.isEmpty())
        return results;
    QRegularExpression re = buildPattern(query, opts);
    if (!re.isValid())
        return results;
    for (int l = 0; l < lines.size(); ++l)
    {
        QRegularExpressionMatchIterator it = re.globalMatch(lines[l]);
        while (it.hasNext())
        {
            QRegularExpressionMatch m = it.next();
            results.append({true, l, static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength())});
        }
    }
    return results;
}
