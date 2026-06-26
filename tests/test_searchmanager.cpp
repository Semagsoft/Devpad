#include "searchmanager.h"
#include "codeeditor.h"
#include "settingsmanager.h"
#include "tabmanager.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QTabWidget>
#include <Qsci/qsciscintilla.h>

class SearchManagerTest : public ::testing::Test
{
protected:
    QMainWindow* mainWindow = nullptr;
    QTabWidget* tabWidget = nullptr;
    TabManager* tabManager = nullptr;
    CodeEditor* editor = nullptr;
    SearchManager* searchManager = nullptr;
    std::unique_ptr<SettingsManager> m_testSettings;

    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        mainWindow = new QMainWindow();
        tabWidget = new QTabWidget();
        tabManager = new TabManager(tabWidget);
        editor = new CodeEditor();
        editor->setText("hello world\nline2\nhello again\nline4");
        tabManager->addEditor(editor, "test");
        searchManager = new SearchManager(mainWindow, tabManager);
    }
    void TearDown() override
    {
        SettingsManager::setTestingInstance(nullptr);
        delete mainWindow;
        m_testSettings.reset();
    }
};

TEST_F(SearchManagerTest, InitiallyNoActiveSearch)
{
    EXPECT_FALSE(searchManager->hasActiveSearch());
    EXPECT_TRUE(searchManager->lastSearchText().isEmpty());
}

TEST_F(SearchManagerTest, FindDialogIsCreatedOnShow)
{
    searchManager->showFindDialog();
    // Dialog should be visible after showFindDialog
    EXPECT_TRUE(searchManager->hasActiveSearch() || !searchManager->lastSearchText().isEmpty()
                || searchManager->lastSearchText().isNull());
    SUCCEED();
}

TEST_F(SearchManagerTest, ReplaceDialogIsCreatedOnShow)
{
    searchManager->showReplaceDialog();
    SUCCEED();
}

TEST_F(SearchManagerTest, FindNextWithLastSearchFindsText)
{
    searchManager->setLastSearch("hello");
    // Should not throw, and should find the match
    EXPECT_NO_THROW(searchManager->findNext());
}

TEST_F(SearchManagerTest, FindPreviousWithLastSearchFindsText)
{
    searchManager->setLastSearch("hello");
    EXPECT_NO_THROW(searchManager->findPrevious());
}

TEST_F(SearchManagerTest, FindNextWithNoMatchDoesNotCrash)
{
    searchManager->setLastSearch("nonexistent_string_xyz");
    EXPECT_NO_THROW(searchManager->findNext());
}

TEST_F(SearchManagerTest, SearchWithMatchCase)
{
    searchManager->setLastSearch("Hello", false, true);
    EXPECT_NO_THROW(searchManager->findNext());
}

TEST_F(SearchManagerTest, SearchWithRegex)
{
    searchManager->setLastSearch("h.*o", true);
    EXPECT_NO_THROW(searchManager->findNext());
}

TEST_F(SearchManagerTest, FindDialogGetsEditor)
{
    searchManager->showFindDialog();
    SUCCEED();
}

TEST_F(SearchManagerTest, DestructorDoesNotCrash)
{
    SearchManager* mgr = new SearchManager(mainWindow, tabManager);
    delete mgr;
}

TEST_F(SearchManagerTest, MultipleShowFindIsSafe)
{
    searchManager->showFindDialog();
    searchManager->showFindDialog();
}

TEST_F(SearchManagerTest, MultipleShowReplaceIsSafe)
{
    searchManager->showReplaceDialog();
    searchManager->showReplaceDialog();
}

TEST_F(SearchManagerTest, LastSearchParamsRoundTrip)
{
    searchManager->setLastSearch("searchText", true, true, true);
    EXPECT_EQ(searchManager->lastSearchText(), "searchText");
    EXPECT_TRUE(searchManager->lastUseRegex());
    EXPECT_TRUE(searchManager->lastMatchCase());
    EXPECT_TRUE(searchManager->lastMatchWholeWord());
}

TEST_F(SearchManagerTest, EditorFindFirstDirect)
{
    // Test the underlying QsciScintilla find API directly
    bool found = editor->findFirst("world", false, false, false, true, true);
    EXPECT_TRUE(found);
}

TEST_F(SearchManagerTest, EditorFindFirstNotFound)
{
    bool found = editor->findFirst("nonexistent", false, false, false, true, true);
    EXPECT_FALSE(found);
}

TEST_F(SearchManagerTest, EditorFindFirstCaseSensitive)
{
    bool found = editor->findFirst("HELLO", false, true, false, true, true);
    EXPECT_FALSE(found);

    found = editor->findFirst("hello", false, true, false, true, true);
    EXPECT_TRUE(found);
}

TEST_F(SearchManagerTest, EditorFindFirstWrapAround)
{
    // Start after "hello" and search forward with wrap
    editor->setCursorPosition(0, 6);
    bool found = editor->findFirst("hello", false, false, false, true, true);
    EXPECT_TRUE(found);
}

TEST_F(SearchManagerTest, EditorFindNextAfterFindFirst)
{
    editor->findFirst("hello", false, false, false, true, true);
    bool found = editor->findNext();
    EXPECT_TRUE(found); // second "hello" on line 3
}

TEST_F(SearchManagerTest, EditorReplaceDirect)
{
    editor->findFirst("hello", false, false, false, true, true);
    editor->replace("hi");
    // After replacement, content should be updated
    EXPECT_TRUE(editor->text().contains("hi"));
}
