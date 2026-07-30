#include "gitignore.h"

#include <QDir>
#include <QTemporaryDir>

#include <gtest/gtest.h>

class GitIgnoreTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    QString m_dirPath;

    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());
        m_dirPath = m_tempDir.path();
    }

    void writeGitIgnore(const QString& content)
    {
        QFile file(m_dirPath + "/.gitignore");
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(content.toUtf8());
        file.close();
    }

    QString makeFile(const QString& name)
    {
        QString path = m_dirPath + "/" + name;
        QFile file(path);
        static_cast<void>(file.open(QIODevice::WriteOnly));
        file.close();
        return path;
    }

    QString makeDir(const QString& name)
    {
        QString path = m_dirPath + "/" + name;
        QDir().mkpath(path);
        return path;
    }
};

TEST_F(GitIgnoreTest, DefaultConstructorIsEmpty)
{
    GitIgnore gi;
    EXPECT_TRUE(gi.isEmpty());
    EXPECT_FALSE(gi.isIgnored("anything", false));
}

TEST_F(GitIgnoreTest, EmptyGitIgnore)
{
    writeGitIgnore("");
    GitIgnore gi(m_dirPath);
    EXPECT_TRUE(gi.isEmpty());
}

TEST_F(GitIgnoreTest, CommentsOnly)
{
    writeGitIgnore("# comment\n# another\n");
    GitIgnore gi(m_dirPath);
    EXPECT_TRUE(gi.isEmpty());
}

TEST_F(GitIgnoreTest, IgnoreSimplePattern)
{
    writeGitIgnore("build/\n");
    GitIgnore gi(m_dirPath);
    EXPECT_FALSE(gi.isEmpty());
    EXPECT_TRUE(gi.isIgnored(makeDir("build"), true));
    EXPECT_TRUE(gi.isIgnored(makeDir("build/subdir"), true));
}

TEST_F(GitIgnoreTest, IgnoreStarDotExt)
{
    writeGitIgnore("*.log\n");
    GitIgnore gi(m_dirPath);
    EXPECT_TRUE(gi.isIgnored(makeFile("trace.log"), false));
    EXPECT_FALSE(gi.isIgnored(makeFile("trace.txt"), false));
}

TEST_F(GitIgnoreTest, NegatePattern)
{
    writeGitIgnore("*.log\n!important.log\n");
    GitIgnore gi(m_dirPath);
    EXPECT_TRUE(gi.isIgnored(makeFile("trace.log"), false));
    EXPECT_FALSE(gi.isIgnored(makeFile("important.log"), false));
}

TEST_F(GitIgnoreTest, AnchoredPattern)
{
    writeGitIgnore("/build\n");
    GitIgnore gi(m_dirPath);
    EXPECT_TRUE(gi.isIgnored(makeDir("build"), true));
    EXPECT_FALSE(gi.isIgnored(makeDir("sub/build"), false));
}

TEST_F(GitIgnoreTest, SubdirectoryGitignore)
{
    writeGitIgnore("*.o\n");
    GitIgnore gi(m_dirPath);
    gi.scanDirectory(m_dirPath);

    QString subDir = makeDir("src");
    QFile gf(subDir + "/.gitignore");
    ASSERT_TRUE(gf.open(QIODevice::WriteOnly | QIODevice::Text));
    gf.write("*.a\n");
    gf.close();
    gi.scanDirectory(subDir);

    EXPECT_TRUE(gi.isIgnored(makeFile("main.o"), false));
    EXPECT_TRUE(gi.isIgnored(makeFile("src/lib.a"), false));
}

TEST_F(GitIgnoreTest, DirOnlyPatternAllowsFile)
{
    writeGitIgnore("build/\n");
    GitIgnore gi(m_dirPath);
    EXPECT_FALSE(gi.isIgnored(makeFile("build"), false));
}

TEST_F(GitIgnoreTest, WildcardMatch)
{
    writeGitIgnore("*.tmp\n");
    GitIgnore gi(m_dirPath);
    EXPECT_TRUE(gi.isIgnored(makeFile("file.tmp"), false));
    EXPECT_TRUE(gi.isIgnored(makeFile("a.b.tmp"), false));
}

TEST_F(GitIgnoreTest, SetRootPathClearsAndReloads)
{
    writeGitIgnore("*.txt\n");
    GitIgnore gi;
    gi.setRootPath(m_dirPath);
    EXPECT_FALSE(gi.isEmpty());
    EXPECT_TRUE(gi.isIgnored(makeFile("notes.txt"), false));
}

TEST_F(GitIgnoreTest, ClearResetsState)
{
    writeGitIgnore("*.txt\n");
    GitIgnore gi(m_dirPath);
    gi.clear();
    EXPECT_TRUE(gi.isEmpty());
    EXPECT_FALSE(gi.isIgnored(makeFile("notes.txt"), false));
}
