/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "theme.h"
#include <QApplication>
#include <QStyleHints>
#include <array>

static ThemeColors createLightTheme() {
    ThemeColors c;
    c.name = "Light";
    c.isDark = false;

    c.background = QColor(255, 255, 255);
    c.foreground = QColor(0, 0, 0);
    c.toolbarBg = QColor(245, 245, 245);
    c.toolbarFg = QColor(0, 0, 0);
    c.menuBg = QColor(252, 252, 252);
    c.menuFg = QColor(0, 0, 0);
    c.statusbarBg = QColor(240, 240, 240);
    c.statusbarFg = QColor(60, 60, 60);
    c.tabBg = QColor(235, 235, 235);
    c.tabBgActive = QColor(255, 255, 255);
    c.tabFg = QColor(80, 80, 80);
    c.tabFgActive = QColor(0, 0, 0);
    c.tabBorder = QColor(200, 200, 200);
    c.scrollbarBg = QColor(240, 240, 240);
    c.scrollbarHandle = QColor(190, 190, 190);
    c.scrollbarHandleHover = QColor(160, 160, 160);
    c.selectionBg = QColor(51, 153, 255);
    c.selectionFg = QColor(255, 255, 255);
    c.caret = QColor(0, 0, 0);
    c.lineHighlight = QColor(232, 242, 254);
    c.marginBg = QColor(240, 240, 240);
    c.marginFg = QColor(100, 100, 100);
    c.foldMarginBg = QColor(220, 220, 220);
    c.foldMarginBgAlt = QColor(200, 200, 200);
    c.matchedBraceBg = QColor(255, 255, 200);
    c.matchedBraceFg = QColor(0, 100, 0);
    c.comment = QColor(0, 128, 0);
    c.keyword = QColor(0, 0, 255);
    c.string = QColor(163, 21, 21);
    c.number = QColor(0, 100, 0);
    c.operator_ = QColor(0, 0, 0);
    c.function = QColor(0, 0, 128);
    c.preprocessor = QColor(128, 0, 128);
    c.dialogBg = QColor(252, 252, 252);
    c.dialogFg = QColor(0, 0, 0);
    c.inputBg = QColor(255, 255, 255);
    c.inputFg = QColor(0, 0, 0);
    c.buttonBg = QColor(240, 240, 240);
    c.buttonFg = QColor(0, 0, 0);
    c.groupboxBg = QColor(252, 252, 252);
    c.groupboxFg = QColor(0, 0, 0);
    c.checkboxIndicator = QColor(0, 0, 0);
    c.separator = QColor(210, 210, 210);
    return c;
}

static ThemeColors createDarkTheme() {
    ThemeColors c;
    c.name = "Dark";
    c.isDark = true;

    c.background = QColor(30, 34, 42);
    c.foreground = QColor(220, 220, 220);
    c.toolbarBg = QColor(45, 49, 58);
    c.toolbarFg = QColor(220, 220, 220);
    c.menuBg = QColor(40, 44, 52);
    c.menuFg = QColor(220, 220, 220);
    c.statusbarBg = QColor(40, 44, 52);
    c.statusbarFg = QColor(180, 180, 180);
    c.tabBg = QColor(35, 39, 47);
    c.tabBgActive = QColor(50, 54, 62);
    c.tabFg = QColor(160, 160, 160);
    c.tabFgActive = QColor(220, 220, 220);
    c.tabBorder = QColor(55, 59, 67);
    c.scrollbarBg = QColor(40, 44, 52);
    c.scrollbarHandle = QColor(80, 84, 92);
    c.scrollbarHandleHover = QColor(100, 104, 112);
    c.selectionBg = QColor(60, 100, 160);
    c.selectionFg = QColor(255, 255, 255);
    c.caret = QColor(240, 240, 240);
    c.lineHighlight = QColor(45, 50, 60);
    c.marginBg = QColor(40, 44, 52);
    c.marginFg = QColor(180, 180, 180);
    c.foldMarginBg = QColor(50, 54, 62);
    c.foldMarginBgAlt = QColor(40, 44, 52);
    c.matchedBraceBg = QColor(60, 64, 72);
    c.matchedBraceFg = QColor(255, 200, 50);
    c.comment = QColor(80, 200, 80);
    c.keyword = QColor(86, 156, 255);
    c.string = QColor(214, 157, 90);
    c.number = QColor(181, 206, 168);
    c.operator_ = QColor(180, 180, 180);
    c.function = QColor(220, 220, 120);
    c.preprocessor = QColor(220, 130, 170);
    c.dialogBg = QColor(40, 44, 52);
    c.dialogFg = QColor(220, 220, 220);
    c.inputBg = QColor(50, 54, 62);
    c.inputFg = QColor(220, 220, 220);
    c.buttonBg = QColor(55, 59, 67);
    c.buttonFg = QColor(220, 220, 220);
    c.groupboxBg = QColor(40, 44, 52);
    c.groupboxFg = QColor(220, 220, 220);
    c.checkboxIndicator = QColor(220, 220, 220);
    c.separator = QColor(60, 64, 72);
    return c;
}

static ThemeColors createNordTheme() {
    ThemeColors c;
    c.name = "Nord";
    c.isDark = true;

    c.background = QColor(46, 52, 64);
    c.foreground = QColor(216, 222, 233);
    c.toolbarBg = QColor(59, 66, 82);
    c.toolbarFg = QColor(216, 222, 233);
    c.menuBg = QColor(59, 66, 82);
    c.menuFg = QColor(216, 222, 233);
    c.statusbarBg = QColor(59, 66, 82);
    c.statusbarFg = QColor(160, 170, 190);
    c.tabBg = QColor(59, 66, 82);
    c.tabBgActive = QColor(67, 76, 94);
    c.tabFg = QColor(160, 170, 190);
    c.tabFgActive = QColor(216, 222, 233);
    c.tabBorder = QColor(76, 86, 106);
    c.scrollbarBg = QColor(46, 52, 64);
    c.scrollbarHandle = QColor(76, 86, 106);
    c.scrollbarHandleHover = QColor(96, 106, 126);
    c.selectionBg = QColor(136, 192, 208);
    c.selectionFg = QColor(46, 52, 64);
    c.caret = QColor(229, 233, 240);
    c.lineHighlight = QColor(59, 66, 82);
    c.marginBg = QColor(40, 44, 52);
    c.marginFg = QColor(180, 190, 200);
    c.foldMarginBg = QColor(50, 54, 62);
    c.foldMarginBgAlt = QColor(46, 52, 64);
    c.matchedBraceBg = QColor(67, 76, 94);
    c.matchedBraceFg = QColor(136, 192, 208);
    c.comment = QColor(76, 86, 106);
    c.keyword = QColor(129, 161, 193);
    c.string = QColor(163, 190, 140);
    c.number = QColor(181, 189, 104);
    c.operator_ = QColor(229, 233, 240);
    c.function = QColor(136, 192, 208);
    c.preprocessor = QColor(191, 97, 106);
    c.dialogBg = QColor(59, 66, 82);
    c.dialogFg = QColor(216, 222, 233);
    c.inputBg = QColor(67, 76, 94);
    c.inputFg = QColor(216, 222, 233);
    c.buttonBg = QColor(76, 86, 106);
    c.buttonFg = QColor(216, 222, 233);
    c.groupboxBg = QColor(59, 66, 82);
    c.groupboxFg = QColor(216, 222, 233);
    c.checkboxIndicator = QColor(136, 192, 208);
    c.separator = QColor(76, 86, 106);
    return c;
}

static ThemeColors createSolarizedLightTheme() {
    ThemeColors c;
    c.name = "Solarized Light";
    c.isDark = false;

    c.background = QColor(253, 246, 227);
    c.foreground = QColor(101, 123, 131);
    c.toolbarBg = QColor(238, 232, 213);
    c.toolbarFg = QColor(101, 123, 131);
    c.menuBg = QColor(253, 246, 227);
    c.menuFg = QColor(101, 123, 131);
    c.statusbarBg = QColor(238, 232, 213);
    c.statusbarFg = QColor(88, 110, 117);
    c.tabBg = QColor(238, 232, 213);
    c.tabBgActive = QColor(253, 246, 227);
    c.tabFg = QColor(101, 123, 131);
    c.tabFgActive = QColor(0, 43, 54);
    c.tabBorder = QColor(211, 204, 186);
    c.scrollbarBg = QColor(238, 232, 213);
    c.scrollbarHandle = QColor(196, 189, 171);
    c.scrollbarHandleHover = QColor(176, 169, 151);
    c.selectionBg = QColor(38, 139, 210);
    c.selectionFg = QColor(253, 246, 227);
    c.caret = QColor(7, 54, 66);
    c.lineHighlight = QColor(238, 232, 213);
    c.marginBg = QColor(238, 232, 213);
    c.marginFg = QColor(101, 123, 131);
    c.foldMarginBg = QColor(228, 222, 203);
    c.foldMarginBgAlt = QColor(218, 212, 193);
    c.matchedBraceBg = QColor(238, 232, 213);
    c.matchedBraceFg = QColor(211, 54, 130);
    c.comment = QColor(147, 161, 161);
    c.keyword = QColor(38, 139, 210);
    c.string = QColor(42, 161, 152);
    c.number = QColor(42, 161, 152);
    c.operator_ = QColor(101, 123, 131);
    c.function = QColor(38, 139, 210);
    c.preprocessor = QColor(211, 54, 130);
    c.dialogBg = QColor(253, 246, 227);
    c.dialogFg = QColor(101, 123, 131);
    c.inputBg = QColor(238, 232, 213);
    c.inputFg = QColor(101, 123, 131);
    c.buttonBg = QColor(211, 204, 186);
    c.buttonFg = QColor(101, 123, 131);
    c.groupboxBg = QColor(253, 246, 227);
    c.groupboxFg = QColor(101, 123, 131);
    c.checkboxIndicator = QColor(101, 123, 131);
    c.separator = QColor(211, 204, 186);
    return c;
}

static ThemeColors createMonokaiTheme() {
    ThemeColors c;
    c.name = "Monokai";
    c.isDark = true;

    c.background = QColor(39, 40, 34);
    c.foreground = QColor(248, 248, 242);
    c.toolbarBg = QColor(52, 53, 46);
    c.toolbarFg = QColor(248, 248, 242);
    c.menuBg = QColor(45, 46, 40);
    c.menuFg = QColor(248, 248, 242);
    c.statusbarBg = QColor(45, 46, 40);
    c.statusbarFg = QColor(180, 180, 175);
    c.tabBg = QColor(45, 46, 40);
    c.tabBgActive = QColor(73, 72, 62);
    c.tabFg = QColor(160, 160, 155);
    c.tabFgActive = QColor(248, 248, 242);
    c.tabBorder = QColor(62, 61, 52);
    c.scrollbarBg = QColor(39, 40, 34);
    c.scrollbarHandle = QColor(72, 73, 66);
    c.scrollbarHandleHover = QColor(92, 93, 86);
    c.selectionBg = QColor(72, 73, 66);
    c.selectionFg = QColor(248, 248, 242);
    c.caret = QColor(248, 248, 240);
    c.lineHighlight = QColor(62, 61, 52);
    c.marginBg = QColor(45, 46, 40);
    c.marginFg = QColor(180, 180, 175);
    c.foldMarginBg = QColor(52, 53, 46);
    c.foldMarginBgAlt = QColor(45, 46, 40);
    c.matchedBraceBg = QColor(72, 73, 66);
    c.matchedBraceFg = QColor(248, 248, 242);
    c.comment = QColor(117, 113, 94);
    c.keyword = QColor(249, 38, 114);
    c.string = QColor(230, 219, 116);
    c.number = QColor(174, 129, 255);
    c.operator_ = QColor(248, 248, 242);
    c.function = QColor(166, 226, 46);
    c.preprocessor = QColor(249, 38, 114);
    c.dialogBg = QColor(45, 46, 40);
    c.dialogFg = QColor(248, 248, 242);
    c.inputBg = QColor(52, 53, 46);
    c.inputFg = QColor(248, 248, 242);
    c.buttonBg = QColor(62, 61, 52);
    c.buttonFg = QColor(248, 248, 242);
    c.groupboxBg = QColor(45, 46, 40);
    c.groupboxFg = QColor(248, 248, 242);
    c.checkboxIndicator = QColor(166, 226, 46);
    c.separator = QColor(62, 61, 52);
    return c;
}

static ThemeColors createGruvboxDarkTheme() {
    ThemeColors c;
    c.name = "Gruvbox Dark";
    c.isDark = true;

    c.background = QColor(40, 40, 40);
    c.foreground = QColor(235, 219, 178);
    c.toolbarBg = QColor(60, 56, 54);
    c.toolbarFg = QColor(235, 219, 178);
    c.menuBg = QColor(50, 48, 47);
    c.menuFg = QColor(235, 219, 178);
    c.statusbarBg = QColor(50, 48, 47);
    c.statusbarFg = QColor(168, 153, 132);
    c.tabBg = QColor(50, 48, 47);
    c.tabBgActive = QColor(80, 73, 69);
    c.tabFg = QColor(168, 153, 132);
    c.tabFgActive = QColor(235, 219, 178);
    c.tabBorder = QColor(80, 73, 69);
    c.scrollbarBg = QColor(40, 40, 40);
    c.scrollbarHandle = QColor(80, 73, 69);
    c.scrollbarHandleHover = QColor(102, 92, 84);
    c.selectionBg = QColor(69, 63, 58);
    c.selectionFg = QColor(235, 219, 178);
    c.caret = QColor(235, 219, 178);
    c.lineHighlight = QColor(60, 56, 54);
    c.marginBg = QColor(50, 48, 47);
    c.marginFg = QColor(168, 153, 132);
    c.foldMarginBg = QColor(60, 56, 54);
    c.foldMarginBgAlt = QColor(50, 48, 47);
    c.matchedBraceBg = QColor(80, 73, 69);
    c.matchedBraceFg = QColor(235, 219, 178);
    c.comment = QColor(146, 131, 116);
    c.keyword = QColor(251, 73, 52);
    c.string = QColor(184, 187, 38);
    c.number = QColor(211, 134, 155);
    c.operator_ = QColor(235, 219, 178);
    c.function = QColor(184, 187, 38);
    c.preprocessor = QColor(211, 134, 155);
    c.dialogBg = QColor(50, 48, 47);
    c.dialogFg = QColor(235, 219, 178);
    c.inputBg = QColor(60, 56, 54);
    c.inputFg = QColor(235, 219, 178);
    c.buttonBg = QColor(80, 73, 69);
    c.buttonFg = QColor(235, 219, 178);
    c.groupboxBg = QColor(50, 48, 47);
    c.groupboxFg = QColor(235, 219, 178);
    c.checkboxIndicator = QColor(184, 187, 38);
    c.separator = QColor(80, 73, 69);
    return c;
}

ThemeColors getThemeColors(ThemeId themeId) {
    static std::array<ThemeColors, 7> s_cache;
    static bool s_initialized = false;
    if (!s_initialized) {
        s_cache[static_cast<int>(ThemeId::Light)] = createLightTheme();
        s_cache[static_cast<int>(ThemeId::Dark)] = createDarkTheme();
        s_cache[static_cast<int>(ThemeId::Nord)] = createNordTheme();
        s_cache[static_cast<int>(ThemeId::SolarizedLight)] = createSolarizedLightTheme();
        s_cache[static_cast<int>(ThemeId::Monokai)] = createMonokaiTheme();
        s_cache[static_cast<int>(ThemeId::GruvboxDark)] = createGruvboxDarkTheme();
        s_initialized = true;
    }

    if (themeId == ThemeId::System) {
        QStyleHints* hints = QApplication::styleHints();
        return (hints && hints->colorScheme() == Qt::ColorScheme::Dark)
            ? s_cache[static_cast<int>(ThemeId::Dark)]
            : s_cache[static_cast<int>(ThemeId::Light)];
    }

    int idx = static_cast<int>(themeId);
    if (idx < 0 || idx >= static_cast<int>(s_cache.size()))
        return s_cache[static_cast<int>(ThemeId::Light)];
    return s_cache[idx];
}

QPalette getThemePalette(const ThemeColors& colors) {
    QPalette palette;
    palette.setColor(QPalette::Window, colors.dialogBg);
    palette.setColor(QPalette::WindowText, colors.dialogFg);
    palette.setColor(QPalette::Base, colors.inputBg);
    palette.setColor(QPalette::Text, colors.inputFg);
    palette.setColor(QPalette::Button, colors.buttonBg);
    palette.setColor(QPalette::ButtonText, colors.buttonFg);
    palette.setColor(QPalette::Highlight, colors.selectionBg);
    palette.setColor(QPalette::HighlightedText, colors.selectionFg);
    palette.setColor(QPalette::Link, QColor(51, 153, 255));
    palette.setColor(QPalette::LinkVisited, QColor(153, 51, 255));
    palette.setColor(QPalette::AlternateBase, colors.background);
    palette.setColor(QPalette::ToolTipBase, colors.background);
    palette.setColor(QPalette::ToolTipText, colors.foreground);
    if (colors.isDark) {
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 120));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));
    } else {
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(160, 160, 160));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(160, 160, 160));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(160, 160, 160));
    }
    return palette;
}

QString themeDisplayName(ThemeId themeId) {
    switch (themeId) {
        case ThemeId::Light: return QObject::tr("Light");
        case ThemeId::Dark: return QObject::tr("Dark");
        case ThemeId::System: return QObject::tr("System");
        case ThemeId::Nord: return QObject::tr("Nord");
        case ThemeId::SolarizedLight: return QObject::tr("Solarized Light");
        case ThemeId::Monokai: return QObject::tr("Monokai");
        case ThemeId::GruvboxDark: return QObject::tr("Gruvbox Dark");
    }
    return QObject::tr("Light");
}

bool isThemeDark(ThemeId themeId) {
    if (themeId == ThemeId::System) {
        QStyleHints* hints = QApplication::styleHints();
        return hints && hints->colorScheme() == Qt::ColorScheme::Dark;
    }
    auto colors = getThemeColors(themeId);
    return colors.isDark;
}
