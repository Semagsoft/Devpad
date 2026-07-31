#include "codeeditor.h"
#include "settingsmanager.h"
#include "tabmanager.h"

#include <QApplication>
#include <QTabWidget>

#include <gtest/gtest.h>

class TabManagerTest : public ::testing::Test
{
protected:
    QTabWidget* tabWidget = nullptr;
    TabManager* tabManager = nullptr;
    std::unique_ptr<SettingsManager> m_testSettings;

    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        tabWidget = new QTabWidget();
        tabManager = new TabManager(tabWidget);
    }
    void TearDown() override
    {
        delete tabManager;
        delete tabWidget;
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }

    CodeEditor* addEditor(const QString& title)
    {
        CodeEditor* editor = tabManager->createEditor();
        tabManager->addEditor(editor, title);
        return editor;
    }
};

TEST_F(TabManagerTest, InitiallyEmpty)
{
    EXPECT_EQ(tabManager->count(), 0);
    EXPECT_EQ(tabManager->currentEditor(), nullptr);
}

TEST_F(TabManagerTest, AddEditorIncreasesCount)
{
    addEditor("file1.txt");
    EXPECT_EQ(tabManager->count(), 1);
}

TEST_F(TabManagerTest, CurrentEditorAfterAdd)
{
    CodeEditor* editor = addEditor("test.cpp");
    EXPECT_EQ(tabManager->currentEditor(), editor);
}

TEST_F(TabManagerTest, MultipleEditors)
{
    addEditor("a.txt");
    addEditor("b.txt");
    addEditor("c.txt");
    EXPECT_EQ(tabManager->count(), 3);
}

TEST_F(TabManagerTest, EditorAtIndex)
{
    CodeEditor* e1 = addEditor("first.txt");
    CodeEditor* e2 = addEditor("second.txt");

    EXPECT_EQ(tabManager->editorAt(0), e1);
    EXPECT_EQ(tabManager->editorAt(1), e2);
}

TEST_F(TabManagerTest, IndexOf)
{
    CodeEditor* e1 = addEditor("alpha.txt");
    CodeEditor* e2 = addEditor("beta.txt");

    EXPECT_EQ(tabManager->indexOf(e1), 0);
    EXPECT_EQ(tabManager->indexOf(e2), 1);
}

TEST_F(TabManagerTest, IndexOfUnknownEditor)
{
    CodeEditor orphan;
    EXPECT_EQ(tabManager->indexOf(&orphan), -1);
}

TEST_F(TabManagerTest, FindEditorByFileName)
{
    CodeEditor* e1 = addEditor("findme.txt");
    e1->setFileName("/path/to/findme.txt");

    CodeEditor* found = tabManager->findEditorByFileName("/path/to/findme.txt");
    EXPECT_EQ(found, e1);
}

TEST_F(TabManagerTest, FindEditorByFileNameNotFound)
{
    EXPECT_EQ(tabManager->findEditorByFileName("/nonexistent.txt"), nullptr);
}

TEST_F(TabManagerTest, RemoveEditorDecreasesCount)
{
    CodeEditor* e1 = addEditor("keep.txt");
    addEditor("remove.txt");
    EXPECT_EQ(tabManager->count(), 2);

    tabManager->closeEditor(1);
    EXPECT_EQ(tabManager->count(), 1);
    EXPECT_EQ(tabManager->editorAt(0), e1);
}

TEST_F(TabManagerTest, CloseEditorDecreasesCount)
{
    addEditor("a.txt");
    addEditor("b.txt");
    EXPECT_EQ(tabManager->count(), 2);

    tabManager->closeEditor(0);
    EXPECT_EQ(tabManager->count(), 1);
}

TEST_F(TabManagerTest, TabTitleReflectsFileName)
{
    addEditor("hello.cpp");

    EXPECT_EQ(tabWidget->tabText(0), "hello.cpp");
}

TEST_F(TabManagerTest, TabTitleShowsModified)
{
    CodeEditor* editor = addEditor("modified.cpp");
    editor->setFileName("/path/to/modified.cpp");
    editor->forceModified();
    tabManager->updateTabTitle(editor);

    EXPECT_TRUE(tabWidget->tabText(0).contains("modified.cpp"));
}

TEST_F(TabManagerTest, TabTitleShowsReadOnly)
{
    CodeEditor* editor = addEditor("readonly.txt");
    editor->setReadOnlyMode(true);
    tabManager->updateTabTitle(editor);

    EXPECT_TRUE(tabWidget->tabText(0).contains("Read Only"));
}

TEST_F(TabManagerTest, CurrentIndexReflectsActiveTab)
{
    addEditor("first.txt");
    addEditor("second.txt");

    tabWidget->setCurrentIndex(1);
    EXPECT_EQ(tabManager->currentIndex(), 1);
}

TEST_F(TabManagerTest, ActivePaneIsPrimary)
{
    EXPECT_EQ(tabManager->activePane(), tabWidget);
}

TEST_F(TabManagerTest, PanesListContainsPrimary)
{
    EXPECT_EQ(tabManager->panes().size(), 1);
    EXPECT_EQ(tabManager->panes()[0], tabWidget);
}

TEST_F(TabManagerTest, AddPaneIncreasesPaneCount)
{
    QTabWidget* secondPane = new QTabWidget();
    tabManager->addPane(secondPane);

    EXPECT_EQ(tabManager->panes().size(), 2);

    tabManager->removePane(secondPane);
    delete secondPane;
}

TEST_F(TabManagerTest, RemovePaneDoesNotRemovePrimary)
{
    QTabWidget* secondPane = new QTabWidget();
    tabManager->addPane(secondPane);
    tabManager->removePane(secondPane);

    EXPECT_EQ(tabManager->panes().size(), 1);
    EXPECT_EQ(tabManager->panes()[0], tabWidget);

    delete secondPane;
}

TEST_F(TabManagerTest, SetActivePaneChangesActivePane)
{
    QTabWidget* secondPane = new QTabWidget();
    tabManager->addPane(secondPane);
    tabManager->setActivePane(secondPane);

    EXPECT_EQ(tabManager->activePane(), secondPane);

    delete secondPane;
}

TEST_F(TabManagerTest, FindEditorAcrossPanes)
{
    CodeEditor* e1 = addEditor("pane1.txt");
    e1->setFileName("/root/pane1.txt");

    QTabWidget* secondPane = new QTabWidget();
    tabManager->addPane(secondPane);
    tabManager->setActivePane(secondPane);

    CodeEditor* e2 = tabManager->createEditor();
    e2->setFileName("/root/pane2.txt");
    tabManager->addEditor(e2, "pane2.txt");

    EXPECT_EQ(tabManager->findEditorByFileName("/root/pane1.txt"), e1);
    EXPECT_EQ(tabManager->findEditorByFileName("/root/pane2.txt"), e2);

    tabManager->removePane(secondPane);
    delete secondPane;
}

TEST_F(TabManagerTest, EditorAtWorksAcrossPanes)
{
    CodeEditor* e1 = addEditor("a.txt");
    CodeEditor* e2 = addEditor("b.txt");

    QTabWidget* secondPane = new QTabWidget();
    tabManager->addPane(secondPane);
    tabManager->setActivePane(secondPane);

    CodeEditor* e3 = tabManager->createEditor();
    tabManager->addEditor(e3, "c.txt");

    EXPECT_EQ(tabManager->editorAt(0), e1);
    EXPECT_EQ(tabManager->editorAt(1), e2);
    EXPECT_EQ(tabManager->editorAt(2), e3);

    tabManager->removePane(secondPane);
    delete secondPane;
}
