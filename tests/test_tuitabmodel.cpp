/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tui/tuitabmodel.h"

#include <gtest/gtest.h>

TEST(TuiTabModel, AddAndCurrent)
{
    TuiTabModel m;
    EXPECT_TRUE(m.isEmpty());
    m.addBuffer(TuiBuffer(QString(), QStringLiteral("a")));
    EXPECT_EQ(m.count(), 1);
    EXPECT_EQ(m.currentIndex(), 0);
    m.addBuffer(TuiBuffer(QString(), QStringLiteral("b")));
    EXPECT_EQ(m.count(), 2);
    EXPECT_EQ(m.currentIndex(), 1);
}

TEST(TuiTabModel, DeduplicateByPath)
{
    TuiTabModel m;
    m.addBuffer(TuiBuffer(QStringLiteral("/tmp/a.txt"), QStringLiteral("hi")));
    m.addBuffer(TuiBuffer(QStringLiteral("/tmp/a.txt"), QStringLiteral("other")));
    EXPECT_EQ(m.count(), 1);
}

TEST(TuiTabModel, FindByFilePath)
{
    TuiTabModel m;
    m.addBuffer(TuiBuffer(QStringLiteral("/tmp/b.txt"), QStringLiteral("x")));
    EXPECT_EQ(m.findByFilePath(QStringLiteral("/tmp/b.txt")), 0);
    EXPECT_EQ(m.findByFilePath(QStringLiteral("/tmp/c.txt")), -1);
}

TEST(TuiTabModel, CloseCurrent)
{
    TuiTabModel m;
    m.addBuffer(TuiBuffer(QString(), QStringLiteral("1")));
    m.addBuffer(TuiBuffer(QString(), QStringLiteral("2")));
    m.setCurrentIndex(0);
    m.closeCurrent();
    EXPECT_EQ(m.count(), 1);
    EXPECT_EQ(m.currentBuffer()->text(), QStringLiteral("2"));
}

TEST(TuiTabModel, Pinned)
{
    TuiTabModel m;
    m.addBuffer(TuiBuffer(QStringLiteral("/tmp/p.txt"), QStringLiteral("x")));
    EXPECT_FALSE(m.isPinned(0));
    m.setPinned(0, true);
    EXPECT_TRUE(m.isPinned(0));
    m.setPinned(0, false);
    EXPECT_FALSE(m.isPinned(0));
}
