#include "encodingmanager.h"
#include "codeeditor.h"
#include "settingsmanager.h"

#include <gtest/gtest.h>
#include <QComboBox>
#include <QMenu>

class EncodingManagerTest : public ::testing::Test
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

TEST_F(EncodingManagerTest, PopulateEncodingMenu)
{
    EncodingManager manager;
    QMenu menu;
    manager.populateEncodingMenu(&menu, true);
    EXPECT_GT(menu.actions().size(), 0);
}

TEST_F(EncodingManagerTest, PopulateEncodingMenuHasUtf8)
{
    EncodingManager manager;
    QMenu menu;
    manager.populateEncodingMenu(&menu, false);
    bool foundUtf8 = false;
    for (QAction* act : menu.actions())
    {
        if (act->text().contains("UTF-8"))
        {
            foundUtf8 = true;
            break;
        }
    }
    EXPECT_TRUE(foundUtf8);
}

TEST_F(EncodingManagerTest, UpdateEncodingSelector)
{
    EncodingManager manager;
    QComboBox combo;
    CodeEditor editor;
    editor.setEncoding("UTF-8");
    manager.updateEncodingSelector(&combo, &editor);
    EXPECT_GE(combo.count(), 1);
}

TEST_F(EncodingManagerTest, UpdateEncodingSelectorSetsCurrent)
{
    EncodingManager manager;
    QComboBox combo;
    manager.updateEncodingSelector(&combo, nullptr);
    // No editor means no selection change
    EXPECT_EQ(combo.currentIndex(), -1);
}
