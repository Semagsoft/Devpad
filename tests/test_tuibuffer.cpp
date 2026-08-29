/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tui/tuibuffer.h"

#include <gtest/gtest.h>

TEST(TuiBuffer, EmptyIsOneLine)
{
    TuiBuffer b;
    EXPECT_EQ(b.lineCount(), 1);
    EXPECT_EQ(b.text(), QStringLiteral(""));
}

TEST(TuiBuffer, SetAndGetText)
{
    TuiBuffer b(QString(), QStringLiteral("a\nb\nc"));
    EXPECT_EQ(b.lineCount(), 3);
    EXPECT_EQ(b.text(), QStringLiteral("a\nb\nc"));
    EXPECT_EQ(b.lines(), QStringList({QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}));
}

TEST(TuiBuffer, InsertChar)
{
    TuiBuffer b(QString(), QStringLiteral("hello"));
    b.setCursor(0, 5);
    b.insertChar(QChar('!'));
    EXPECT_EQ(b.text(), QStringLiteral("hello!"));
    EXPECT_TRUE(b.isModified());
}

TEST(TuiBuffer, BackspaceMerge)
{
    TuiBuffer b(QString(), QStringLiteral("a\nb"));
    b.setCursor(1, 0);
    b.backspace();
    EXPECT_EQ(b.text(), QStringLiteral("ab"));
    EXPECT_EQ(b.cursorLine(), 0);
}

TEST(TuiBuffer, NewLineSplit)
{
    TuiBuffer b(QString(), QStringLiteral("hello world"));
    b.setCursor(0, 5);
    b.newLine();
    EXPECT_EQ(b.text(), QStringLiteral("hello\n world"));
    EXPECT_EQ(b.cursorLine(), 1);
    EXPECT_EQ(b.cursorCol(), 0);
}

TEST(TuiBuffer, ReadOnlyPreventsEdit)
{
    TuiBuffer b(QString(), QStringLiteral("hi"));
    b.setReadOnly(true);
    b.insertChar(QChar('!'));
    EXPECT_EQ(b.text(), QStringLiteral("hi"));
}

TEST(TuiBuffer, Bookmarks)
{
    TuiBuffer b;
    b.toggleBookmark(2);
    EXPECT_TRUE(b.hasBookmark(2));
    b.toggleBookmark(2);
    EXPECT_FALSE(b.hasBookmark(2));
}

TEST(TuiBuffer, CursorClamping)
{
    TuiBuffer b(QString(), QStringLiteral("ab"));
    b.setCursor(10, 10);
    EXPECT_EQ(b.cursorLine(), 0);
    EXPECT_EQ(b.cursorCol(), 2);
}
