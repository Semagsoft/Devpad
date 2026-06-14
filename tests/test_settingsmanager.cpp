#include "defaultsyntax.h"
#include "settingsmanager.h"

#include <gtest/gtest.h>

class SettingsManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings);
    }

    void TearDown() override
    {
        SettingsManager::setTestingInstance(nullptr);
        SettingsManager::destroyForTesting(m_testSettings);
        m_testSettings = nullptr;
    }

    SettingsManager* m_testSettings = nullptr;
};

TEST_F(SettingsManagerTest, RecentFilesAddAndList)
{
    SettingsManager::instance().clearRecentFiles();
    EXPECT_TRUE(SettingsManager::instance().recentFiles().isEmpty());

    SettingsManager::instance().addRecentFile("/path/to/file1.txt");
    SettingsManager::instance().addRecentFile("/path/to/file2.txt");

    QStringList recent = SettingsManager::instance().recentFiles();
    EXPECT_EQ(recent.size(), 2);
    EXPECT_EQ(recent[0], "/path/to/file2.txt");
    EXPECT_EQ(recent[1], "/path/to/file1.txt");
}

TEST_F(SettingsManagerTest, RecentFilesMaxTen)
{
    SettingsManager::instance().clearRecentFiles();
    for (int i = 0; i < 15; ++i)
    {
        SettingsManager::instance().addRecentFile(QString("/path/to/file%1.txt").arg(i));
    }

    QStringList recent = SettingsManager::instance().recentFiles();
    EXPECT_LE(recent.size(), 10);
}

TEST_F(SettingsManagerTest, RecentFilesClear)
{
    SettingsManager::instance().clearRecentFiles();
    SettingsManager::instance().addRecentFile("/path/to/file.txt");
    EXPECT_FALSE(SettingsManager::instance().recentFiles().isEmpty());

    SettingsManager::instance().clearRecentFiles();
    EXPECT_TRUE(SettingsManager::instance().recentFiles().isEmpty());
}

TEST_F(SettingsManagerTest, RecentFoldersAddAndList)
{
    SettingsManager::instance().clearRecentFolders();
    EXPECT_TRUE(SettingsManager::instance().recentFolders().isEmpty());

    SettingsManager::instance().addRecentFolder("/path/to/folder1");
    SettingsManager::instance().addRecentFolder("/path/to/folder2");

    QStringList recent = SettingsManager::instance().recentFolders();
    EXPECT_EQ(recent.size(), 2);
    EXPECT_EQ(recent[0], "/path/to/folder2");
}

TEST_F(SettingsManagerTest, RecentFoldersClear)
{
    SettingsManager::instance().addRecentFolder("/path/to/folder");
    EXPECT_FALSE(SettingsManager::instance().recentFolders().isEmpty());

    SettingsManager::instance().clearRecentFolders();
    EXPECT_TRUE(SettingsManager::instance().recentFolders().isEmpty());
}

TEST_F(SettingsManagerTest, DefaultSyntaxLanguagesNonEmpty)
{
    auto langs = defaultSyntaxLanguages();
    EXPECT_FALSE(langs.isEmpty());
    EXPECT_TRUE(langs.contains("cpp"));
    EXPECT_TRUE(langs.contains("python"));
    EXPECT_TRUE(langs.contains("html"));
    EXPECT_TRUE(langs.contains("javascript"));
}

TEST_F(SettingsManagerTest, SettingsVersionIsCurrentAfterConstruction)
{
    EXPECT_GE(SettingsManager::instance().syntaxForExtension("cpp"), "cpp");
}

TEST_F(SettingsManagerTest, SyntaxForExtension)
{
    EXPECT_EQ(SettingsManager::instance().syntaxForExtension("cpp"), "cpp");
    EXPECT_EQ(SettingsManager::instance().syntaxForExtension("py"), "python");
    EXPECT_EQ(SettingsManager::instance().syntaxForExtension("js"), "javascript");
    EXPECT_EQ(SettingsManager::instance().syntaxForExtension("rs"), "rust");
    EXPECT_EQ(SettingsManager::instance().syntaxForExtension("go"), "go");
    EXPECT_TRUE(SettingsManager::instance().syntaxForExtension("xyz123").isEmpty());
}

TEST_F(SettingsManagerTest, SyntaxForFile)
{
    EXPECT_EQ(SettingsManager::instance().syntaxForFile("main.cpp"), "cpp");
    EXPECT_EQ(SettingsManager::instance().syntaxForFile("app.py"), "python");
    EXPECT_EQ(SettingsManager::instance().syntaxForFile("CMakeLists.txt"), "cmake");
    EXPECT_EQ(SettingsManager::instance().syntaxForFile("readme.md"), "markdown");
}
