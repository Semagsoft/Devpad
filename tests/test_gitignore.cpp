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
TEST_F(GitIgnoreTest, SlashPatternIsAnchoredToGitIgnoreRoot)
{
    // gitignore(5): a pattern with a separator (not only at the end) only
    // matches paths relative to the .gitignore file's directory.
    writeGitIgnore("doc/*.html\n");
    GitIgnore gi(m_dirPath);

    EXPECT_TRUE(gi.isIgnored(makeFile("doc/page.html"), false));
    // Nested under another directory: not matched (git anchors slash patterns).
    EXPECT_FALSE(gi.isIgnored(makeFile("a/doc/page.html"), false));
}
TEST_F(GitIgnoreTest, AnchoredSlashPatternMatchesFullRelativePath)
{
    writeGitIgnore("src/*.cpp\n");
    GitIgnore gi(m_dirPath);

    EXPECT_TRUE(gi.isIgnored(makeFile("src/main.cpp"), false));
    EXPECT_FALSE(gi.isIgnored(makeFile("src/nested/main.cpp"), false));
    EXPECT_FALSE(gi.isIgnored(makeFile("tools/src/main.cpp"), false));
}

TEST_F(GitIgnoreTest, BarePatternMatchesIntermediateDirectoryComponent)
{
    // Pattern 'b' (no slash) is a shell glob matched against pathname
    // components; it must exclude the directory b and everything beneath.
    writeGitIgnore("b\n");
    GitIgnore gi(m_dirPath);

    EXPECT_TRUE(gi.isIgnored(makeFile("a/b/file.txt"), false));
    EXPECT_TRUE(gi.isIgnored(makeFile("a/b/deep/file.txt"), false));
    EXPECT_TRUE(gi.isIgnored(makeDir("a/b"), true));
}

TEST_F(GitIgnoreTest, SlashedPatternMatchesLeadingPathPrefix)
{
    // 'a/b' anchors to the .gitignore root and must exclude the directory
    // a/b and everything under it; not x/a/b.
    writeGitIgnore("a/b\n");
    GitIgnore gi(m_dirPath);

    EXPECT_TRUE(gi.isIgnored(makeFile("a/b/file.txt"), false));
    EXPECT_TRUE(gi.isIgnored(makeDir("a/b"), true));
    EXPECT_FALSE(gi.isIgnored(makeFile("x/a/b/file.txt"), false));
}

TEST_F(GitIgnoreTest, NegatedCharacterClass)
{
    // '[!abc]' in git is a negated class; it must not match a, b or c.
    writeGitIgnore("file[!abc].txt\n");
    GitIgnore gi(m_dirPath);

    EXPECT_TRUE(gi.isIgnored(makeFile("filed.txt"), false));
    EXPECT_FALSE(gi.isIgnored(makeFile("filea.txt"), false));
    EXPECT_FALSE(gi.isIgnored(makeFile("fileb.txt"), false));
    EXPECT_FALSE(gi.isIgnored(makeFile("filec.txt"), false));
}

TEST_F(GitIgnoreTest, NegatedCharacterClassRange)
{
    writeGitIgnore("file[!0-9].txt\n");
    GitIgnore gi(m_dirPath);

    EXPECT_TRUE(gi.isIgnored(makeFile("filex.txt"), false));
    EXPECT_FALSE(gi.isIgnored(makeFile("file5.txt"), false));
}

TEST_F(GitIgnoreTest, EscapedSpecialCharacter)
{
    // Backslash escapes the following character: '\!important.log' matches
    // the literal file '!important.log'.
    writeGitIgnore("\\!important.log\n");
    GitIgnore gi(m_dirPath);

    EXPECT_TRUE(gi.isIgnored(makeFile("!important.log"), false));
    EXPECT_FALSE(gi.isIgnored(makeFile("important.log"), false));
}

TEST_F(GitIgnoreTest, TrailingSlashPatternExcludesNestedDirAndContents)
{
    writeGitIgnore("cache/\n");
    GitIgnore gi(m_dirPath);

    EXPECT_TRUE(gi.isIgnored(makeFile("cache/data.bin"), false));
    EXPECT_TRUE(gi.isIgnored(makeDir("cache/deep/nested"), true));
    EXPECT_FALSE(gi.isIgnored(makeFile("cache.txt"), false));
}
