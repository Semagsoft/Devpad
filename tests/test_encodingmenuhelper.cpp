#include "encodingmenuhelper.h"

#include <QMenu>

#include <gtest/gtest.h>

class EncodingMenuHelperTest : public ::testing::Test
{
protected:
    EncodingMenuHelper helper;
};

TEST_F(EncodingMenuHelperTest, InitialState)
{
    EXPECT_EQ(helper.reopenMenu(), nullptr);
    EXPECT_EQ(helper.saveMenu(), nullptr);
}

TEST_F(EncodingMenuHelperTest, CreateReopenMenu)
{
    QMenu* menu = helper.createReopenMenu(nullptr);
    ASSERT_NE(menu, nullptr);
    EXPECT_EQ(helper.reopenMenu(), menu);
    EXPECT_EQ(menu->title().toStdString(), "Reopen with Encoding");
}

TEST_F(EncodingMenuHelperTest, CreateSaveMenu)
{
    QMenu* menu = helper.createSaveMenu(nullptr);
    ASSERT_NE(menu, nullptr);
    EXPECT_EQ(helper.saveMenu(), menu);
    EXPECT_EQ(menu->title().toStdString(), "Save with Encoding");
}

TEST_F(EncodingMenuHelperTest, BothMenusAreDistinct)
{
    QMenu* reopen = helper.createReopenMenu(nullptr);
    QMenu* save = helper.createSaveMenu(nullptr);
    EXPECT_NE(reopen, save);
    EXPECT_EQ(helper.reopenMenu(), reopen);
    EXPECT_EQ(helper.saveMenu(), save);
}

TEST_F(EncodingMenuHelperTest, BothMenusAreInitiallyEmpty)
{
    helper.createReopenMenu(nullptr);
    helper.createSaveMenu(nullptr);
    EXPECT_TRUE(helper.reopenMenu()->actions().isEmpty());
    EXPECT_TRUE(helper.saveMenu()->actions().isEmpty());
}
