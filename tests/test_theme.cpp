#include "settingsmanager.h"
#include "theme.h"

#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class ThemeTest : public ::testing::Test
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

TEST_F(ThemeTest, AllNonSystemThemesHaveUniqueNames)
{
    QStringList names;
    for (auto id : allBuiltInThemes())
    {
        if (id == ThemeId::System)
            continue;
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
    EXPECT_TRUE(getThemeColors(ThemeId::CatppuccinMocha).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::CatppuccinMacchiato).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::CatppuccinFrappe).isDark);
    EXPECT_FALSE(getThemeColors(ThemeId::CatppuccinLatte).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::TokyoNight).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::TokyoNightStorm).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::Dracula).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::OneDark).isDark);
    EXPECT_FALSE(getThemeColors(ThemeId::AyuLight).isDark);
    EXPECT_TRUE(getThemeColors(ThemeId::AyuDark).isDark);
}

TEST_F(ThemeTest, AllColorFieldsAreValid)
{
    for (auto id : allBuiltInThemes())
    {
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
    for (auto id : allBuiltInThemes())
    {
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
    for (auto id : allBuiltInThemes())
    {
        ThemeColors c = getThemeColors(id);

        EXPECT_NE(c.toolbarBg, c.toolbarFg) << "toolbar fg/bg same for " << c.name.toStdString();
        EXPECT_NE(c.selectionBg, c.selectionFg) << "selection fg/bg same for " << c.name.toStdString();
        EXPECT_NE(c.tabBg, c.tabFg) << "tab fg/bg same for " << c.name.toStdString();
    }
}

TEST_F(ThemeTest, AccentColorDiffersFromBackground)
{
    for (auto id : allBuiltInThemes())
    {
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
    EXPECT_TRUE(isThemeDark(ThemeId::CatppuccinMocha));
    EXPECT_FALSE(isThemeDark(ThemeId::CatppuccinLatte));
    EXPECT_TRUE(isThemeDark(ThemeId::TokyoNight));
    EXPECT_TRUE(isThemeDark(ThemeId::Dracula));
    EXPECT_TRUE(isThemeDark(ThemeId::OneDark));
    EXPECT_FALSE(isThemeDark(ThemeId::AyuLight));
    EXPECT_TRUE(isThemeDark(ThemeId::AyuDark));
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
    EXPECT_EQ(themeDisplayName(ThemeId::CatppuccinMocha), QObject::tr("Catppuccin Mocha"));
    EXPECT_EQ(themeDisplayName(ThemeId::CatppuccinLatte), QObject::tr("Catppuccin Latte"));
    EXPECT_EQ(themeDisplayName(ThemeId::TokyoNight), QObject::tr("Tokyo Night"));
    EXPECT_EQ(themeDisplayName(ThemeId::Dracula), QObject::tr("Dracula"));
    EXPECT_EQ(themeDisplayName(ThemeId::OneDark), QObject::tr("One Dark"));
    EXPECT_EQ(themeDisplayName(ThemeId::AyuLight), QObject::tr("Ayu Light"));
    EXPECT_EQ(themeDisplayName(ThemeId::AyuDark), QObject::tr("Ayu Dark"));
}

TEST_F(ThemeTest, PrefersNativeStylingOnlyForSystem)
{
    EXPECT_TRUE(prefersNativeStyling(ThemeId::System));
    EXPECT_FALSE(prefersNativeStyling(ThemeId::Light));
    EXPECT_FALSE(prefersNativeStyling(ThemeId::Dark));
    EXPECT_FALSE(prefersNativeStyling(ThemeId::Nord));
}

TEST_F(ThemeTest, AllBuiltInThemesCount)
{
    int count = builtInThemeCount();
    EXPECT_EQ(count, 17);
    EXPECT_EQ(allBuiltInThemes().size(), count);
}

TEST_F(ThemeTest, ParseThemeJson)
{
    QJsonObject colors;
    colors["background"] = "#1e1e2e";
    colors["foreground"] = "#cdd6f4";
    colors["accent"] = "#cba6f7";

    QJsonObject root;
    root["name"] = "Test Theme";
    root["isDark"] = true;
    root["colors"] = colors;

    ThemeColors tc = parseThemeJson(root);
    EXPECT_EQ(tc.name, "Test Theme");
    EXPECT_TRUE(tc.isDark);
    EXPECT_EQ(tc.background, QColor(30, 30, 46));
    EXPECT_EQ(tc.foreground, QColor(205, 214, 244));
    EXPECT_EQ(tc.accent, QColor(203, 166, 247));
    EXPECT_TRUE(tc.toolbarBg.isValid());
    EXPECT_TRUE(tc.tabBg.isValid());
}

TEST_F(ThemeTest, BuiltInThemeCountMatchesEnum)
{
    int maxEnum = static_cast<int>(ThemeId::AyuDark);
    EXPECT_EQ(builtInThemeCount(), maxEnum + 1);
}
