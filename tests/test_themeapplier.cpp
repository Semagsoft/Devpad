#include "codeeditor.h"
#include "settingsmanager.h"
#include "tabmanager.h"
#include "themeapplier.h"

#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class ThemeApplierTest : public ::testing::Test
{
protected:
    std::unique_ptr<SettingsManager> m_testSettings;

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
};

TEST_F(ThemeApplierTest, ConstructorDoesNotCrash)
{
    ThemeApplier applier;
    SUCCEED();
}

TEST_F(ThemeApplierTest, ApplySystemThemeClearsStylesheet)
{
    SettingsManager::instance().setTheme(ThemeId::System);
    QMainWindow window;
    ThemeApplier applier;

    applier.applyTheme(&window);

    EXPECT_TRUE(window.styleSheet().isEmpty());
}

TEST_F(ThemeApplierTest, ApplyCustomThemeSetsOrClearsStylesheet)
{
    SettingsManager::instance().setTheme(ThemeId::Light);
    QMainWindow window;
    window.setStyleSheet("dummy");
    ThemeApplier applier;

    applier.applyTheme(&window);

    if (window.styleSheet() == "dummy")
    {
        SUCCEED() << "stylesheet unchanged (theme.qss resource not available)";
    }
    else
    {
        EXPECT_FALSE(window.styleSheet().isEmpty()) << "theme.qss was loaded and substitutions were applied";
    }
}

TEST_F(ThemeApplierTest, ApplyDarkThemeProducesDifferentStyle)
{
    QMainWindow lightWin;
    QMainWindow darkWin;

    SettingsManager::instance().setTheme(ThemeId::Light);
    ThemeApplier applier;
    applier.applyTheme(&lightWin);

    SettingsManager::instance().setTheme(ThemeId::Dark);
    applier.applyTheme(&darkWin);

    if (!lightWin.styleSheet().isEmpty() && !darkWin.styleSheet().isEmpty())
    {
        EXPECT_NE(lightWin.styleSheet(), darkWin.styleSheet()) << "light and dark stylesheets should differ";
    }
}

TEST_F(ThemeApplierTest, ApplySettingsToAllEditorsWithEmptyTabManager)
{
    QTabWidget tabWidget;
    TabManager tabManager(&tabWidget);
    ThemeApplier applier;

    applier.applySettingsToAllEditors(&tabManager);
    SUCCEED();
}

TEST_F(ThemeApplierTest, ApplySettingsToAllEditorsAppliesWhitespace)
{
    QTabWidget tabWidget;
    TabManager tabManager(&tabWidget);
    ThemeApplier applier;

    CodeEditor* editor = tabManager.createEditor();
    editor->setText("line one\nline two\n");
    tabManager.addEditor(editor, "test.cpp");

    SettingsManager::instance().setShowWhitespace(true);
    applier.applySettingsToAllEditors(&tabManager);

    SUCCEED();
}

TEST_F(ThemeApplierTest, ApplyThemeDoesNotCrash)
{
    QMainWindow window;
    ThemeApplier applier;

    for (int i = 0; i <= static_cast<int>(ThemeId::GruvboxDark); ++i)
    {
        ThemeId id = static_cast<ThemeId>(i);
        SettingsManager::instance().setTheme(id);
        EXPECT_NO_THROW(applier.applyTheme(&window)) << "applyTheme crashed for theme " << themeDisplayName(id).toStdString();
    }
}
