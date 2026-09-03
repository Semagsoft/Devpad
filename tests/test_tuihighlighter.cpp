/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tui/tuihighlighter.h"

#include <gtest/gtest.h>

TEST(TuiHighlighter, LanguageForFile)
{
    EXPECT_EQ(TuiHighlighter::languageForFile(QStringLiteral("foo.cpp")), QStringLiteral("cpp"));
    EXPECT_EQ(TuiHighlighter::languageForFile(QStringLiteral("bar.py")), QStringLiteral("python"));
    EXPECT_EQ(TuiHighlighter::languageForFile(QStringLiteral("test.js")), QStringLiteral("javascript"));
    EXPECT_EQ(TuiHighlighter::languageForFile(QStringLiteral("unknown.xyz")), QString());
}

TEST(TuiHighlighter, KeywordsCpp)
{
    auto segs = TuiHighlighter::highlightLine(QStringLiteral("int main() { return 0; }"), QStringLiteral("cpp"));
    bool hasKeyword = false;
    for (auto& s : segs)
        if (s.kind == HighlightKind::Keyword)
            hasKeyword = true;
    EXPECT_TRUE(hasKeyword);
}

TEST(TuiHighlighter, StringHighlight)
{
    auto segs = TuiHighlighter::highlightLine(QStringLiteral("\"hello\""), QStringLiteral("cpp"));
    bool hasString = false;
    for (auto& s : segs)
        if (s.kind == HighlightKind::String)
            hasString = true;
    EXPECT_TRUE(hasString);
}

TEST(TuiHighlighter, CommentHighlight)
{
    auto segs = TuiHighlighter::highlightLine(QStringLiteral("// comment"), QStringLiteral("cpp"));
    bool hasComment = false;
    for (auto& s : segs)
        if (s.kind == HighlightKind::Comment)
            hasComment = true;
    EXPECT_TRUE(hasComment);
}

TEST(TuiHighlighter, Disabled)
{
    TuiHighlighter::setEnabled(false);
    auto segs = TuiHighlighter::highlightLine(QStringLiteral("int x;"), QStringLiteral("cpp"));
    EXPECT_TRUE(segs.isEmpty());
    TuiHighlighter::setEnabled(true);
}
