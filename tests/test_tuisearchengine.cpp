/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tui/tuibuffer.h"
#include "tui/tuisearchengine.h"

#include <gtest/gtest.h>

TEST(TuiSearchEngine, FindNextSimple)
{
    QStringList lines = {QStringLiteral("hello world"), QStringLiteral("foo bar")};
    SearchOptions opts;
    auto r = TuiSearchEngine::findNext(lines, QStringLiteral("world"), opts, 0, 0);
    EXPECT_TRUE(r.found);
    EXPECT_EQ(r.line, 0);
    EXPECT_EQ(r.column, 6);
}

TEST(TuiSearchEngine, CaseInsensitiveDefault)
{
    QStringList lines = {QStringLiteral("Hello")};
    SearchOptions opts;
    opts.caseSensitive = false;
    auto r = TuiSearchEngine::findNext(lines, QStringLiteral("hello"), opts, 0, 0);
    EXPECT_TRUE(r.found);
}

TEST(TuiSearchEngine, CaseSensitive)
{
    QStringList lines = {QStringLiteral("Hello")};
    SearchOptions opts;
    opts.caseSensitive = true;
    auto r = TuiSearchEngine::findNext(lines, QStringLiteral("hello"), opts, 0, 0);
    EXPECT_FALSE(r.found);
}

TEST(TuiSearchEngine, WholeWords)
{
    QStringList lines = {QStringLiteral("foobar foo bar")};
    SearchOptions opts;
    opts.wholeWords = true;
    auto results = TuiSearchEngine::findAll(lines, QStringLiteral("foo"), opts);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].column, 7);
}

TEST(TuiSearchEngine, Regex)
{
    QStringList lines = {QStringLiteral("a1 b2 c3")};
    SearchOptions opts;
    opts.regex = true;
    auto results = TuiSearchEngine::findAll(lines, QStringLiteral("\\d"), opts);
    EXPECT_EQ(results.size(), 3);
}

TEST(TuiSearchEngine, NotFound)
{
    QStringList lines = {QStringLiteral("abc")};
    SearchOptions opts;
    auto r = TuiSearchEngine::findNext(lines, QStringLiteral("xyz"), opts, 0, 0);
    EXPECT_FALSE(r.found);
}

TEST(TuiSearchEngine, ReplaceNextSimple)
{
    QStringList lines = {QStringLiteral("hello world"), QStringLiteral("hello")};
    SearchOptions opts;
    auto r = TuiSearchEngine::replaceNext(lines, QStringLiteral("hello"), QStringLiteral("hi"), opts, 0, 0);
    EXPECT_TRUE(r.found);
    EXPECT_EQ(lines[0], QStringLiteral("hi world"));
    EXPECT_EQ(lines[1], QStringLiteral("hello"));
}

TEST(TuiSearchEngine, ReplaceAll)
{
    QStringList lines = {QStringLiteral("foo foo"), QStringLiteral("foo")};
    SearchOptions opts;
    int cnt = TuiSearchEngine::replaceAll(lines, QStringLiteral("foo"), QStringLiteral("bar"), opts);
    EXPECT_EQ(cnt, 3);
    EXPECT_EQ(lines[0], QStringLiteral("bar bar"));
    EXPECT_EQ(lines[1], QStringLiteral("bar"));
}

TEST(TuiSearchEngine, ReplaceRegexCapture)
{
    QStringList lines = {QStringLiteral("a1 b2")};
    SearchOptions opts;
    opts.regex = true;
    int cnt = TuiSearchEngine::replaceAll(lines, QStringLiteral("(\\w)(\\d)"), QStringLiteral("\\2\\1"), opts);
    EXPECT_EQ(cnt, 2);
    EXPECT_EQ(lines[0], QStringLiteral("1a 2b"));
}

TEST(TuiSearchEngine, ReplaceWholeWords)
{
    QStringList lines = {QStringLiteral("foobar foo")};
    SearchOptions opts;
    opts.wholeWords = true;
    int cnt = TuiSearchEngine::replaceAll(lines, QStringLiteral("foo"), QStringLiteral("bar"), opts);
    EXPECT_EQ(cnt, 1);
    EXPECT_EQ(lines[0], QStringLiteral("foobar bar"));
}

TEST(TuiSearchEngine, ReplaceReadOnlyGuardViaBuffer)
{
    TuiBuffer b(QString(), QStringLiteral("hello hello"));
    b.setReadOnly(true);
    SearchOptions opts;
    auto rr = b.replaceNext(QStringLiteral("hello"), QStringLiteral("hi"), opts);
    EXPECT_FALSE(rr.found);
    EXPECT_EQ(b.text(), QStringLiteral("hello hello"));
}
