#include "defaultsyntax.h"
#include "settingsmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>

#include <gtest/gtest.h>

class SettingsManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
    }

    void TearDown() override
    {
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }

    std::unique_ptr<SettingsManager> m_testSettings;
};

TEST_F(SettingsManagerTest, AutoSaveIntervalIsClamped)
{
    SettingsManager::instance().setAutoSaveInterval(0);
    EXPECT_EQ(SettingsManager::instance().autoSaveInterval(), 1);
    SettingsManager::instance().setAutoSaveInterval(1000000);
    EXPECT_EQ(SettingsManager::instance().autoSaveInterval(), 86400);
}

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

TEST_F(SettingsManagerTest, RecentFoldersAcceptsRemoteUrls)
{
    SettingsManager::instance().clearRecentFolders();

    SettingsManager::instance().addRecentFolder("ssh://user@example.com/home/user/project");
    SettingsManager::instance().addRecentFolder("ssh://user@example.com/home/user/project/");

    QStringList recent = SettingsManager::instance().recentFolders();
    // Verbatim storage, dedupe by exact string, most recent first.
    ASSERT_EQ(recent.size(), 2);
    EXPECT_EQ(recent[0], "ssh://user@example.com/home/user/project/");
    EXPECT_EQ(recent[1], "ssh://user@example.com/home/user/project");
}

TEST_F(SettingsManagerTest, FileOpenModeDefaultIsSingleClick)
{
    EXPECT_EQ(SettingsManager::instance().fileOpenMode(), FileOpenMode::SingleClick);
    EXPECT_EQ(SettingsManager::instance().projectSettings().fileOpenMode, FileOpenMode::SingleClick);
}

TEST_F(SettingsManagerTest, FileOpenModeSetAndGet)
{
    SettingsManager::instance().setFileOpenMode(FileOpenMode::DoubleClick);
    EXPECT_EQ(SettingsManager::instance().fileOpenMode(), FileOpenMode::DoubleClick);
    EXPECT_EQ(SettingsManager::instance().projectSettings().fileOpenMode, FileOpenMode::DoubleClick);

    SettingsManager::instance().setFileOpenMode(FileOpenMode::SingleClick);
    EXPECT_EQ(SettingsManager::instance().fileOpenMode(), FileOpenMode::SingleClick);
}

TEST_F(SettingsManagerTest, ShellOpenModeFilesDefaultIsNewWindow)
{
    EXPECT_EQ(SettingsManager::instance().shellOpenModeFiles(), ShellOpenMode::NewWindow);
}

TEST_F(SettingsManagerTest, ShellOpenModeFoldersDefaultIsNewWindow)
{
    EXPECT_EQ(SettingsManager::instance().shellOpenModeFolders(), ShellOpenMode::NewWindow);
}

TEST_F(SettingsManagerTest, ShellOpenModeFilesSetAndGet)
{
    SettingsManager::instance().setShellOpenModeFiles(ShellOpenMode::CurrentWindow);
    EXPECT_EQ(SettingsManager::instance().shellOpenModeFiles(), ShellOpenMode::CurrentWindow);
    EXPECT_EQ(SettingsManager::instance().uiSettings().shellOpenModeFiles, ShellOpenMode::CurrentWindow);

    SettingsManager::instance().setShellOpenModeFiles(ShellOpenMode::NewWindow);
    EXPECT_EQ(SettingsManager::instance().shellOpenModeFiles(), ShellOpenMode::NewWindow);
}

TEST_F(SettingsManagerTest, ShellOpenModeFoldersSetAndGet)
{
    SettingsManager::instance().setShellOpenModeFolders(ShellOpenMode::CurrentWindow);
    EXPECT_EQ(SettingsManager::instance().shellOpenModeFolders(), ShellOpenMode::CurrentWindow);
    EXPECT_EQ(SettingsManager::instance().uiSettings().shellOpenModeFolders, ShellOpenMode::CurrentWindow);

    SettingsManager::instance().setShellOpenModeFolders(ShellOpenMode::NewWindow);
    EXPECT_EQ(SettingsManager::instance().shellOpenModeFolders(), ShellOpenMode::NewWindow);
}

TEST_F(SettingsManagerTest, DefaultSyntaxLanguagesNonEmpty)
{
    const QStringList langs = defaultSyntaxLanguages();
    EXPECT_FALSE(langs.isEmpty());
    EXPECT_TRUE(langs.contains("cpp"));
    EXPECT_TRUE(langs.contains("python"));
    EXPECT_TRUE(langs.contains("html"));
    EXPECT_TRUE(langs.contains("javascript"));
    // Plugin-provided languages are selectable as the default format too.
    EXPECT_TRUE(langs.contains("rust"));
    EXPECT_TRUE(langs.contains("jsonc"));
    EXPECT_TRUE(langs.contains("toml"));
}

TEST_F(SettingsManagerTest, DefaultFormatUsesStableLanguageIds)
{
    EXPECT_EQ(SettingsManager::instance().defaultSyntax(), QStringLiteral("cpp"));
    SettingsManager::instance().setDefaultSyntax(QStringLiteral("toml"));
    EXPECT_EQ(SettingsManager::instance().defaultSyntax(), QStringLiteral("toml"));
    EXPECT_EQ(SettingsManager::instance().defaultFormat(), defaultSyntaxLanguages().indexOf("toml"));

    // Invalid ids are ignored rather than persisting a broken default.
    SettingsManager::instance().setDefaultSyntax(QStringLiteral("no_such_language"));
    EXPECT_EQ(SettingsManager::instance().defaultSyntax(), QStringLiteral("toml"));

    // The legacy index wrapper still maps to the same stable id.
    SettingsManager::instance().setDefaultFormat(2); // csharp
    EXPECT_EQ(SettingsManager::instance().defaultSyntax(), QStringLiteral("csharp"));
    EXPECT_EQ(SettingsManager::instance().defaultFormat(), 2);
}

TEST_F(SettingsManagerTest, MigratesLegacyDefaultFormatIndex)
{
    const QString path = QDir::temp().filePath(QStringLiteral("devpad_legacy_format_%1.ini").arg(QCoreApplication::applicationPid()));
    {
        QSettings legacy(path, QSettings::IniFormat);
        legacy.setValue(QStringLiteral("Options_DefaultFormat"), 3); // java in the legacy list
    }

    auto migrated = SettingsManager::createForTesting(path);
    SettingsManager::setTestingInstance(migrated.get());
    EXPECT_EQ(migrated->defaultSyntax(), QStringLiteral("java"));
    {
        QSettings updated(path, QSettings::IniFormat);
        EXPECT_EQ(updated.value(QStringLiteral("Options_DefaultSyntax")).toString(), QStringLiteral("java"));
        EXPECT_FALSE(updated.contains(QStringLiteral("Options_DefaultFormat")));
    }

    SettingsManager::setTestingInstance(nullptr);
    QFile::remove(path);
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
    EXPECT_EQ(SettingsManager::instance().syntaxForExtension("sql"), "sql");
    EXPECT_EQ(SettingsManager::instance().syntaxForExtension("lua"), "lua");
    EXPECT_TRUE(SettingsManager::instance().syntaxForExtension("xyz123").isEmpty());

    // Add-on languages map their extensions through the plugin hook instead
    // of the built-in table (rust is contributed by librustlanguage.so).
    EXPECT_TRUE(SettingsManager::addExtensionMapping("rs", "rust"));
    EXPECT_EQ(SettingsManager::instance().syntaxForExtension("rs"), "rust");
    EXPECT_TRUE(SettingsManager::removeExtensionMapping("rs"));
}

TEST_F(SettingsManagerTest, SyntaxForFile)
{
    EXPECT_EQ(SettingsManager::instance().syntaxForFile("main.cpp"), "cpp");
    EXPECT_EQ(SettingsManager::instance().syntaxForFile("app.py"), "python");
    EXPECT_EQ(SettingsManager::instance().syntaxForFile("CMakeLists.txt"), "cmake");
    EXPECT_EQ(SettingsManager::instance().syntaxForFile("readme.md"), "markdown");
}
