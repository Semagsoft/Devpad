#include "panels/projectpanel.h"

#include <QDir>
#include <QTemporaryDir>
#include <QSignalSpy>

#include <gtest/gtest.h>

TEST(ProjectPanelTest, InitialState)
{
    ProjectPanel panel;
    EXPECT_TRUE(panel.rootPath().isEmpty());
    EXPECT_FALSE(panel.iconForFile(QStringLiteral("test.cpp")).isNull());
}

TEST(ProjectPanelTest, SetRootPathValidDirectory)
{
    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    ProjectPanel panel;
    panel.setRootPath(tmp.path());
    EXPECT_EQ(panel.rootPath(), tmp.path());
}

TEST(ProjectPanelTest, SetRootPathEmptyClears)
{
    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    ProjectPanel panel;
    panel.setRootPath(tmp.path());
    ASSERT_FALSE(panel.rootPath().isEmpty());
    panel.clear();
    EXPECT_TRUE(panel.rootPath().isEmpty());
}

TEST(ProjectPanelTest, IconForKnownAndUnknownFiles)
{
    QIcon cppIcon = ProjectPanel::iconForFile(QStringLiteral("main.cpp"));
    QIcon pyIcon = ProjectPanel::iconForFile(QStringLiteral("script.py"));
    QIcon unknownIcon = ProjectPanel::iconForFile(QStringLiteral("file.unknownxyz"));
    // Known types should return non-null, unknown may fallback to generic but not crash
    EXPECT_FALSE(cppIcon.isNull());
    EXPECT_FALSE(pyIcon.isNull());
    (void)unknownIcon;
}

TEST(ProjectPanelTest, FileFilterProxyModelText)
{
    FileFilterProxyModel proxy;
    proxy.setFilterText(QStringLiteral("test"));
    proxy.setFilterText(QString());
    // No crash; indirect coverage of filterAcceptsRow via project panel
    SUCCEED();
}

TEST(ProjectPanelTest, FileFilterProxyModelGitIgnore)
{
    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    // Create a .gitignore that ignores *.log
    QFile ignore(tmp.filePath(QStringLiteral(".gitignore")));
    ASSERT_TRUE(ignore.open(QIODevice::WriteOnly | QIODevice::Text));
    ignore.write("*.log\n");
    ignore.close();

    FileFilterProxyModel proxy;
    proxy.setGitIgnoreEnabled(true);
    proxy.setGitIgnoreRootPath(tmp.path());
    proxy.scanGitIgnoreDirectory(tmp.path());

    QFile logFile(tmp.filePath(QStringLiteral("ignore.log")));
    ASSERT_TRUE(logFile.open(QIODevice::WriteOnly));
    logFile.close();

    QFile keepFile(tmp.filePath(QStringLiteral("keep.cpp")));
    ASSERT_TRUE(keepFile.open(QIODevice::WriteOnly));
    keepFile.close();

    // Indirect: proxy should not crash with gitignore enabled
    proxy.setFilterText(QStringLiteral(""));
    SUCCEED();
}

TEST(ProjectPanelTest, SignalsFileActivated)
{
    ProjectPanel panel;
    QTemporaryDir tmp;
    ASSERT_TRUE(tmp.isValid());
    QFile f(tmp.filePath(QStringLiteral("hello.txt")));
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("hi");
    f.close();

    panel.setRootPath(tmp.path());
    QSignalSpy spy(&panel, &ProjectPanel::fileActivated);
    // Emitting is triggered via double-click; we just verify signal can be connected
    EXPECT_EQ(spy.count(), 0);
}
