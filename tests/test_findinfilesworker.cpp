/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * Regression test for find-in-files directory exclusion.
 *
 * Cover: excludeDirs entries are bare names ("node_modules", "__pycache__")
 * and are matched against whole path segments, so real paths under excluded
 * directories are skipped while similarly-named directories
 * ("my_node_modules") are not.
 *
 * Note: QDirIterator is constructed with QDir::Files only, so hidden entries
 * (".git", ".svn", ".hg", ".gitignore") are not even listed by the iterator;
 * this test therefore focuses on the non-hidden exclude dirs where the bug
 * actually caused full-tree searches.
 */

#include "findinfilesdialog.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

namespace
{
bool writeFile(const QString& path, const QString& contents)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    f.write(contents.toUtf8());
    f.close();
    return true;
}
} // namespace

TEST(FindInFilesWorker, ExcludesBareDirNames)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    const QString root = dir.path();

    // Files that SHOULD be searched (contain "hello").
    ASSERT_TRUE(writeFile(root + "/keep.txt", "hello world\n"));
    ASSERT_TRUE(QDir(root).mkdir("sub"));
    ASSERT_TRUE(writeFile(root + "/sub/deep.txt", "hello\n"));

    // Files under non-hidden excluded directories - must NOT be searched.
    QDir(root).mkdir("node_modules");
    QDir(root + "/node_modules").mkdir("pkg");
    ASSERT_TRUE(writeFile(root + "/node_modules/pkg/index.js", "hello\n"));
    QDir(root).mkdir("__pycache__");
    ASSERT_TRUE(writeFile(root + "/__pycache__/mod.cpython.pyc", "hello\n"));

    // A similarly-named directory must NOT be excluded (whole-segment match).
    QDir(root).mkdir("my_node_modules");
    ASSERT_TRUE(writeFile(root + "/my_node_modules/keep.js", "hello\n"));

    int totalMatches = -1;
    FindInFilesWorker worker("hello", root, "*", QStringList{"node_modules", "__pycache__"}, false, false, false);
    QObject::connect(&worker, &FindInFilesWorker::searchFinished, [&totalMatches](int n) { totalMatches = n; });

    worker.run();

    // keep.txt (1) + sub/deep.txt (1) + my_node_modules/keep.js (1) = 3
    // node_modules/pkg/index.js and __pycache__/mod.cpython.pyc are excluded (0 each).
    EXPECT_EQ(totalMatches, 3);
}

TEST(FindInFilesWorker, DoesNotExcludeSimilarlyNamedDirectory)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());

    const QString root = dir.path();

    ASSERT_TRUE(writeFile(root + "/keep.txt", "hello\n"));
    QDir(root).mkdir("node_modules");
    ASSERT_TRUE(writeFile(root + "/node_modules/pkg.js", "hello\n"));
    // "my_node_modules" contains "node_modules" but is a different segment.
    QDir(root).mkdir("my_node_modules");
    ASSERT_TRUE(writeFile(root + "/my_node_modules/pkg.js", "hello\n"));

    int totalMatches = -1;
    FindInFilesWorker worker("hello", root, "*", QStringList{"node_modules"}, false, false, false);
    QObject::connect(&worker, &FindInFilesWorker::searchFinished, [&totalMatches](int n) { totalMatches = n; });

    worker.run();

    // keep.txt (1) + my_node_modules/pkg.js (1) = 2; node_modules/pkg.js excluded (0).
    EXPECT_EQ(totalMatches, 2);
}
