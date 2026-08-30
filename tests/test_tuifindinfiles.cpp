/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tui/tuifindinfiles.h"
#include "tui/tuisearchengine.h"

#include <QDir>
#include <QTemporaryDir>
#include <QFile>

#include <gtest/gtest.h>

TEST(TuiFindInFiles, SimpleSearch)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QFile f1(dir.filePath("a.txt"));
    ASSERT_TRUE(f1.open(QIODevice::WriteOnly | QIODevice::Text));
    f1.write("hello world\nfoo bar\n");
    f1.close();
    QFile f2(dir.filePath("b.txt"));
    ASSERT_TRUE(f2.open(QIODevice::WriteOnly | QIODevice::Text));
    f2.write("hello again\n");
    f2.close();

    SearchOptions opts;
    auto results = TuiFindInFiles::search(dir.path(), QStringLiteral("hello"), opts);
    EXPECT_EQ(results.size(), 2);
}

TEST(TuiFindInFiles, GlobFiltering)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QFile f1(dir.filePath("a.cpp"));
    f1.open(QIODevice::WriteOnly); f1.write("hello"); f1.close();
    QFile f2(dir.filePath("b.txt"));
    f2.open(QIODevice::WriteOnly); f2.write("hello"); f2.close();

    SearchOptions opts;
    auto results = TuiFindInFiles::search(dir.path(), QStringLiteral("hello"), opts, QStringLiteral("*.cpp"));
    EXPECT_EQ(results.size(), 1);
    EXPECT_TRUE(results[0].filePath.endsWith("a.cpp"));
}

TEST(TuiFindInFiles, GitIgnore)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QFile ignore(dir.filePath(".gitignore"));
    ASSERT_TRUE(ignore.open(QIODevice::WriteOnly | QIODevice::Text));
    ignore.write("ignored.txt\n");
    ignore.close();
    QFile f1(dir.filePath("ignored.txt"));
    f1.open(QIODevice::WriteOnly); f1.write("hello"); f1.close();
    QFile f2(dir.filePath("kept.txt"));
    f2.open(QIODevice::WriteOnly); f2.write("hello"); f2.close();

    SearchOptions opts;
    auto results = TuiFindInFiles::search(dir.path(), QStringLiteral("hello"), opts);
    bool hasIgnored = false, hasKept = false;
    for (auto &r : results) {
        if (r.filePath.endsWith("ignored.txt")) hasIgnored = true;
        if (r.filePath.endsWith("kept.txt")) hasKept = true;
    }
    EXPECT_FALSE(hasIgnored);
    EXPECT_TRUE(hasKept);
}

TEST(TuiFindInFiles, Regex)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QFile f(dir.filePath("a.txt"));
    f.open(QIODevice::WriteOnly); f.write("a1 b2 c3"); f.close();
    SearchOptions opts;
    opts.regex = true;
    auto results = TuiFindInFiles::search(dir.path(), QStringLiteral("\\d"), opts);
    EXPECT_EQ(results.size(), 3);
}
