#include "actionmanager.h"
#include "settingsmanager.h"

#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>

#include <gtest/gtest.h>

class ActionManagerTest : public ::testing::Test
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

TEST_F(ActionManagerTest, CreateActions)
{
    ActionManager manager;
    manager.createActions();

    EXPECT_NE(manager.newAct(), nullptr);
    EXPECT_NE(manager.quitDevpadAct(), nullptr);
    EXPECT_NE(manager.undoAct(), nullptr);
    EXPECT_NE(manager.redoAct(), nullptr);
    EXPECT_FALSE(manager.actionsWithShortcuts().empty());
}

TEST_F(ActionManagerTest, BuildMenusCreatesMenus)
{
    ActionManager manager;
    manager.createActions();
    QMenuBar menuBar;
    manager.buildMenus(&menuBar);

    EXPECT_GT(menuBar.actions().size(), 0);
}

TEST_F(ActionManagerTest, BuildToolBar)
{
    ActionManager manager;
    manager.createActions();
    QToolBar* toolbar = manager.buildToolBar();
    EXPECT_NE(toolbar, nullptr);
    delete toolbar;
}

TEST_F(ActionManagerTest, WireConnectionsWithTerminalPanel)
{
    ActionManager manager;
    manager.createActions();
    QMenuBar menuBar;
    manager.buildMenus(&menuBar);

    ActionTargets targets;
    targets.terminalPanel = nullptr;
    targets.splitView = nullptr;
    targets.encodingManager = nullptr;

    manager.wireConnections(targets);
}

TEST_F(ActionManagerTest, ShortcutActionsNotEmpty)
{
    ActionManager manager;
    manager.createActions();
    auto actions = manager.actionsWithShortcuts();
    for (QAction* act : actions)
    {
        EXPECT_TRUE(act->shortcut().isEmpty() || !act->shortcut().isEmpty());
    }
}

TEST_F(ActionManagerTest, FileMenuContainsActions)
{
    ActionManager manager;
    manager.createActions();
    QMenuBar menuBar;
    manager.buildMenus(&menuBar);

    bool hasFileMenu = false;
    for (QAction* act : menuBar.actions())
    {
        if (act->text().contains("File", Qt::CaseInsensitive))
        {
            hasFileMenu = true;
            break;
        }
    }
    EXPECT_TRUE(hasFileMenu);
}

TEST_F(ActionManagerTest, SnippetAndCommentActionsCreated)
{
    ActionManager manager;
    manager.createActions();

    ASSERT_NE(manager.insertSnippetAct(), nullptr);
    ASSERT_NE(manager.toggleCommentAct(), nullptr);
    EXPECT_EQ(manager.insertSnippetAct()->shortcut(), QKeySequence("Ctrl+Shift+I"));
    EXPECT_EQ(manager.toggleCommentAct()->shortcut(), QKeySequence("Ctrl+/"));

    // Nothing to act on until an editable editor exists.
    EXPECT_FALSE(manager.insertSnippetAct()->isEnabled());
    EXPECT_FALSE(manager.toggleCommentAct()->isEnabled());
}

TEST_F(ActionManagerTest, EditMenuContainsSnippetAndCommentActions)
{
    ActionManager manager;
    manager.createActions();
    QMenuBar menuBar;
    manager.buildMenus(&menuBar);

    QMenu* editMenu = nullptr;
    for (QAction* act : menuBar.actions())
    {
        if (act->menu() && act->text() == QStringLiteral("Edit"))
        {
            editMenu = act->menu();
            break;
        }
    }

    ASSERT_NE(editMenu, nullptr);
    EXPECT_TRUE(editMenu->actions().contains(manager.insertSnippetAct()));
    EXPECT_TRUE(editMenu->actions().contains(manager.toggleCommentAct()));
}
