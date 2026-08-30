/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * Lightweight syntax highlighter for TUI - keyword/string/comment/number.
 */

#ifndef TUIHIGHLIGHTER_H
#define TUIHIGHLIGHTER_H

#include <QHash>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

enum class HighlightKind
{
    Normal = 0,
    Keyword,
    String,
    Comment,
    Number,
    Preprocessor
};

struct HighlightSegment
{
    int start = 0;
    int length = 0;
    HighlightKind kind = HighlightKind::Normal;
};

class TuiHighlighter
{
public:
    static QList<HighlightSegment> highlightLine(const QString& line, const QString& language);
    static QString languageForFile(const QString& filePath);
    static bool isSyntaxEnabled() { return s_enabled; }
    static void setEnabled(bool e) { s_enabled = e; }
    static void setLanguageOverride(const QString& lang) { s_override = lang; }
    static QString currentLanguage(const QString& filePath);

private:
    static const QStringList& keywordsForLang(const QString& lang);
    static QHash<QString, QStringList> s_keywordCache;
    static bool s_enabled;
    static QString s_override;
};

#endif // TUIHIGHLIGHTER_H
