#include "theme.h"
#include "settingsmanager.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QApplication>

class ThemeTest : public ::testing::Test
{
protected:
    SettingsManager* m_testSettings = nullptr;

    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings);
    }

    void TearDown() override
    {
        SettingsManager::setTestingInstance(nullptr);
        SettingsManager::destroyForTesting(m_testSettings);
        m_testSettings = nullptr;
    }
};

TEST_F(ThemeTest, AllNonSystemThemesHaveUniqueNames)
{
    QStringList names;
    QList<ThemeId> explicitThemes = {
        ThemeId::Light, ThemeId::Dark, ThemeId::Nord,
        ThemeId::SolarizedLight, ThemeId::Monokai, ThemeId::GruvboxDark
    };
    for (auto id : explicitThemes)
    {
        ThemeColors c = getThemeColors(id);
        EXPECT_FALSE(c.name.isEmpty());
        EXPECT_FALSE(names.contains(c.name)) << "duplicate name: " << c.name.toStdString();
        names << c.name;
    }
}

TEST_F(ThemeTest, DarkThemesAreDark)
{
    EXPECT_FALSE(getThemeColors(ThemeId::Light).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::Dark).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::Nord).isDark);
    EXPECT_FALSE(getThemeColors(ThemeId::SolarizedLight).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::Monokai).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::GruvboxDark).isDark);
}

TEST_F(ThemeTest, AllColorFieldsAreValid)
{
    for (int i = 0; i <= static_cast<int>(ThemeId::GruvboxDark); ++i)
    {
        ThemeId id = static_cast<ThemeId>(i);
        ThemeColors c = getThemeColors(id);

        EXPECT_TRUE(c.background.isValid()) << "background invalid for " << c.name.toStdString();
        EXPECT_TRUE(c.foreground.isValid()) << "foreground invalid for " << c.name.toStdString();
        EXPECT_TRUE(c.surfaceBg.isValid());
        EXPECT_TRUE(c.surfaceFg.isValid());
        EXPECT_TRUE(c.accent.isValid());
        EXPECT_TRUE(c.caret.isValid());
        EXPECT_TRUE(c.keyword.isValid());
        EXPECT_TRUE(c.string.isValid());
        EXPECT_TRUE(c.number.isValid());
    }
}

TEST_F(ThemeTest, DerivedColorsAreValidAfterResolve)
{
    for (int i = 0; i <= static_cast<int>(ThemeId::GruvboxDark); ++i)
    {
        ThemeId id = static_cast<ThemeId>(i);
        ThemeColors c = getThemeColors(id);

        EXPECT_TRUE(c.toolbarBg.isValid());
        EXPECT_TRUE(c.toolbarFg.isValid());
        EXPECT_TRUE(c.statusbarBg.isValid());
        EXPECT_TRUE(c.statusbarFg.isValid());
        EXPECT_TRUE(c.tabBg.isValid());
        EXPECT_TRUE(c.tabFg.isValid());
        EXPECT_TRUE(c.selectionBg.isValid());
        EXPECT_TRUE(c.selectionFg.isValid());
        EXPECT_TRUE(c.scrollbarBg.isValid());
    }
}

TEST_F(ThemeTest, ResolveComputesContrastingColors)
{
    for (int i = 0; i <= static_cast<int>(ThemeId::GruvboxDark); ++i)
    {
        ThemeId id = static_cast<ThemeId>(i);
        ThemeColors c = getThemeColors(id);

        EXPECT_NE(c.toolbarBg, c.toolbarFg) << "toolbar fg/bg same for " << c.name.toStdString();
        EXPECT_NE(c.selectionBg, c.selectionFg) << "selection fg/bg same for " << c.name.toStdString();
        EXPECT_NE(c.tabBg, c.tabFg) << "tab fg/bg same for " << c.name.toStdString();
    }
}

TEST_F(ThemeTest, AccentColorDiffersFromBackground)
{
    for (int i = 0; i <= static_cast<int>(ThemeId::GruvboxDark); ++i)
    {
        ThemeId id = static_cast<ThemeId>(i);
        ThemeColors c = getThemeColors(id);

        EXPECT_NE(c.accent, c.background) << "accent same as background for " << c.name.toStdString();
    }
}

TEST_F(ThemeTest, GetThemePaletteMapsColorsCorrectly)
{
    ThemeColors c = getThemeColors(ThemeId::Light);
    QPalette palette = getThemePalette(c);

    EXPECT_EQ(palette.color(QPalette::Window), c.dialogBg);
    EXPECT_EQ(palette.color(QPalette::WindowText), c.dialogFg);
    EXPECT_EQ(palette.color(QPalette::Base), c.inputBg);
    EXPECT_EQ(palette.color(QPalette::Text), c.inputFg);
    EXPECT_EQ(palette.color(QPalette::Highlight), c.selectionBg);
    EXPECT_EQ(palette.color(QPalette::HighlightedText), c.selectionFg);
    EXPECT_EQ(palette.color(QPalette::Link), c.accent);
    EXPECT_EQ(palette.color(QPalette::LinkVisited), c.accent);
}

TEST_F(ThemeTest, DarkPaletteHasDimmedDisabledColors)
{
    ThemeColors c = getThemeColors(ThemeId::Dark);
    QPalette palette = getThemePalette(c);

    QColor disabledText = palette.color(QPalette::Disabled, QPalette::WindowText);
    EXPECT_TRUE(disabledText.isValid());
    EXPECT_LT(disabledText.lightness(), 200);
}

TEST_F(ThemeTest, LightPaletteHasLighterDisabledColors)
{
    ThemeColors c = getThemeColors(ThemeId::Light);
    QPalette palette = getThemePalette(c);

    QColor disabledText = palette.color(QPalette::Disabled, QPalette::WindowText);
    EXPECT_TRUE(disabledText.isValid());
    EXPECT_GT(disabledText.lightness(), 100);
}

TEST_F(ThemeTest, IsThemeDarkReturnsCorrectValue)
{
    EXPECT_FALSE(isThemeDark(ThemeId::Light));
    EXPECT_TRUE(isThemeDark(ThemeId::Dark));
    EXPECT_TRUE(isThemeDark(ThemeId::Nord));
    EXPECT_FALSE(isThemeDark(ThemeId::SolarizedLight));
    EXPECT_TRUE(isThemeDark(ThemeId::Monokai));
    EXPECT_TRUE(isThemeDark(ThemeId::GruvboxDark));
}

TEST_F(ThemeTest, ThemeDisplayNames)
{
    EXPECT_EQ(themeDisplayName(ThemeId::Light), QObject::tr("Light"));
    EXPECT_EQ(themeDisplayName(ThemeId::Dark), QObject::tr("Dark"));
    EXPECT_EQ(themeDisplayName(ThemeId::System), QObject::tr("System"));
    EXPECT_EQ(themeDisplayName(ThemeId::Nord), QObject::tr("Nord"));
    EXPECT_EQ(themeDisplayName(ThemeId::Monokai), QObject::tr("Monokai"));
    EXPECT_EQ(themeDisplayName(ThemeId::GruvboxDark), QObject::tr("Gruvbox Dark"));
    EXPECT_EQ(themeDisplayName(ThemeId::SolarizedLight), QObject::tr("Solarized Light"));
}

TEST_F(ThemeTest, PrefersNativeStylingOnlyForSystem)
{
    EXPECT_TRUE(prefersNativeStyling(ThemeId::System));
    EXPECT_FALSE(prefersNativeStyling(ThemeId::Light));
    EXPECT_FALSE(prefersNativeStyling(ThemeId::Dark));
    EXPECT_FALSE(prefersNativeStyling(ThemeId::Nord));
}
