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

TEST(TuiBuffer, UndoRedo)
{
    TuiBuffer b(QString(), QStringLiteral("hello"));
    b.setCursor(0, 5);
    b.insertChar(QChar('!'));
    EXPECT_EQ(b.text(), QStringLiteral("hello!"));
    EXPECT_TRUE(b.canUndo());
    EXPECT_TRUE(b.undo());
    EXPECT_EQ(b.text(), QStringLiteral("hello"));
    EXPECT_FALSE(b.canUndo());
    EXPECT_TRUE(b.canRedo());
    EXPECT_TRUE(b.redo());
    EXPECT_EQ(b.text(), QStringLiteral("hello!"));
}

TEST(TuiBuffer, UndoClearsRedo)
{
    TuiBuffer b(QString(), QStringLiteral("a"));
    b.setCursor(0, 1);
    b.insertChar(QChar('b'));
    b.undo();
    EXPECT_TRUE(b.canRedo());
    b.setCursor(0, 1);
    b.insertChar(QChar('c'));
    EXPECT_FALSE(b.canRedo());
    EXPECT_EQ(b.text(), QStringLiteral("ac"));
}

TEST(TuiBuffer, SelectionAndDelete)
{
    TuiBuffer b(QString(), QStringLiteral("hello world"));
    b.setCursor(0, 0);
    b.setSelectionAnchor(0, 0);
    b.setCursor(0, 5);
    EXPECT_TRUE(b.hasSelection());
    EXPECT_EQ(b.selectedText(), QStringLiteral("hello"));
    b.deleteSelection();
    EXPECT_EQ(b.text(), QStringLiteral(" world"));
    EXPECT_FALSE(b.hasSelection());
}

TEST(TuiBuffer, SelectAllAndCutPaste)
{
    TuiBuffer b(QString(), QStringLiteral("abc\ndef"));
    b.selectAll();
    EXPECT_TRUE(b.hasSelection());
    EXPECT_EQ(b.selectedText(), QStringLiteral("abc\ndef"));
    QString sel = b.selectedText();
    b.deleteSelection();
    EXPECT_EQ(b.text(), QStringLiteral(""));
    b.insertText(sel);
    EXPECT_EQ(b.text(), QStringLiteral("abc\ndef"));
}

TEST(TuiBuffer, InsertReplacesSelection)
{
    TuiBuffer b(QString(), QStringLiteral("hello world"));
    b.setSelectionAnchor(0, 0);
    b.setCursor(0, 5);
    b.insertChar(QChar('X'));
    EXPECT_EQ(b.text(), QStringLiteral("X world"));
}

TEST(TuiBuffer, ReplaceNext)
{
    TuiBuffer b(QString(), QStringLiteral("foo foo foo"));
    b.setCursor(0, 0);
    SearchOptions opts;
    auto rr = b.replaceNext(QStringLiteral("foo"), QStringLiteral("bar"), opts);
    EXPECT_TRUE(rr.found);
    EXPECT_EQ(b.text(), QStringLiteral("bar foo foo"));
    EXPECT_TRUE(b.canUndo());
    b.undo();
    EXPECT_EQ(b.text(), QStringLiteral("foo foo foo"));
}

TEST(TuiBuffer, ReplaceAll)
{
    TuiBuffer b(QString(), QStringLiteral("a a a"));
    SearchOptions opts;
    int cnt = b.replaceAll(QStringLiteral("a"), QStringLiteral("b"), opts);
    EXPECT_EQ(cnt, 3);
    EXPECT_EQ(b.text(), QStringLiteral("b b b"));
    EXPECT_TRUE(b.undo());
    EXPECT_EQ(b.text(), QStringLiteral("a a a"));
}

TEST(TuiBuffer, ReplaceUndoRedo)
{
    TuiBuffer b(QString(), QStringLiteral("hello hello"));
    SearchOptions opts;
    b.replaceAll(QStringLiteral("hello"), QStringLiteral("hi"), opts);
    EXPECT_EQ(b.text(), QStringLiteral("hi hi"));
    b.undo();
    EXPECT_EQ(b.text(), QStringLiteral("hello hello"));
    b.redo();
    EXPECT_EQ(b.text(), QStringLiteral("hi hi"));
}

TEST(TuiBuffer, BookmarkNextPrevWrap)
{
    TuiBuffer b(QString(), QStringLiteral("a\nb\nc\nd"));
    b.setBookmarks(QList<int>{1, 3});
    int out = -1;
    EXPECT_TRUE(b.nextBookmark(0, &out));
    EXPECT_EQ(out, 1);
    EXPECT_TRUE(b.nextBookmark(1, &out));
    EXPECT_EQ(out, 3);
    EXPECT_TRUE(b.nextBookmark(3, &out));
    EXPECT_EQ(out, 1); // wrap
    EXPECT_TRUE(b.prevBookmark(3, &out));
    EXPECT_EQ(out, 1);
    EXPECT_TRUE(b.prevBookmark(1, &out));
    EXPECT_EQ(out, 3); // wrap
    EXPECT_TRUE(b.prevBookmark(0, &out));
    EXPECT_EQ(out, 3);
}

TEST(TuiBuffer, BookmarkShiftOnInsert)
{
    TuiBuffer b(QString(), QStringLiteral("a\nb\nc"));
    b.setBookmarks(QList<int>{2});
    b.setCursor(1, 0);
    b.newLine(); // inserts at line 1, shifts bookmark 2 -> 3
    EXPECT_TRUE(b.hasBookmark(3));
    EXPECT_FALSE(b.hasBookmark(2));
}

TEST(TuiBuffer, BookmarkShiftOnDelete)
{
    TuiBuffer b(QString(), QStringLiteral("a\nb\nc\nd"));
    b.setBookmarks(QList<int>{1, 3});
    b.deleteLine(1); // removes line 1, bookmarks 3 -> 2
    EXPECT_FALSE(b.hasBookmark(1));
    EXPECT_TRUE(b.hasBookmark(2));
    EXPECT_EQ(b.bookmarkCount(), 1);
}

TEST(TuiBuffer, BookmarkClear)
{
    TuiBuffer b(QString(), QStringLiteral("a\nb\nc"));
    b.setBookmarks(QList<int>{0, 1, 2});
    b.clearBookmarks();
    EXPECT_EQ(b.bookmarkCount(), 0);
    EXPECT_FALSE(b.hasBookmark(0));
}
