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

#include <QColor>
#include <QJsonObject>
#include <QPalette>
#include <QString>

enum class ThemeId
{
    Light = 0,
    Dark = 1,
    System = 2,
    Nord = 3,
    SolarizedLight = 4,
    Monokai = 5,
    GruvboxDark = 6,
    CatppuccinMocha = 7,
    CatppuccinMacchiato = 8,
    CatppuccinFrappe = 9,
    CatppuccinLatte = 10,
    TokyoNight = 11,
    TokyoNightStorm = 12,
    Dracula = 13,
    OneDark = 14,
    AyuLight = 15,
    AyuDark = 16,
    Count
};

struct ThemeColors
{
    QString name;
    bool isDark;

    // Core fields (19)
    QColor background;
    QColor foreground;
    QColor surfaceBg;
    QColor surfaceFg;
    QColor accent;
    QColor border;
    QColor caret;
    QColor lineHighlight;
    QColor marginBg;
    QColor marginFg;
    QColor foldMarginBg;
    QColor foldMarginBgAlt;
    QColor comment;
    QColor keyword;
    QColor string;
    QColor number;
    QColor operator_;
    QColor function;
    QColor preprocessor;

    // Derived fields (23) — computed by resolve()
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
    QColor matchedBraceBg;
    QColor matchedBraceFg;
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

    void resolve();
};

void initThemeSystem();
ThemeColors getThemeColors(ThemeId themeId);
QPalette getThemePalette(const ThemeColors& colors);
QString themeDisplayName(ThemeId themeId);
bool isThemeDark(ThemeId themeId);
bool prefersNativeStyling(ThemeId themeId);
int builtInThemeCount();
QList<ThemeId> allBuiltInThemes();

// Custom user themes from ~/.config/devpad/themes/*.json
QList<ThemeColors> loadCustomThemes();
ThemeColors parseThemeJson(const QJsonObject& json);
QStringList customThemeNames();
ThemeColors getCustomThemeColors(const QString& name);

#endif
