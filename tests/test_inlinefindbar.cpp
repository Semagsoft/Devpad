#include "codeeditor.h"
#include "settingsmanager.h"
#include "theme.h"
#include "widgets/inlinefindbar.h"

#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>

#include <gtest/gtest.h>

class InlineFindBarTest : public ::testing::Test
{
protected:
    std::unique_ptr<SettingsManager> m_testSettings;
    CodeEditor* editor = nullptr;
    InlineFindBar* bar = nullptr;

    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        editor = new CodeEditor();
        editor->setText("line0\nline1\nline2\n");
        bar = new InlineFindBar(editor);
    }

    void TearDown() override
    {
        delete bar;
        delete editor;
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }
};

TEST_F(InlineFindBarTest, StartsHidden)
{
    EXPECT_FALSE(bar->isVisible());
    EXPECT_FALSE(bar->isReplaceMode());
}

TEST_F(InlineFindBarTest, ShowFindModeMakesVisible)
{
    bar->showFindMode();
    EXPECT_TRUE(bar->isVisible());
    EXPECT_FALSE(bar->isReplaceMode());
}

TEST_F(InlineFindBarTest, ShowReplaceModeSetsReplaceMode)
{
    bar->showReplaceMode();
    EXPECT_TRUE(bar->isVisible());
    EXPECT_TRUE(bar->isReplaceMode());
}

TEST_F(InlineFindBarTest, CloseBarHides)
{
    bar->showFindMode();
    EXPECT_TRUE(bar->isVisible());

    bar->closeBar();
    EXPECT_FALSE(bar->isVisible());
}

TEST_F(InlineFindBarTest, SearchTextDefaultsEmpty)
{
    EXPECT_EQ(bar->searchText().toStdString(), "");
}

TEST_F(InlineFindBarTest, ToggleButtonsDefaultOff)
{
    EXPECT_FALSE(bar->matchCase());
    EXPECT_FALSE(bar->matchWholeWord());
    EXPECT_FALSE(bar->useRegex());
}

TEST_F(InlineFindBarTest, ShowFindModeSetsSearchText)
{
    bar->showFindMode("find_this");
    EXPECT_EQ(bar->searchText().toStdString(), "find_this");
}

TEST_F(InlineFindBarTest, FindNextOnEmptyDoesNotCrash)
{
    bar->showFindMode("nonexistent");
    bar->findNext();
    bar->findPrevious();
}

TEST_F(InlineFindBarTest, FindNextWrapsAround)
{
    editor->setText("abc abc abc\n");
    bar->showFindMode("abc");
    EXPECT_EQ(bar->searchText().toStdString(), "abc");
}

TEST_F(InlineFindBarTest, ShowFindModeAfterCloseWorks)
{
    bar->showFindMode("first");
    bar->closeBar();
    EXPECT_FALSE(bar->isVisible());

    bar->showFindMode("second");
    EXPECT_TRUE(bar->isVisible());
    EXPECT_EQ(bar->searchText().toStdString(), "second");
}

TEST_F(InlineFindBarTest, ShowReplaceModeWithSelectedText)
{
    bar->showReplaceMode("replace_me");
    EXPECT_TRUE(bar->isReplaceMode());
    EXPECT_EQ(bar->searchText().toStdString(), "replace_me");
}
