/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tui/tuifiletree.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

TEST(TuiFileTree, EmptyRoot)
{
    TuiFileTree tree;
    EXPECT_FALSE(tree.hasRoot());
    EXPECT_TRUE(tree.visibleNodes().isEmpty());
}

TEST(TuiFileTree, SetRootAndList)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QFile f1(dir.filePath("a.txt"));
    ASSERT_TRUE(f1.open(QIODevice::WriteOnly));
    f1.write("hi");
    f1.close();
    QDir().mkdir(dir.filePath("sub"));
    QFile f2(dir.filePath("sub/b.txt"));
    ASSERT_TRUE(f2.open(QIODevice::WriteOnly));
    f2.write("hi");
    f2.close();

    TuiFileTree tree;
    tree.setRootPath(dir.path());
    ASSERT_TRUE(tree.hasRoot());
    auto nodes = tree.visibleNodes();
    // Root expanded by default, should contain a.txt, sub, and sub/b.txt when sub expanded?
    // Initially sub not expanded? Root expanded true, sub collapsed false, so visible = a.txt + sub
    // Our implementation auto-expands root only, not sub, so expect 2 nodes
    EXPECT_EQ(nodes.size(), 2);
    // Expand sub
    tree.setExpanded(dir.filePath("sub"), true);
    nodes = tree.visibleNodes();
    EXPECT_EQ(nodes.size(), 3);
}

TEST(TuiFileTree, Filter)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QFile f1(dir.filePath("hello.txt"));
    f1.open(QIODevice::WriteOnly);
    f1.close();
    QFile f2(dir.filePath("world.cpp"));
    f2.open(QIODevice::WriteOnly);
    f2.close();

    TuiFileTree tree;
    tree.setRootPath(dir.path());
    tree.setFilter("hello");
    auto nodes = tree.visibleNodes();
    // Only hello.txt should be visible (plus maybe dirs)
    bool foundHello = false, foundWorld = false;
    for (auto& n : nodes)
    {
        if (n.name.contains("hello"))
            foundHello = true;
        if (n.name.contains("world"))
            foundWorld = true;
    }
    EXPECT_TRUE(foundHello);
    EXPECT_FALSE(foundWorld);
}

TEST(TuiFileTree, ShowHidden)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QFile f1(dir.filePath(".hidden"));
    f1.open(QIODevice::WriteOnly);
    f1.close();
    QFile f2(dir.filePath("visible.txt"));
    f2.open(QIODevice::WriteOnly);
    f2.close();

    TuiFileTree tree;
    tree.setRootPath(dir.path());
    tree.setShowHidden(false);
    auto nodes = tree.visibleNodes();
    bool hasHidden = false;
    for (auto& n : nodes)
        if (n.name == ".hidden")
            hasHidden = true;
    EXPECT_FALSE(hasHidden);
    tree.setShowHidden(true);
    nodes = tree.visibleNodes();
    hasHidden = false;
    for (auto& n : nodes)
        if (n.name == ".hidden")
            hasHidden = true;
    EXPECT_TRUE(hasHidden);
}

TEST(TuiFileTree, GitIgnore)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QFile ignore(dir.filePath(".gitignore"));
    ASSERT_TRUE(ignore.open(QIODevice::WriteOnly | QIODevice::Text));
    ignore.write("ignored.txt\n");
    ignore.close();
    QFile f1(dir.filePath("ignored.txt"));
    f1.open(QIODevice::WriteOnly);
    f1.close();
    QFile f2(dir.filePath("kept.txt"));
    f2.open(QIODevice::WriteOnly);
    f2.close();

    TuiFileTree tree;
    tree.setRootPath(dir.path());
    auto nodes = tree.visibleNodes();
    bool hasIgnored = false, hasKept = false;
    for (auto& n : nodes)
    {
        if (n.name == "ignored.txt")
            hasIgnored = true;
        if (n.name == "kept.txt")
            hasKept = true;
    }
    EXPECT_FALSE(hasIgnored);
    EXPECT_TRUE(hasKept);
}

TEST(TuiFileTree, Cursor)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    for (int i = 0; i < 3; ++i)
    {
        QFile f(dir.filePath(QString("f%1.txt").arg(i)));
        f.open(QIODevice::WriteOnly);
        f.close();
    }
    TuiFileTree tree;
    tree.setRootPath(dir.path());
    EXPECT_EQ(tree.cursorIndex(), 0);
    tree.moveCursor(1);
    EXPECT_EQ(tree.cursorIndex(), 1);
    tree.moveCursor(10);
    auto nodes = tree.visibleNodes();
    EXPECT_EQ(tree.cursorIndex(), nodes.size() - 1);
}
