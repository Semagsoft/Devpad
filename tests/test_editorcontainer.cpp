#include "codeeditor.h"
#include "widgets/editorcontainer.h"
#include "widgets/inlinefindbar.h"
#include "settingsmanager.h"

#include <gtest/gtest.h>

class EditorContainerTest : public ::testing::Test
{
protected:
    std::unique_ptr<SettingsManager> m_testSettings;
    CodeEditor* editor = nullptr;
    EditorContainer* container = nullptr;

    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        editor = new CodeEditor();
        container = new EditorContainer(editor);
        container->resize(400, 300);
    }

    void TearDown() override
    {
        // Container owns the editor via Qt parent-child, so only delete container
        delete container;
        editor = nullptr;
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }
};

TEST_F(EditorContainerTest, ConstructorCreatesEditorAndFindBar)
{
    EXPECT_EQ(container->editor(), editor);
    ASSERT_NE(container->findBar(), nullptr);
    EXPECT_FALSE(container->findBar()->isVisible());
}

TEST_F(EditorContainerTest, ShowFindBarMakesFindBarVisible)
{
    container->show();
    container->showFindBar();
    EXPECT_TRUE(container->isFindBarVisible());
}

TEST_F(EditorContainerTest, ShowReplaceBarMakesFindBarVisibleInReplaceMode)
{
    container->show();
    container->showReplaceBar();
    EXPECT_TRUE(container->isFindBarVisible());
    EXPECT_TRUE(container->findBar()->isReplaceMode());
}

TEST_F(EditorContainerTest, HideFindBarHidesIt)
{
    container->show();
    container->showFindBar();
    EXPECT_TRUE(container->isFindBarVisible());

    container->hideFindBar();
    EXPECT_FALSE(container->isFindBarVisible());
}

TEST_F(EditorContainerTest, HideFindBarViaCloseBar)
{
    container->show();
    container->showFindBar();
    EXPECT_TRUE(container->isFindBarVisible());

    container->findBar()->closeBar();
    EXPECT_FALSE(container->isFindBarVisible());
}

TEST_F(EditorContainerTest, FindBarStartsHidden)
{
    EXPECT_FALSE(container->findBar()->isVisible());
}
