#include "sessionmanager.h"
#include "codeeditor.h"
#include "settingsmanager.h"
#include "tabmanager.h"
#include "projectpanel.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QApplication>
#include <QTabWidget>

class SessionManagerTest : public ::testing::Test
{
protected:
    std::unique_ptr<SettingsManager> m_testSettings;

    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        SessionManager cleaner;
        cleaner.clearSession();
    }

    void TearDown() override
    {
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }
};

TEST_F(SessionManagerTest, InitiallyNoSessionData)
{
    SessionManager mgr;
    EXPECT_TRUE(mgr.sessionFiles().isEmpty());
    EXPECT_EQ(mgr.sessionActiveIndex(), 0);
    EXPECT_TRUE(mgr.sessionProjectPath().isEmpty());
}

TEST_F(SessionManagerTest, SaveAndLoadSessionFiles)
{
    SessionManager mgr;
    QStringList files = {"/path/to/file1.cpp", "/path/to/file2.py", "/path/to/file3.html"};

    mgr.saveSessionData(files, 1, "/path/to/project", "/path/to/project");

    QStringList loaded = mgr.sessionFiles();
    EXPECT_EQ(loaded, files);
}

TEST_F(SessionManagerTest, SaveAndLoadActiveIndex)
{
    SessionManager mgr;
    mgr.saveSessionData({"a.txt", "b.txt"}, 1, "");

    EXPECT_EQ(mgr.sessionActiveIndex(), 1);
}

TEST_F(SessionManagerTest, SaveAndLoadProjectPath)
{
    SessionManager mgr;
    mgr.saveSessionData({"a.txt"}, 0, "/home/user/myproject");

    EXPECT_EQ(mgr.sessionProjectPath(), "/home/user/myproject");
}

TEST_F(SessionManagerTest, SaveAndLoadEmptyProjectPath)
{
    SessionManager mgr;
    mgr.saveSessionData({"a.txt"}, 0, "");

    EXPECT_TRUE(mgr.sessionProjectPath().isEmpty());
}

TEST_F(SessionManagerTest, ClearSessionRemovesAllData)
{
    SessionManager mgr;
    mgr.saveSessionData({"a.txt", "b.txt"}, 1, "/project");
    EXPECT_FALSE(mgr.sessionFiles().isEmpty());

    mgr.clearSession();

    EXPECT_TRUE(mgr.sessionFiles().isEmpty());
    EXPECT_TRUE(mgr.sessionProjectPath().isEmpty());
    EXPECT_EQ(mgr.sessionActiveIndex(), 0);
}

TEST_F(SessionManagerTest, SaveAndLoadBookmarks)
{
    SessionManager mgr;
    QHash<QString, QList<int>> bookmarks;
    bookmarks["/path/to/file.cpp"] = {5, 10, 15};
    bookmarks["/path/to/file.h"] = {42};

    mgr.saveSessionBookmarks(bookmarks);

    QHash<QString, QList<int>> loaded = mgr.loadSessionBookmarks();
    EXPECT_EQ(loaded.size(), 2);
    EXPECT_TRUE(loaded.contains("/path/to/file.cpp"));
    EXPECT_TRUE(loaded.contains("/path/to/file.h"));
    EXPECT_EQ(loaded["/path/to/file.cpp"], QList<int>({5, 10, 15}));
    EXPECT_EQ(loaded["/path/to/file.h"], QList<int>({42}));
}

TEST_F(SessionManagerTest, BookmarksAreClearedWithSession)
{
    SessionManager mgr;
    QHash<QString, QList<int>> bookmarks;
    bookmarks["/test/file.txt"] = {1, 2, 3};
    mgr.saveSessionBookmarks(bookmarks);

    mgr.clearSession();

    QHash<QString, QList<int>> loaded = mgr.loadSessionBookmarks();
    EXPECT_TRUE(loaded.isEmpty());
}

TEST_F(SessionManagerTest, ActiveIndexDefaultsToZero)
{
    SessionManager mgr;
    EXPECT_EQ(mgr.sessionActiveIndex(), 0);
}

TEST_F(SessionManagerTest, SaveSessionWithTabManager)
{
    SessionManager mgr;
    QTabWidget tabWidget;
    TabManager tabManager(&tabWidget);
    ProjectPanel projectPanel;

    CodeEditor* editor = tabManager.createEditor();
    editor->setFileName("/path/to/test.cpp");
    editor->setText("test content");
    tabManager.addEditor(editor, "test.cpp");

    mgr.saveSession(&tabManager, &projectPanel, "/path/to");

    QStringList files = mgr.sessionFiles();
    EXPECT_EQ(files.size(), 1);
    EXPECT_TRUE(files.contains("/path/to/test.cpp"));
}

TEST_F(SessionManagerTest, SaveSessionSkipsUntitledFiles)
{
    SessionManager mgr;
    QTabWidget tabWidget;
    TabManager tabManager(&tabWidget);
    ProjectPanel projectPanel;

    CodeEditor* untitled = tabManager.createEditor();
    untitled->setText("unsaved");
    tabManager.addEditor(untitled, "Untitled");

    CodeEditor* saved = tabManager.createEditor();
    saved->setFileName("/path/to/real.cpp");
    saved->setText("real");
    tabManager.addEditor(saved, "real.cpp");

    mgr.saveSession(&tabManager, &projectPanel, "");

    QStringList files = mgr.sessionFiles();
    EXPECT_EQ(files.size(), 1);
    EXPECT_TRUE(files.contains("/path/to/real.cpp"));
    EXPECT_FALSE(files.contains("Untitled"));
}

TEST_F(SessionManagerTest, SaveAndLoadWithBookmarksInSession)
{
    SessionManager mgr;
    QTabWidget tabWidget;
    TabManager tabManager(&tabWidget);
    ProjectPanel projectPanel;

    CodeEditor* editor = tabManager.createEditor();
    editor->setFileName("/path/to/bookmarked.cpp");
    editor->setText("line one\nline two\nline three\nline four\nline five\n");
    editor->toggleBookmark(1);
    editor->toggleBookmark(3);
    tabManager.addEditor(editor, "bookmarked.cpp");

    mgr.saveSession(&tabManager, &projectPanel, "");

    EXPECT_EQ(mgr.sessionFiles().size(), 1);
    EXPECT_TRUE(mgr.sessionFiles().contains("/path/to/bookmarked.cpp"));

    QHash<QString, QList<int>> bookmarks = mgr.loadSessionBookmarks();
    EXPECT_TRUE(bookmarks.contains("/path/to/bookmarked.cpp"));
    EXPECT_EQ(bookmarks["/path/to/bookmarked.cpp"], QList<int>({1, 3}));
}

TEST_F(SessionManagerTest, SaveAndLoadTerminalWorkingDir)
{
    SessionManager mgr;
    mgr.saveSessionData({"a.txt"}, 0, "/project", "/project/subdir");

    EXPECT_EQ(mgr.sessionTerminalWorkingDir(), "/project/subdir");
}

TEST_F(SessionManagerTest, SaveAndLoadEmptyTerminalWorkingDir)
{
    SessionManager mgr;
    mgr.saveSessionData({"a.txt"}, 0, "/project");

    EXPECT_TRUE(mgr.sessionTerminalWorkingDir().isEmpty());
}
