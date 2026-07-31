#include "recentfileshelper.h"

#include <QAction>
#include <QMenu>
#include <QSignalSpy>

#include <gtest/gtest.h>

class RecentFilesHelperTest : public ::testing::Test
{
protected:
    RecentFilesHelper helper;
};

TEST_F(RecentFilesHelperTest, InitialState)
{
    EXPECT_EQ(helper.menu(), nullptr);
    EXPECT_EQ(helper.clearAction(), nullptr);
    for (int i = 0; i < 10; ++i)
        EXPECT_EQ(helper.fileAction(i), nullptr);
}

TEST_F(RecentFilesHelperTest, CreateMenuReturnsMenu)
{
    QMenu* menu = helper.createMenu(nullptr);
    EXPECT_EQ(helper.menu(), menu);
    EXPECT_FALSE(menu->actions().isEmpty());
    EXPECT_EQ(menu->title().toStdString(), "Recent Files");
}

TEST_F(RecentFilesHelperTest, FileActionsAccessibleAfterCreate)
{
    helper.createMenu(nullptr);
    for (int i = 0; i < 10; ++i)
    {
        QAction* act = helper.fileAction(i);
        ASSERT_NE(act, nullptr);
        EXPECT_FALSE(act->isVisible());
    }
}

TEST_F(RecentFilesHelperTest, FileActionOutOfRangeReturnsNull)
{
    helper.createMenu(nullptr);
    EXPECT_EQ(helper.fileAction(-1), nullptr);
    EXPECT_EQ(helper.fileAction(10), nullptr);
    EXPECT_EQ(helper.fileAction(100), nullptr);
}

TEST_F(RecentFilesHelperTest, ClearActionExists)
{
    helper.createMenu(nullptr);
    ASSERT_NE(helper.clearAction(), nullptr);
    EXPECT_EQ(helper.clearAction()->text().toStdString(), "Clear Recent Files");
}

TEST_F(RecentFilesHelperTest, SignalEmittedOnFileActionTrigger)
{
    helper.createMenu(nullptr);
    QSignalSpy spy(&helper, &RecentFilesHelper::openRecentFileTriggered);

    QAction* act = helper.fileAction(0);
    act->setData("/path/to/file.cpp");
    act->trigger();

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.at(0).at(0).toString().toStdString(), "/path/to/file.cpp");
}

TEST_F(RecentFilesHelperTest, SignalEmittedOnClear)
{
    helper.createMenu(nullptr);
    QSignalSpy spy(&helper, &RecentFilesHelper::clearRecentFilesTriggered);

    helper.clearAction()->trigger();
    ASSERT_EQ(spy.count(), 1);
}

TEST_F(RecentFilesHelperTest, MenuHasActionsAndSeparator)
{
    QMenu* menu = helper.createMenu(nullptr);
    QList<QAction*> actions = menu->actions();
    ASSERT_GE(actions.size(), 11);
    for (int i = 0; i < 10; ++i)
        EXPECT_EQ(actions[i], helper.fileAction(i));
    ASSERT_TRUE(actions[10]->isSeparator());
    ASSERT_GE(actions.size(), 12);
    EXPECT_EQ(actions[11], helper.clearAction());
}
