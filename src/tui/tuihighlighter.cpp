/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuihighlighter.h"

#include "keywords.h"

#include <QFileInfo>
#include <QRegularExpression>

QHash<QString, QStringList> TuiHighlighter::s_keywordCache;
bool TuiHighlighter::s_enabled = true;
QString TuiHighlighter::s_override;

QString TuiHighlighter::languageForFile(const QString& filePath)
{
    if (filePath.isEmpty())
        return QString();
    QString ext = QFileInfo(filePath).suffix().toLower();
    QString base = QFileInfo(filePath).fileName().toLower();
    // Direct extension mapping (simplified, mirrors SettingsManager extensionMap)
    static const QHash<QString, QString> extMap = {
        {QStringLiteral("cpp"), QStringLiteral("cpp")},
        {QStringLiteral("cc"), QStringLiteral("cpp")},
        {QStringLiteral("cxx"), QStringLiteral("cpp")},
        {QStringLiteral("h"), QStringLiteral("cpp")},
        {QStringLiteral("hpp"), QStringLiteral("cpp")},
        {QStringLiteral("c"), QStringLiteral("cpp")},
        {QStringLiteral("cs"), QStringLiteral("csharp")},
        {QStringLiteral("java"), QStringLiteral("java")},
        {QStringLiteral("py"), QStringLiteral("python")},
        {QStringLiteral("js"), QStringLiteral("javascript")},
        {QStringLiteral("ts"), QStringLiteral("typescript")},
        {QStringLiteral("html"), QStringLiteral("html")},
        {QStringLiteral("htm"), QStringLiteral("html")},
        {QStringLiteral("css"), QStringLiteral("css")},
        {QStringLiteral("xml"), QStringLiteral("xml")},
        {QStringLiteral("sql"), QStringLiteral("sql")},
        {QStringLiteral("sh"), QStringLiteral("bash")},
        {QStringLiteral("bash"), QStringLiteral("bash")},
        {QStringLiteral("cmake"), QStringLiteral("cmake")},
        {QStringLiteral("md"), QStringLiteral("markdown")},
        {QStringLiteral("json"), QStringLiteral("json")},
        {QStringLiteral("qml"), QStringLiteral("qml")},
        {QStringLiteral("lua"), QStringLiteral("lua")},
        {QStringLiteral("rs"), QStringLiteral("rust")},
        {QStringLiteral("go"), QStringLiteral("go")},
    };
    auto it = extMap.find(ext);
    if (it != extMap.end())
        return it.value();
    if (base == QStringLiteral("CMakeLists.txt"))
        return QStringLiteral("cmake");
    if (base == QStringLiteral("makefile") || base == QStringLiteral("Makefile"))
        return QStringLiteral("bash");
    return QString();
}

QString TuiHighlighter::currentLanguage(const QString& filePath)
{
    if (!s_override.isEmpty())
        return s_override;
    return languageForFile(filePath);
}

const QStringList& TuiHighlighter::keywordsForLang(const QString& lang)
{
    auto it = s_keywordCache.find(lang);
    if (it != s_keywordCache.end())
        return it.value();

    const QStringList* src = nullptr;
    if (lang == QStringLiteral("cpp")) src = &cppKeywords();
    else if (lang == QStringLiteral("csharp")) src = &csharpKeywords();
    else if (lang == QStringLiteral("java")) src = &javaKeywords();
    else if (lang == QStringLiteral("python")) src = &pythonKeywords();
    else if (lang == QStringLiteral("javascript")) src = &javascriptKeywords();
    else if (lang == QStringLiteral("html")) src = &htmlKeywords();
    else if (lang == QStringLiteral("css")) src = &cssKeywords();
    else if (lang == QStringLiteral("xml")) src = &xmlKeywords();
    else if (lang == QStringLiteral("sql")) src = &sqlKeywords();
    else if (lang == QStringLiteral("typescript")) src = &typescriptKeywords();
    else if (lang == QStringLiteral("rust")) src = &rustKeywords();
    else if (lang == QStringLiteral("go")) src = &goKeywords();
    else if (lang == QStringLiteral("markdown")) src = &markdownKeywords();
    else if (lang == QStringLiteral("json")) src = &jsonKeywords();
    else if (lang == QStringLiteral("bash")) src = &bashKeywords();
    else if (lang == QStringLiteral("cmake")) src = &cmakeKeywords();
    else if (lang == QStringLiteral("lua")) src = &luaKeywords();
    else if (lang == QStringLiteral("qml")) src = &qmlKeywords();
    else
    {
        static const QStringList empty;
        s_keywordCache.insert(lang, empty);
        return s_keywordCache[lang];
    }
    s_keywordCache.insert(lang, *src);
    return s_keywordCache[lang];
}

QList<HighlightSegment> TuiHighlighter::highlightLine(const QString& line, const QString& language)
{
    QList<HighlightSegment> segs;
    if (!s_enabled || language.isEmpty() || line.isEmpty())
        return segs;

    // Very simple state: detect string, comment, number, keyword
    // This is line-local, not handling multi-line block comments accurately, but good enough for MVP.

    const QStringList& kws = keywordsForLang(language);
    QHash<QString, bool> kwSet;
    if (!kws.isEmpty())
    {
        for (const QString& k : kws)
            kwSet.insert(k, true);
    }

    int n = line.size();

    // Preprocess: find string and comment ranges
    // Track string delims: " and ' (and for python """ but we simplify)
    // We'll first mark string/comment ranges, then keywords/numbers outside those.

    // Simple scan for strings and comments
    QList<QPair<int,int>> stringRanges;
    QList<QPair<int,int>> commentRanges;

    bool inString = false;
    QChar stringDelim;
    int stringStart = -1;
    for (int p = 0; p < n; ++p)
    {
        QChar c = line[p];
        if (!inString)
        {
            // Check line comment start: // or # (depending on language)
            if (language == QStringLiteral("python") || language == QStringLiteral("bash") || language == QStringLiteral("cmake"))
            {
                if (c == QLatin1Char('#'))
                {
                    commentRanges.append(qMakePair(p, n - p));
                    break; // rest is comment
                }
            }
            if (p + 1 < n && c == QLatin1Char('/') && line[p+1] == QLatin1Char('/'))
            {
                commentRanges.append(qMakePair(p, n - p));
                break;
            }
            if (p + 1 < n && c == QLatin1Char('/') && line[p+1] == QLatin1Char('*'))
            {
                // Block comment start, find end on same line
                int end = line.indexOf(QStringLiteral("*/"), p+2);
                if (end != -1)
                {
                    commentRanges.append(qMakePair(p, end + 2 - p));
                    p = end + 1;
                    continue;
                }
                else
                {
                    commentRanges.append(qMakePair(p, n - p));
                    break;
                }
            }
            if (c == QLatin1Char('"') || c == QLatin1Char('\''))
            {
                // Check if it's start of string (not escaped)
                bool escaped = (p > 0 && line[p-1] == QLatin1Char('\\'));
                if (!escaped)
                {
                    inString = true;
                    stringDelim = c;
                    stringStart = p;
                }
            }
        }
        else
        {
            if (c == stringDelim)
            {
                bool escaped = (p > 0 && line[p-1] == QLatin1Char('\\'));
                if (!escaped)
                {
                    stringRanges.append(qMakePair(stringStart, p - stringStart + 1));
                    inString = false;
                    stringStart = -1;
                }
            }
        }
    }
    if (inString && stringStart != -1)
        stringRanges.append(qMakePair(stringStart, n - stringStart));

    auto isInRanges = [&](int pos, const QList<QPair<int,int>>& ranges) -> bool {
        for (auto &pr : ranges)
            if (pos >= pr.first && pos < pr.first + pr.second)
                return true;
        return false;
    };

    // Add string segments
    for (auto &pr : stringRanges)
        segs.append({pr.first, pr.second, HighlightKind::String});
    for (auto &pr : commentRanges)
        segs.append({pr.first, pr.second, HighlightKind::Comment});

    // For remaining positions, detect keywords and numbers
    // Tokenize by word boundaries
    QRegularExpression wordRe(QStringLiteral("\\b\\w+\\b"));
    auto it = wordRe.globalMatch(line);
    while (it.hasNext())
    {
        auto m = it.next();
        int s = m.capturedStart();
        int len = m.capturedLength();
        if (isInRanges(s, stringRanges) || isInRanges(s, commentRanges))
            continue;
        QString w = m.captured(0);
        if (kwSet.contains(w))
            segs.append({s, len, HighlightKind::Keyword});
        else
        {
            // Number detection: simple regex for numbers
            static QRegularExpression numRe(QStringLiteral("^[0-9]+(\\.[0-9]+)?$"));
            if (numRe.match(w).hasMatch())
                segs.append({s, len, HighlightKind::Number});
        }
    }

    // Preprocessor for cpp: line starting with # (when not already string/comment)
    if ((language == QStringLiteral("cpp") || language == QStringLiteral("csharp")) && !line.isEmpty())
    {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith(QLatin1Char('#')))
        {
            int start = line.indexOf(QLatin1Char('#'));
            if (!isInRanges(start, stringRanges))
                segs.append({start, n - start, HighlightKind::Preprocessor});
        }
    }

    // Sort by start (stable)
    std::sort(segs.begin(), segs.end(), [](const HighlightSegment& a, const HighlightSegment& b){ return a.start < b.start; });
    // Remove overlapping (keep first - strings/comments already take precedence, but keywords might overlap numbers - keep earliest)
    QList<HighlightSegment> filtered;
    int lastEnd = -1;
    for (auto &s : segs)
    {
        if (s.start >= lastEnd)
        {
            filtered.append(s);
            lastEnd = s.start + s.length;
        }
    }
    return filtered;
}
