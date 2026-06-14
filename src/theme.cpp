#include "theme.h"
#include "settingsmanager.h"
#include <QApplication>
#include <QStyleHints>
#include <array>

static int clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }

static QColor shiftRgb(const QColor &c, int delta) {
    return QColor(clamp(c.red() + delta, 0, 255),
                  clamp(c.green() + delta, 0, 255),
                  clamp(c.blue() + delta, 0, 255));
}

static QColor dimToGray(const QColor &c, double amount) {
    int gray = qRound(c.red() * 0.299 + c.green() * 0.587 + c.blue() * 0.114);
    int r = qRound(c.red() * (1.0 - amount) + gray * amount);
    int g = qRound(c.green() * (1.0 - amount) + gray * amount);
    int b = qRound(c.blue() * (1.0 - amount) + gray * amount);
    return QColor(clamp(r, 0, 255), clamp(g, 0, 255), clamp(b, 0, 255));
}

static QColor blend(const QColor &a, const QColor &b, double t) {
    int r = qRound(a.red() * (1.0 - t) + b.red() * t);
    int g = qRound(a.green() * (1.0 - t) + b.green() * t);
    int bl = qRound(a.blue() * (1.0 - t) + b.blue() * t);
    return QColor(clamp(r, 0, 255), clamp(g, 0, 255), clamp(bl, 0, 255));
}

static QColor accentFgFor(const QColor &accent) {
    return accent.lightness() < 128 ? QColor(255, 255, 255) : QColor(0, 0, 0);
}

void ThemeColors::resolve() {
    bool dark = isDark;
    int surfDelta = dark ? 8 : -5;
    int statusDelta = dark ? 4 : -12;
    int tabDelta = dark ? -4 : -17;
    int tabActiveDelta = dark ? 12 : 0;

    toolbarBg = shiftRgb(surfaceBg, surfDelta);
    toolbarFg = surfaceFg;
    menuBg = surfaceBg;
    menuFg = surfaceFg;
    statusbarBg = shiftRgb(surfaceBg, statusDelta);
    statusbarFg = dimToGray(surfaceFg, 0.35);
    tabBg = shiftRgb(surfaceBg, tabDelta);
    tabBgActive = shiftRgb(surfaceBg, tabActiveDelta);
    tabFg = dimToGray(surfaceFg, 0.30);
    tabFgActive = surfaceFg;
    tabBorder = border;

    scrollbarBg = shiftRgb(surfaceBg, dark ? 6 : -3);
    scrollbarHandle = border;
    scrollbarHandleHover = shiftRgb(border, dark ? 20 : 25);

    selectionBg = accent;
    selectionFg = accentFgFor(accent);
    matchedBraceBg = blend(accent, background, 0.25);
    matchedBraceFg = foreground;
    checkboxIndicator = accent;

    dialogBg = surfaceBg;
    dialogFg = surfaceFg;
    groupboxBg = surfaceBg;
    groupboxFg = surfaceFg;

    inputBg = background;
    inputFg = foreground;
    buttonBg = shiftRgb(surfaceBg, surfDelta);
    buttonFg = surfaceFg;

    separator = border;
}

// ---------- Theme factories ----------

static ThemeColors createLightTheme() {
    ThemeColors c;
    c.name = "Light";
    c.isDark = false;
    c.background = QColor(255, 255, 255);
    c.foreground = QColor(0, 0, 0);
    c.surfaceBg = QColor(252, 252, 252);
    c.surfaceFg = QColor(0, 0, 0);
    c.accent = QColor(51, 153, 255);
    c.border = QColor(210, 210, 210);
    c.caret = QColor(0, 0, 0);
    c.lineHighlight = QColor(232, 242, 254);
    c.marginBg = QColor(240, 240, 240);
    c.marginFg = QColor(100, 100, 100);
    c.foldMarginBg = QColor(220, 220, 220);
    c.foldMarginBgAlt = QColor(200, 200, 200);
    c.comment = QColor(0, 128, 0);
    c.keyword = QColor(0, 0, 255);
    c.string = QColor(163, 21, 21);
    c.number = QColor(0, 100, 0);
    c.operator_ = QColor(0, 0, 0);
    c.function = QColor(0, 0, 128);
    c.preprocessor = QColor(128, 0, 128);
    c.resolve();
    return c;
}

static ThemeColors createDarkTheme() {
    ThemeColors c;
    c.name = "Dark";
    c.isDark = true;
    c.background = QColor(30, 34, 42);
    c.foreground = QColor(220, 220, 220);
    c.surfaceBg = QColor(40, 44, 52);
    c.surfaceFg = QColor(220, 220, 220);
    c.accent = QColor(60, 100, 160);
    c.border = QColor(60, 64, 72);
    c.caret = QColor(240, 240, 240);
    c.lineHighlight = QColor(45, 50, 60);
    c.marginBg = QColor(40, 44, 52);
    c.marginFg = QColor(180, 180, 180);
    c.foldMarginBg = QColor(50, 54, 62);
    c.foldMarginBgAlt = QColor(40, 44, 52);
    c.comment = QColor(80, 200, 80);
    c.keyword = QColor(86, 156, 255);
    c.string = QColor(214, 157, 90);
    c.number = QColor(181, 206, 168);
    c.operator_ = QColor(180, 180, 180);
    c.function = QColor(220, 220, 120);
    c.preprocessor = QColor(220, 130, 170);
    c.resolve();
    c.toolbarBg = QColor(45, 49, 58);
    c.statusbarBg = QColor(40, 44, 52);
    return c;
}

static ThemeColors createNordTheme() {
    ThemeColors c;
    c.name = "Nord";
    c.isDark = true;
    c.background = QColor(46, 52, 64);
    c.foreground = QColor(216, 222, 233);
    c.surfaceBg = QColor(59, 66, 82);
    c.surfaceFg = QColor(216, 222, 233);
    c.accent = QColor(136, 192, 208);
    c.border = QColor(76, 86, 106);
    c.caret = QColor(229, 233, 240);
    c.lineHighlight = QColor(59, 66, 82);
    c.marginBg = QColor(40, 44, 52);
    c.marginFg = QColor(180, 190, 200);
    c.foldMarginBg = QColor(50, 54, 62);
    c.foldMarginBgAlt = QColor(46, 52, 64);
    c.comment = QColor(76, 86, 106);
    c.keyword = QColor(129, 161, 193);
    c.string = QColor(163, 190, 140);
    c.number = QColor(181, 189, 104);
    c.operator_ = QColor(229, 233, 240);
    c.function = QColor(136, 192, 208);
    c.preprocessor = QColor(191, 97, 106);
    c.resolve();
    c.toolbarBg = QColor(59, 66, 82);
    c.statusbarBg = QColor(59, 66, 82);
    c.scrollbarBg = QColor(46, 52, 64);
    return c;
}

static ThemeColors createSolarizedLightTheme() {
    ThemeColors c;
    c.name = "Solarized Light";
    c.isDark = false;
    c.background = QColor(253, 246, 227);
    c.foreground = QColor(101, 123, 131);
    c.surfaceBg = QColor(253, 246, 227);
    c.surfaceFg = QColor(101, 123, 131);
    c.accent = QColor(38, 139, 210);
    c.border = QColor(211, 204, 186);
    c.caret = QColor(7, 54, 66);
    c.lineHighlight = QColor(238, 232, 213);
    c.marginBg = QColor(238, 232, 213);
    c.marginFg = QColor(101, 123, 131);
    c.foldMarginBg = QColor(228, 222, 203);
    c.foldMarginBgAlt = QColor(218, 212, 193);
    c.comment = QColor(147, 161, 161);
    c.keyword = QColor(38, 139, 210);
    c.string = QColor(42, 161, 152);
    c.number = QColor(42, 161, 152);
    c.operator_ = QColor(101, 123, 131);
    c.function = QColor(38, 139, 210);
    c.preprocessor = QColor(211, 54, 130);
    c.resolve();
    c.toolbarBg = QColor(238, 232, 213);
    c.statusbarBg = QColor(238, 232, 213);
    c.scrollbarBg = QColor(238, 232, 213);
    c.tabFgActive = QColor(0, 43, 54);
    return c;
}

static ThemeColors createMonokaiTheme() {
    ThemeColors c;
    c.name = "Monokai";
    c.isDark = true;
    c.background = QColor(39, 40, 34);
    c.foreground = QColor(248, 248, 242);
    c.surfaceBg = QColor(45, 46, 40);
    c.surfaceFg = QColor(248, 248, 242);
    c.accent = QColor(72, 73, 66);
    c.border = QColor(62, 61, 52);
    c.caret = QColor(248, 248, 240);
    c.lineHighlight = QColor(62, 61, 52);
    c.marginBg = QColor(45, 46, 40);
    c.marginFg = QColor(180, 180, 175);
    c.foldMarginBg = QColor(52, 53, 46);
    c.foldMarginBgAlt = QColor(45, 46, 40);
    c.comment = QColor(117, 113, 94);
    c.keyword = QColor(249, 38, 114);
    c.string = QColor(230, 219, 116);
    c.number = QColor(174, 129, 255);
    c.operator_ = QColor(248, 248, 242);
    c.function = QColor(166, 226, 46);
    c.preprocessor = QColor(249, 38, 114);
    c.resolve();
    c.toolbarBg = QColor(52, 53, 46);
    c.statusbarBg = QColor(45, 46, 40);
    c.tabBg = QColor(45, 46, 40);
    return c;
}

static ThemeColors createGruvboxDarkTheme() {
    ThemeColors c;
    c.name = "Gruvbox Dark";
    c.isDark = true;
    c.background = QColor(40, 40, 40);
    c.foreground = QColor(235, 219, 178);
    c.surfaceBg = QColor(50, 48, 47);
    c.surfaceFg = QColor(235, 219, 178);
    c.accent = QColor(69, 63, 58);
    c.border = QColor(80, 73, 69);
    c.caret = QColor(235, 219, 178);
    c.lineHighlight = QColor(60, 56, 54);
    c.marginBg = QColor(50, 48, 47);
    c.marginFg = QColor(168, 153, 132);
    c.foldMarginBg = QColor(60, 56, 54);
    c.foldMarginBgAlt = QColor(50, 48, 47);
    c.comment = QColor(146, 131, 116);
    c.keyword = QColor(251, 73, 52);
    c.string = QColor(184, 187, 38);
    c.number = QColor(211, 134, 155);
    c.operator_ = QColor(235, 219, 178);
    c.function = QColor(184, 187, 38);
    c.preprocessor = QColor(211, 134, 155);
    c.resolve();
    c.toolbarBg = QColor(60, 56, 54);
    c.statusbarBg = QColor(50, 48, 47);
    c.tabBg = QColor(50, 48, 47);
    c.tabBgActive = QColor(80, 73, 69);
    return c;
}

// ---------- Accent overlay ----------

static void applyAccent(ThemeColors &colors) {
    const auto &s = SettingsManager::instance();
    if (!s.hasAccentColor())
        return;
    QColor accent = s.accentColor();
    colors.accent = accent;
    colors.selectionBg = accent;
    colors.selectionFg = accentFgFor(accent);
    colors.caret = accent;
    int l = accent.lightness();
    int lineR = qRound(colors.background.red() * 0.85 + accent.red() * 0.15);
    int lineG = qRound(colors.background.green() * 0.85 + accent.green() * 0.15);
    int lineB = qRound(colors.background.blue() * 0.85 + accent.blue() * 0.15);
    colors.lineHighlight = QColor(clamp(lineR, 0, 255), clamp(lineG, 0, 255), clamp(lineB, 0, 255));
    colors.matchedBraceBg = blend(accent, colors.background, 0.25);
    colors.matchedBraceFg = l < 128 ? QColor(255, 255, 255) : QColor(0, 0, 0);
    colors.checkboxIndicator = accent;
    colors.separator = l < 128 ? shiftRgb(accent, 40) : shiftRgb(accent, -20);
}

// ---------- Public API ----------

static bool systemIsDark() {
    auto *hints = QApplication::styleHints();
    if (hints) {
        if (hints->colorScheme() == Qt::ColorScheme::Dark)
            return true;
        if (hints->colorScheme() == Qt::ColorScheme::Light)
            return false;
    }
    QColor windowColor = QApplication::palette().color(QPalette::Window);
    return windowColor.lightness() < 128;
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

    ThemeId resolved = themeId;
    if (themeId == ThemeId::System) {
        resolved = systemIsDark() ? ThemeId::Dark : ThemeId::Light;
    }

    int idx = static_cast<int>(resolved);
    if (idx < 0 || idx >= static_cast<int>(s_cache.size()))
        return s_cache[static_cast<int>(ThemeId::Light)];

    ThemeColors colors = s_cache[idx];
    applyAccent(colors);
    return colors;
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
    palette.setColor(QPalette::Link, colors.accent);
    palette.setColor(QPalette::LinkVisited, colors.accent);
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
    if (themeId == ThemeId::System)
        return systemIsDark();
    auto colors = getThemeColors(themeId);
    return colors.isDark;
}

bool prefersNativeStyling(ThemeId themeId) {
    return themeId == ThemeId::System;
}
