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
#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QColor>
#include <QPalette>

enum class ThemeId {
    Light = 0,
    Dark = 1,
    System = 2,
    Nord = 3,
    SolarizedLight = 4,
    Monokai = 5,
    GruvboxDark = 6
};

struct ThemeColors {
    QString name;
    bool isDark;

    QColor background;
    QColor foreground;
    QColor toolbarBg;
    QColor toolbarFg;
    QColor menuBg;
    QColor menuFg;
    QColor statusbarBg;
    QColor statusbarFg;
    QColor tabBg;
    QColor tabBgActive;
    QColor tabFg;
    QColor tabFgActive;
    QColor tabBorder;
    QColor scrollbarBg;
    QColor scrollbarHandle;
    QColor scrollbarHandleHover;
    QColor selectionBg;
    QColor selectionFg;
    QColor caret;
    QColor lineHighlight;
    QColor marginBg;
    QColor marginFg;
    QColor foldMarginBg;
    QColor foldMarginBgAlt;
    QColor matchedBraceBg;
    QColor matchedBraceFg;
    QColor comment;
    QColor keyword;
    QColor string;
    QColor number;
    QColor operator_;
    QColor function;
    QColor preprocessor;
    QColor dialogBg;
    QColor dialogFg;
    QColor inputBg;
    QColor inputFg;
    QColor buttonBg;
    QColor buttonFg;
    QColor groupboxBg;
    QColor groupboxFg;
    QColor checkboxIndicator;
    QColor separator;
};

ThemeColors getThemeColors(ThemeId themeId);
QPalette getThemePalette(const ThemeColors& colors);
QString themeDisplayName(ThemeId themeId);
bool isThemeDark(ThemeId themeId);

#endif
