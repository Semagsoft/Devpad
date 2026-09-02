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

SearchResult TuiSearchEngine::findNext(const QStringList& lines, const QString& query, const SearchOptions& opts, int startLine, int startCol,
                                       bool forward, bool wrap)
{
    if (query.isEmpty() || lines.isEmpty())
        return {};

    QRegularExpression re = buildPattern(query, opts);
    if (!re.isValid())
        return {};

    auto searchInLine = [&](int lineIdx, int fromCol, bool isFirstLine) -> SearchResult
    {
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
        QRegularExpressionMatch m = re.match(line, offset);
        if (m.hasMatch())
        {
            return {true, lineIdx, static_cast<int>(m.capturedStart()), static_cast<int>(m.capturedLength())};
        }
        return {};
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

QString TuiSearchEngine::expandReplace(const QRegularExpressionMatch& m, const QString& replaceTemplate, const SearchOptions& opts)
{
    if (!opts.regex)
        return replaceTemplate;
    QString out;
    for (int i = 0; i < replaceTemplate.size(); ++i)
    {
        QChar c = replaceTemplate[i];
        if (c == QLatin1Char('\\') && i + 1 < replaceTemplate.size())
        {
            QChar nxt = replaceTemplate[i + 1];
            if (nxt.isDigit())
            {
                int cap = nxt.digitValue();
                if (cap >= 0 && cap <= m.lastCapturedIndex())
                {
                    out += m.captured(cap);
                    ++i;
                    continue;
                }
            }
            // Escaped backslash or unknown: keep next char literally
            out += nxt;
            ++i;
        }
        else
        {
            out += c;
        }
    }
    return out;
}

SearchResult TuiSearchEngine::replaceNext(QStringList& lines, const QString& find, const QString& replace, const SearchOptions& opts, int startLine,
                                          int startCol, bool wrap)
{
    if (find.isEmpty() || lines.isEmpty())
        return {};
    SearchResult r = findNext(lines, find, opts, startLine, startCol, true, wrap);
    if (!r.found)
        return {};
    QRegularExpression re = buildPattern(find, opts);
    QString& line = lines[r.line];
    QRegularExpressionMatch m = re.match(line, r.column);
    if (!m.hasMatch())
        return {};
    QString expanded = expandReplace(m, replace, opts);
    line.replace(m.capturedStart(), m.capturedLength(), expanded);
    return {true, r.line, r.column, static_cast<int>(expanded.size())};
}

int TuiSearchEngine::replaceAll(QStringList& lines, const QString& find, const QString& replace, const SearchOptions& opts)
{
    if (find.isEmpty() || lines.isEmpty())
        return 0;
    QRegularExpression re = buildPattern(find, opts);
    if (!re.isValid())
        return 0;
    int count = 0;
    for (int l = 0; l < lines.size(); ++l)
    {
        QString& line = lines[l];
        // For each line, replace all non-overlapping occurrences iteratively
        int offset = 0;
        while (offset <= line.size())
        {
            QRegularExpressionMatch m = re.match(line, offset);
            if (!m.hasMatch())
                break;
            // Zero-length match guard to avoid infinite loop (e.g., regex ".*")
            if (m.capturedLength() == 0)
            {
                if (offset >= line.size())
                    break;
                ++offset;
                continue;
            }
            QString expanded = expandReplace(m, replace, opts);
            line.replace(m.capturedStart(), m.capturedLength(), expanded);
            offset = m.capturedStart() + expanded.size();
            ++count;
            // If replacement is empty, advance at least 1 to avoid re-matching same position
            if (expanded.isEmpty() && count > 0 && offset == m.capturedStart())
                ++offset;
        }
    }
    return count;
}
