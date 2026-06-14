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

ThemeColors getThemeColors(ThemeId themeId);
QPalette getThemePalette(const ThemeColors& colors);
QString themeDisplayName(ThemeId themeId);
bool isThemeDark(ThemeId themeId);
bool prefersNativeStyling(ThemeId themeId);

#endif
