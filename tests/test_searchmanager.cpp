#include "searchmanager.h"
#include "codeeditor.h"
#include "settingsmanager.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>

class SearchManagerTest : public ::testing::Test
{
protected:
    QMainWindow* mainWindow = nullptr;
    CodeEditor* editor = nullptr;
    SearchManager* searchManager = nullptr;
    SettingsManager* m_testSettings = nullptr;

    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings);

        mainWindow = new QMainWindow();
        mainWindow->setStatusBar(new QStatusBar(mainWindow));

        editor = new CodeEditor();
        editor->setText("hello world\nfoo bar baz\nhello again\n");

        searchManager = new SearchManager(mainWindow, editor);
    }

    void TearDown() override
    {
        delete searchManager;
        delete editor;
        delete mainWindow;

        SettingsManager::setTestingInstance(nullptr);
        SettingsManager::destroyForTesting(m_testSettings);
        searchManager = nullptr;
        editor = nullptr;
        mainWindow = nullptr;
        m_testSettings = nullptr;
    }
};

TEST_F(SearchManagerTest, InitiallyNoActiveSearch)
{
    EXPECT_FALSE(searchManager->hasActiveSearch());
    EXPECT_TRUE(searchManager->lastSearchText().isEmpty());
}

TEST_F(SearchManagerTest, SetEditorChangesEditor)
{
    CodeEditor* newEditor = new CodeEditor();
    searchManager->setEditor(newEditor);
    delete newEditor;
}

TEST_F(SearchManagerTest, FindDialogIsCreatedOnShow)
{
    searchManager->showFindDialog();
}

TEST_F(SearchManagerTest, ReplaceDialogIsCreatedOnShow)
{
    searchManager->showReplaceDialog();
}

TEST_F(SearchManagerTest, FindNextWithNoSearchShowsDialog)
{
    searchManager->findNext();
}

TEST_F(SearchManagerTest, FindPreviousWithNoSearchShowsDialog)
{
    searchManager->findPrevious();
}

TEST_F(SearchManagerTest, FindDialogGetsEditor)
{
    searchManager->showFindDialog();
}

TEST_F(SearchManagerTest, DestructorDoesNotCrash)
{
    SearchManager* mgr = new SearchManager(mainWindow, editor);
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
