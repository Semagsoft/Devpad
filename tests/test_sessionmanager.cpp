#include "codeeditor.h"
#include "projectpanel.h"
#include "sessionmanager.h"
#include "settingsmanager.h"
#include "tabmanager.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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

    const QString filePath = QStringLiteral("/path/to/test.cpp");
    CodeEditor* editor = tabManager.createEditor();
    editor->setFileName(filePath);
    editor->setText("test content");
    tabManager.addEditor(editor, "test.cpp");

    mgr.saveSession(&tabManager, &projectPanel, "/path/to");

    QStringList files = mgr.sessionFiles();
    EXPECT_EQ(files.size(), 1);
    // saveSession() canonicalises paths; the expected form is platform-specific
    // (on Windows "/path/to/test.cpp" becomes a drive-qualified absolute path).
    EXPECT_TRUE(files.contains(QFileInfo(filePath).absoluteFilePath())) << files.join(QLatin1String(", ")).toStdString();
}

TEST_F(SessionManagerTest, SaveSessionStoresAbsolutePathForRelativeFileName)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString absPath = dir.filePath(QStringLiteral("file.cpp"));
    QFile file(absPath);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.close();

    // saveSession() canonicalises paths, resolving symlinks. On macOS the temp
    // dir lives under /var, which is a symlink to /private/var, so the path
    // QTemporaryDir reports differs from the one SessionManager stores.
    const QString expectedPath = QFileInfo(absPath).canonicalFilePath();
    ASSERT_FALSE(expectedPath.isEmpty());

    const QString oldCwd = QDir::currentPath();
    ASSERT_TRUE(QDir::setCurrent(dir.path()));
    {
        SessionManager mgr;
        QTabWidget tabWidget;
        TabManager tabManager(&tabWidget);
        ProjectPanel projectPanel;

        CodeEditor* editor = tabManager.createEditor();
        editor->setFileName(QStringLiteral("file.cpp"));
        tabManager.addEditor(editor, QStringLiteral("file.cpp"));

        mgr.saveSession(&tabManager, &projectPanel, "");
        EXPECT_EQ(mgr.sessionFiles(), QStringList{expectedPath});
    }
    QDir::setCurrent(oldCwd);
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

    const QString savedPath = QStringLiteral("/path/to/real.cpp");
    CodeEditor* saved = tabManager.createEditor();
    saved->setFileName(savedPath);
    saved->setText("real");
    tabManager.addEditor(saved, "real.cpp");

    mgr.saveSession(&tabManager, &projectPanel, "");

    QStringList files = mgr.sessionFiles();
    EXPECT_EQ(files.size(), 1);
    EXPECT_TRUE(files.contains(QFileInfo(savedPath).absoluteFilePath())) << files.join(QLatin1String(", ")).toStdString();
    EXPECT_FALSE(files.contains("Untitled"));
}

TEST_F(SessionManagerTest, SaveAndLoadWithBookmarksInSession)
{
    SessionManager mgr;
    QTabWidget tabWidget;
    TabManager tabManager(&tabWidget);
    ProjectPanel projectPanel;

    const QString filePath = QStringLiteral("/path/to/bookmarked.cpp");
    const QString canonicalPath = QFileInfo(filePath).absoluteFilePath();
    CodeEditor* editor = tabManager.createEditor();
    editor->setFileName(filePath);
    editor->setText("line one\nline two\nline three\nline four\nline five\n");
    editor->toggleBookmark(1);
    editor->toggleBookmark(3);
    tabManager.addEditor(editor, "bookmarked.cpp");

    mgr.saveSession(&tabManager, &projectPanel, "");

    EXPECT_EQ(mgr.sessionFiles().size(), 1);
    EXPECT_TRUE(mgr.sessionFiles().contains(canonicalPath)) << mgr.sessionFiles().join(QLatin1String(", ")).toStdString();

    QHash<QString, QList<int>> bookmarks = mgr.loadSessionBookmarks();
    EXPECT_TRUE(bookmarks.contains(canonicalPath));
    EXPECT_EQ(bookmarks[canonicalPath], QList<int>({1, 3}));
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

TEST_F(SessionManagerTest, LoadIgnoresAndPrunesForeignSessionGroups)
{
    // Stale group left behind by a crashed or exited instance
    QSettings raw(QStringLiteral("Semagsoft"), QStringLiteral("Devpad"));
    raw.beginGroup(QStringLiteral("Session_987654"));
    raw.setValue(QStringLiteral("Files"), QStringList{QStringLiteral("/stale/from-crashed.cpp")});
    raw.setValue(QStringLiteral("ActiveIndex"), 0);
    raw.endGroup();
    raw.sync();

    SessionManager mgr;
    SessionManager::SessionData data = mgr.loadSessionData();

    // Files from the crashed/exited instance must not be reopened uninvited
    EXPECT_TRUE(data.files.isEmpty());
    // ...and the stale group must be pruned at load, not resurrected
    EXPECT_FALSE(raw.childGroups().contains(QStringLiteral("Session_987654")));

    mgr.clearSession();
}

TEST_F(SessionManagerTest, LoadFallsBackToLegacyFlatKeys)
{
    QSettings raw(QStringLiteral("Semagsoft"), QStringLiteral("Devpad"));
    raw.setValue(QStringLiteral("Session_Files"), QStringList{QStringLiteral("/prev/a.cpp"), QStringLiteral("/prev/b.cpp")});
    raw.setValue(QStringLiteral("Session_ActiveIndex"), 1);
    raw.setValue(QStringLiteral("Session_ProjectPath"), QStringLiteral("/prev/project"));
    raw.setValue(QStringLiteral("Session_Bookmarks"), QVariantMap{{QStringLiteral("/prev/a.cpp"), QVariantList{3, 7}}});
    raw.sync();

    SessionManager mgr;
    SessionManager::SessionData data = mgr.loadSessionData();

    EXPECT_EQ(data.files, QStringList({QStringLiteral("/prev/a.cpp"), QStringLiteral("/prev/b.cpp")}));
    EXPECT_EQ(data.activeIndex, 1);
    EXPECT_EQ(data.projectPath, QStringLiteral("/prev/project"));
    EXPECT_EQ(data.bookmarks.value(QStringLiteral("/prev/a.cpp")), QList<int>({3, 7}));

    mgr.clearSession();
}

TEST_F(SessionManagerTest, LoadPrefersOwnPidGroupOverForeignContentAndStaleFlatKeys)
{
    QSettings raw(QStringLiteral("Semagsoft"), QStringLiteral("Devpad"));

    // Stale flat snapshot and a foreign PID group
    raw.setValue(QStringLiteral("Session_Files"), QStringList{QStringLiteral("/stale/old.cpp")});
    raw.beginGroup(QStringLiteral("Session_111222"));
    raw.setValue(QStringLiteral("Files"), QStringList{QStringLiteral("/stale/foreign.cpp")});
    raw.endGroup();
    raw.sync();

    SessionManager mgr;
    mgr.saveSessionData({QStringLiteral("/current/fresh.cpp")}, 0, QStringLiteral("/current"));

    // Corrupt the flat mirror with a stale value: the PID group must win
    raw.setValue(QStringLiteral("Session_Files"), QStringList{QStringLiteral("/stale/old.cpp")});
    raw.sync();

    SessionManager::SessionData data = mgr.loadSessionData();

    EXPECT_EQ(data.files, QStringList{QStringLiteral("/current/fresh.cpp")});
    EXPECT_EQ(data.projectPath, QStringLiteral("/current"));
    EXPECT_FALSE(raw.childGroups().contains(QStringLiteral("Session_111222")));

    mgr.clearSession();
}
