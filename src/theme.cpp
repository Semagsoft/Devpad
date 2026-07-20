#include "theme.h"

#include "settingsmanager.h"

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QStyleHints>
#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>

static int clamp(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

static QColor shiftRgb(const QColor& c, int delta)
{
    return QColor{clamp(c.red() + delta, 0, 255), clamp(c.green() + delta, 0, 255), clamp(c.blue() + delta, 0, 255)};
}

static QColor dimToGray(const QColor& c, double amount)
{
    int gray = qRound(c.red() * 0.299 + c.green() * 0.587 + c.blue() * 0.114);
    int r = qRound(c.red() * (1.0 - amount) + gray * amount);
    int g = qRound(c.green() * (1.0 - amount) + gray * amount);
    int b = qRound(c.blue() * (1.0 - amount) + gray * amount);
    return QColor{clamp(r, 0, 255), clamp(g, 0, 255), clamp(b, 0, 255)};
}

static QColor blend(const QColor& a, const QColor& b, double t)
{
    int r = qRound(a.red() * (1.0 - t) + b.red() * t);
    int g = qRound(a.green() * (1.0 - t) + b.green() * t);
    int bl = qRound(a.blue() * (1.0 - t) + b.blue() * t);
    return QColor{clamp(r, 0, 255), clamp(g, 0, 255), clamp(bl, 0, 255)};
}

static QColor accentFgFor(const QColor& accent)
{
    return accent.lightness() < 128 ? QColor(255, 255, 255) : QColor(0, 0, 0);
}

// ── WCAG contrast utilities ──────────────────────────────────────

static double relativeLuminance(const QColor& c)
{
    auto linearize = [](double ch) -> double { return ch <= 0.04045 ? ch / 12.92 : std::pow((ch + 0.055) / 1.055, 2.4); };
    double r = linearize(c.redF());
    double g = linearize(c.greenF());
    double b = linearize(c.blueF());
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

static double contrastRatio(const QColor& a, const QColor& b)
{
    double l1 = relativeLuminance(a);
    double l2 = relativeLuminance(b);
    if (l1 < l2)
        std::swap(l1, l2);
    return (l1 + 0.05) / (l2 + 0.05);
}

static QColor ensureContrast(const QColor& text, const QColor& bg, double minRatio)
{
    if (contrastRatio(text, bg) >= minRatio)
        return text;
    QColor result = text;
    bool dark = bg.lightness() < 128;
    for (int i = 0; i < 30; ++i)
    {
        result = dark ? result.lighter(108) : result.darker(108);
        if (contrastRatio(result, bg) >= minRatio)
            return result;
    }
    return dark ? QColor(255, 255, 255) : QColor(0, 0, 0);
}

// ── Saturation-aware dim (preserve hue better) ───────────────────

static QColor dimSaturated(const QColor& c, double amount, bool preserveHue = true)
{
    if (!preserveHue)
        return dimToGray(c, amount);
    int gray = qRound(c.red() * 0.299 + c.green() * 0.587 + c.blue() * 0.114);
    double s = 1.0 - amount;
    return QColor{clamp(qRound(c.red() * s + gray * amount), 0, 255), clamp(qRound(c.green() * s + gray * amount), 0, 255),
                  clamp(qRound(c.blue() * s + gray * amount), 0, 255)};
}

// ── Theme polish: per-theme overrides after resolve() ────────────

static void applyThemePolish(ThemeColors& c, ThemeId id)
{
    switch (id)
    {
    case ThemeId::Dark:
        c.toolbarBg = QColor(18, 18, 22);
        c.statusbarBg = QColor(10, 10, 12);
        break;
    case ThemeId::Nord:
        c.toolbarBg = QColor(59, 66, 82);
        c.statusbarBg = QColor(59, 66, 82);
        c.scrollbarBg = QColor(46, 52, 64);
        break;
    case ThemeId::SolarizedLight:
        c.toolbarBg = QColor(238, 232, 213);
        c.statusbarBg = QColor(238, 232, 213);
        c.scrollbarBg = QColor(238, 232, 213);
        c.tabFgActive = QColor(0, 43, 54);
        break;
    case ThemeId::Monokai:
        c.toolbarBg = QColor(52, 53, 46);
        c.statusbarBg = QColor(45, 46, 40);
        c.tabBg = QColor(45, 46, 40);
        break;
    case ThemeId::GruvboxDark:
        c.toolbarBg = QColor(60, 56, 54);
        c.statusbarBg = QColor(50, 48, 47);
        c.tabBg = QColor(50, 48, 47);
        c.tabBgActive = QColor(80, 73, 69);
        break;
    case ThemeId::CatppuccinMocha:
        c.toolbarBg = QColor(24, 24, 37);
        c.statusbarBg = QColor(17, 17, 27);
        c.tabBg = QColor(30, 30, 46);
        break;
    case ThemeId::CatppuccinMacchiato:
        c.toolbarBg = QColor(30, 32, 48);
        c.statusbarBg = QColor(24, 26, 42);
        c.tabBg = QColor(36, 39, 58);
        break;
    case ThemeId::CatppuccinFrappe:
        c.toolbarBg = QColor(41, 44, 60);
        c.statusbarBg = QColor(35, 38, 52);
        c.tabBg = QColor(48, 52, 70);
        break;
    case ThemeId::CatppuccinLatte:
        c.toolbarBg = QColor(230, 233, 239);
        c.statusbarBg = QColor(220, 224, 232);
        c.tabBg = QColor(239, 241, 245);
        break;
    case ThemeId::TokyoNight:
        c.toolbarBg = QColor(22, 22, 30);
        c.statusbarBg = QColor(18, 18, 28);
        c.tabBg = QColor(26, 27, 38);
        break;
    case ThemeId::TokyoNightStorm:
        c.toolbarBg = QColor(29, 33, 49);
        c.statusbarBg = QColor(26, 30, 46);
        c.tabBg = QColor(36, 40, 59);
        break;
    case ThemeId::Dracula:
        c.toolbarBg = QColor(33, 34, 44);
        c.statusbarBg = QColor(28, 29, 38);
        c.tabBg = QColor(40, 42, 54);
        break;
    case ThemeId::OneDark:
        c.toolbarBg = QColor(33, 37, 43);
        c.statusbarBg = QColor(30, 33, 39);
        c.tabBg = QColor(40, 44, 52);
        break;
    case ThemeId::AyuLight:
        c.toolbarBg = QColor(240, 240, 240);
        c.statusbarBg = QColor(235, 235, 235);
        c.tabBg = QColor(250, 250, 250);
        c.tabBgActive = QColor(255, 255, 255);
        break;
    case ThemeId::AyuDark:
        c.toolbarBg = QColor(25, 30, 42);
        c.statusbarBg = QColor(22, 26, 38);
        c.tabBg = QColor(31, 36, 48);
        break;
    default:
        break;
    }
}

// ── Improved resolve() ───────────────────────────────────────────

void ThemeColors::resolve()
{
    bool dark = isDark;

    toolbarBg = shiftRgb(surfaceBg, dark ? 6 : -4);
    toolbarFg = ensureContrast(surfaceFg, toolbarBg, 4.5);

    menuBg = surfaceBg;
    menuFg = surfaceFg;

    statusbarBg = shiftRgb(surfaceBg, dark ? 4 : -10);
    statusbarFg = ensureContrast(dimSaturated(surfaceFg, 0.25), statusbarBg, 4.5);

    tabBg = shiftRgb(surfaceBg, dark ? -3 : -14);
    tabFg = ensureContrast(dimSaturated(surfaceFg, 0.30), tabBg, 3.5);
    tabBgActive = shiftRgb(surfaceBg, dark ? 10 : -2);
    tabFgActive = ensureContrast(surfaceFg, tabBgActive, 4.5);
    tabBorder = border;

    scrollbarBg = shiftRgb(surfaceBg, dark ? 5 : -2);
    scrollbarHandle = ensureContrast(border, scrollbarBg, 2.0);
    scrollbarHandleHover = shiftRgb(scrollbarHandle, dark ? 20 : 25);

    selectionBg = accent;
    selectionFg = accentFgFor(accent);
    matchedBraceBg = blend(accent, background, 0.25);
    matchedBraceFg = ensureContrast(foreground, matchedBraceBg, 3.0);
    checkboxIndicator = accent;

    dialogBg = surfaceBg;
    dialogFg = surfaceFg;
    groupboxBg = surfaceBg;
    groupboxFg = surfaceFg;

    inputBg = background;
    inputFg = foreground;

    buttonBg = shiftRgb(surfaceBg, dark ? 6 : -4);
    buttonFg = ensureContrast(surfaceFg, buttonBg, 4.5);

    separator = border;
}

// ── Theme factory functions ──────────────────────────────────────

static ThemeColors createLightTheme()
{
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

static ThemeColors createDarkTheme()
{
    ThemeColors c;
    c.name = "Dark";
    c.isDark = true;
    c.background = QColor(0, 0, 0);
    c.foreground = QColor(220, 220, 220);
    c.surfaceBg = QColor(13, 13, 15);
    c.surfaceFg = QColor(220, 220, 220);
    c.accent = QColor(60, 100, 160);
    c.border = QColor(35, 35, 40);
    c.caret = QColor(240, 240, 240);
    c.lineHighlight = QColor(18, 20, 25);
    c.marginBg = QColor(13, 13, 15);
    c.marginFg = QColor(160, 160, 160);
    c.foldMarginBg = QColor(20, 20, 22);
    c.foldMarginBgAlt = QColor(13, 13, 15);
    c.comment = QColor(80, 200, 80);
    c.keyword = QColor(86, 156, 255);
    c.string = QColor(214, 157, 90);
    c.number = QColor(181, 206, 168);
    c.operator_ = QColor(180, 180, 180);
    c.function = QColor(220, 220, 120);
    c.preprocessor = QColor(220, 130, 170);
    c.resolve();
    return c;
}

static ThemeColors createNordTheme()
{
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
    return c;
}

static ThemeColors createSolarizedLightTheme()
{
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
    return c;
}

static ThemeColors createMonokaiTheme()
{
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
    return c;
}

static ThemeColors createGruvboxDarkTheme()
{
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
    return c;
}

// ── New Theme: Catppuccin Mocha ──────────────────────────────────

static ThemeColors createCatppuccinMochaTheme()
{
    ThemeColors c;
    c.name = "Catppuccin Mocha";
    c.isDark = true;
    c.background = QColor(30, 30, 46);
    c.foreground = QColor(205, 214, 244);
    c.surfaceBg = QColor(24, 24, 37);
    c.surfaceFg = QColor(205, 214, 244);
    c.accent = QColor(203, 166, 247);
    c.border = QColor(69, 71, 90);
    c.caret = QColor(245, 224, 220);
    c.lineHighlight = QColor(49, 50, 68);
    c.marginBg = QColor(24, 24, 37);
    c.marginFg = QColor(108, 112, 134);
    c.foldMarginBg = QColor(30, 30, 46);
    c.foldMarginBgAlt = QColor(24, 24, 37);
    c.comment = QColor(108, 112, 134);
    c.keyword = QColor(203, 166, 247);
    c.string = QColor(166, 227, 161);
    c.number = QColor(250, 179, 135);
    c.operator_ = QColor(137, 220, 235);
    c.function = QColor(137, 180, 250);
    c.preprocessor = QColor(243, 139, 168);
    c.resolve();
    return c;
}

// ── New Theme: Catppuccin Macchiato ──────────────────────────────

static ThemeColors createCatppuccinMacchiatoTheme()
{
    ThemeColors c;
    c.name = "Catppuccin Macchiato";
    c.isDark = true;
    c.background = QColor(36, 39, 58);
    c.foreground = QColor(202, 211, 245);
    c.surfaceBg = QColor(30, 32, 48);
    c.surfaceFg = QColor(202, 211, 245);
    c.accent = QColor(203, 166, 247);
    c.border = QColor(73, 77, 100);
    c.caret = QColor(244, 219, 214);
    c.lineHighlight = QColor(54, 58, 79);
    c.marginBg = QColor(30, 32, 48);
    c.marginFg = QColor(110, 115, 141);
    c.foldMarginBg = QColor(36, 39, 58);
    c.foldMarginBgAlt = QColor(30, 32, 48);
    c.comment = QColor(110, 115, 141);
    c.keyword = QColor(203, 166, 247);
    c.string = QColor(166, 218, 149);
    c.number = QColor(245, 169, 127);
    c.operator_ = QColor(139, 213, 202);
    c.function = QColor(138, 173, 244);
    c.preprocessor = QColor(237, 135, 150);
    c.resolve();
    return c;
}

// ── New Theme: Catppuccin Frappé ─────────────────────────────────

static ThemeColors createCatppuccinFrappeTheme()
{
    ThemeColors c;
    c.name = "Catppuccin Frappé";
    c.isDark = true;
    c.background = QColor(48, 52, 70);
    c.foreground = QColor(198, 208, 245);
    c.surfaceBg = QColor(41, 44, 60);
    c.surfaceFg = QColor(198, 208, 245);
    c.accent = QColor(202, 158, 230);
    c.border = QColor(81, 87, 109);
    c.caret = QColor(242, 213, 207);
    c.lineHighlight = QColor(65, 69, 89);
    c.marginBg = QColor(41, 44, 60);
    c.marginFg = QColor(115, 121, 148);
    c.foldMarginBg = QColor(48, 52, 70);
    c.foldMarginBgAlt = QColor(41, 44, 60);
    c.comment = QColor(115, 121, 148);
    c.keyword = QColor(202, 158, 230);
    c.string = QColor(166, 209, 137);
    c.number = QColor(239, 159, 118);
    c.operator_ = QColor(129, 200, 190);
    c.function = QColor(140, 170, 238);
    c.preprocessor = QColor(231, 130, 132);
    c.resolve();
    return c;
}

// ── New Theme: Catppuccin Latte ──────────────────────────────────

static ThemeColors createCatppuccinLatteTheme()
{
    ThemeColors c;
    c.name = "Catppuccin Latte";
    c.isDark = false;
    c.background = QColor(239, 241, 245);
    c.foreground = QColor(76, 79, 105);
    c.surfaceBg = QColor(230, 233, 239);
    c.surfaceFg = QColor(76, 79, 105);
    c.accent = QColor(136, 57, 239);
    c.border = QColor(204, 208, 218);
    c.caret = QColor(220, 138, 120);
    c.lineHighlight = QColor(220, 224, 232);
    c.marginBg = QColor(230, 233, 239);
    c.marginFg = QColor(156, 160, 176);
    c.foldMarginBg = QColor(220, 224, 232);
    c.foldMarginBgAlt = QColor(230, 233, 239);
    c.comment = QColor(156, 160, 176);
    c.keyword = QColor(136, 57, 239);
    c.string = QColor(64, 160, 43);
    c.number = QColor(254, 100, 11);
    c.operator_ = QColor(4, 165, 229);
    c.function = QColor(30, 102, 245);
    c.preprocessor = QColor(210, 15, 57);
    c.resolve();
    return c;
}

// ── New Theme: Tokyo Night ───────────────────────────────────────

static ThemeColors createTokyoNightTheme()
{
    ThemeColors c;
    c.name = "Tokyo Night";
    c.isDark = true;
    c.background = QColor(26, 27, 38);
    c.foreground = QColor(169, 177, 214);
    c.surfaceBg = QColor(22, 22, 30);
    c.surfaceFg = QColor(169, 177, 214);
    c.accent = QColor(122, 162, 247);
    c.border = QColor(47, 51, 70);
    c.caret = QColor(192, 202, 245);
    c.lineHighlight = QColor(31, 34, 51);
    c.marginBg = QColor(22, 22, 30);
    c.marginFg = QColor(86, 95, 137);
    c.foldMarginBg = QColor(26, 27, 38);
    c.foldMarginBgAlt = QColor(22, 22, 30);
    c.comment = QColor(86, 95, 137);
    c.keyword = QColor(157, 124, 216);
    c.string = QColor(158, 206, 106);
    c.number = QColor(255, 158, 100);
    c.operator_ = QColor(137, 221, 255);
    c.function = QColor(122, 162, 247);
    c.preprocessor = QColor(247, 118, 142);
    c.resolve();
    return c;
}

// ── New Theme: Tokyo Night Storm ─────────────────────────────────

static ThemeColors createTokyoNightStormTheme()
{
    ThemeColors c;
    c.name = "Tokyo Night Storm";
    c.isDark = true;
    c.background = QColor(36, 40, 59);
    c.foreground = QColor(169, 177, 214);
    c.surfaceBg = QColor(29, 33, 50);
    c.surfaceFg = QColor(169, 177, 214);
    c.accent = QColor(122, 162, 247);
    c.border = QColor(54, 59, 84);
    c.caret = QColor(192, 202, 245);
    c.lineHighlight = QColor(42, 47, 68);
    c.marginBg = QColor(29, 33, 50);
    c.marginFg = QColor(86, 95, 137);
    c.foldMarginBg = QColor(36, 40, 59);
    c.foldMarginBgAlt = QColor(29, 33, 50);
    c.comment = QColor(86, 95, 137);
    c.keyword = QColor(157, 124, 216);
    c.string = QColor(158, 206, 106);
    c.number = QColor(255, 158, 100);
    c.operator_ = QColor(137, 221, 255);
    c.function = QColor(122, 162, 247);
    c.preprocessor = QColor(247, 118, 142);
    c.resolve();
    return c;
}

// ── New Theme: Dracula ───────────────────────────────────────────

static ThemeColors createDraculaTheme()
{
    ThemeColors c;
    c.name = "Dracula";
    c.isDark = true;
    c.background = QColor(40, 42, 54);
    c.foreground = QColor(248, 248, 242);
    c.surfaceBg = QColor(33, 34, 44);
    c.surfaceFg = QColor(248, 248, 242);
    c.accent = QColor(189, 147, 249);
    c.border = QColor(68, 71, 90);
    c.caret = QColor(248, 248, 242);
    c.lineHighlight = QColor(49, 50, 68);
    c.marginBg = QColor(33, 34, 44);
    c.marginFg = QColor(98, 114, 164);
    c.foldMarginBg = QColor(40, 42, 54);
    c.foldMarginBgAlt = QColor(33, 34, 44);
    c.comment = QColor(98, 114, 164);
    c.keyword = QColor(255, 121, 198);
    c.string = QColor(241, 250, 140);
    c.number = QColor(189, 147, 249);
    c.operator_ = QColor(248, 248, 242);
    c.function = QColor(80, 250, 123);
    c.preprocessor = QColor(255, 85, 85);
    c.resolve();
    return c;
}

// ── New Theme: One Dark ──────────────────────────────────────────

static ThemeColors createOneDarkTheme()
{
    ThemeColors c;
    c.name = "One Dark";
    c.isDark = true;
    c.background = QColor(40, 44, 52);
    c.foreground = QColor(171, 178, 191);
    c.surfaceBg = QColor(33, 37, 43);
    c.surfaceFg = QColor(171, 178, 191);
    c.accent = QColor(97, 175, 239);
    c.border = QColor(59, 64, 72);
    c.caret = QColor(82, 139, 255);
    c.lineHighlight = QColor(44, 49, 58);
    c.marginBg = QColor(33, 37, 43);
    c.marginFg = QColor(92, 99, 112);
    c.foldMarginBg = QColor(40, 44, 52);
    c.foldMarginBgAlt = QColor(33, 37, 43);
    c.comment = QColor(92, 99, 112);
    c.keyword = QColor(198, 120, 221);
    c.string = QColor(152, 195, 121);
    c.number = QColor(209, 154, 102);
    c.operator_ = QColor(171, 178, 191);
    c.function = QColor(97, 175, 239);
    c.preprocessor = QColor(224, 108, 117);
    c.resolve();
    return c;
}

// ── New Theme: Ayu Light ─────────────────────────────────────────

static ThemeColors createAyuLightTheme()
{
    ThemeColors c;
    c.name = "Ayu Light";
    c.isDark = false;
    c.background = QColor(250, 250, 250);
    c.foreground = QColor(92, 97, 102);
    c.surfaceBg = QColor(240, 240, 240);
    c.surfaceFg = QColor(92, 97, 102);
    c.accent = QColor(243, 156, 18);
    c.border = QColor(217, 217, 217);
    c.caret = QColor(255, 106, 0);
    c.lineHighlight = QColor(242, 242, 242);
    c.marginBg = QColor(240, 240, 240);
    c.marginFg = QColor(138, 145, 153);
    c.foldMarginBg = QColor(232, 232, 232);
    c.foldMarginBgAlt = QColor(240, 240, 240);
    c.comment = QColor(138, 145, 153);
    c.keyword = QColor(240, 113, 113);
    c.string = QColor(134, 179, 0);
    c.number = QColor(242, 174, 73);
    c.operator_ = QColor(92, 97, 102);
    c.function = QColor(85, 180, 212);
    c.preprocessor = QColor(163, 122, 204);
    c.resolve();
    return c;
}

// ── New Theme: Ayu Dark ──────────────────────────────────────────

static ThemeColors createAyuDarkTheme()
{
    ThemeColors c;
    c.name = "Ayu Dark";
    c.isDark = true;
    c.background = QColor(31, 36, 48);
    c.foreground = QColor(203, 204, 198);
    c.surfaceBg = QColor(25, 30, 42);
    c.surfaceFg = QColor(203, 204, 198);
    c.accent = QColor(242, 158, 116);
    c.border = QColor(46, 52, 64);
    c.caret = QColor(242, 158, 116);
    c.lineHighlight = QColor(35, 42, 56);
    c.marginBg = QColor(25, 30, 42);
    c.marginFg = QColor(86, 91, 102);
    c.foldMarginBg = QColor(31, 36, 48);
    c.foldMarginBgAlt = QColor(25, 30, 42);
    c.comment = QColor(86, 91, 102);
    c.keyword = QColor(255, 167, 89);
    c.string = QColor(186, 230, 126);
    c.number = QColor(255, 213, 128);
    c.operator_ = QColor(203, 204, 198);
    c.function = QColor(92, 207, 230);
    c.preprocessor = QColor(242, 135, 121);
    c.resolve();
    return c;
}

// ── Custom theme loading from JSON ───────────────────────────────

static QList<ThemeColors> s_customThemeCache;
static QStringList s_customThemeNames;

static QColor jsonColor(const QJsonObject& obj, const QString& key, const QColor& fallback)
{
    if (obj.contains(key))
    {
        QString s = obj.value(key).toString();
        QColor c(s);
        if (c.isValid())
            return c;
    }
    return fallback;
}

ThemeColors parseThemeJson(const QJsonObject& json)
{
    ThemeColors c;
    c.name = json.value("name").toString("Custom");
    c.isDark = json.value("isDark").toBool(false);

    QJsonObject colors = json.value("colors").toObject();
    c.background = jsonColor(colors, "background", QColor(255, 255, 255));
    c.foreground = jsonColor(colors, "foreground", QColor(0, 0, 0));
    c.surfaceBg = jsonColor(colors, "surfaceBg", c.background);
    c.surfaceFg = jsonColor(colors, "surfaceFg", c.foreground);
    c.accent = jsonColor(colors, "accent", QColor(51, 153, 255));
    c.border = jsonColor(colors, "border", QColor(200, 200, 200));
    c.caret = jsonColor(colors, "caret", c.foreground);
    c.lineHighlight = jsonColor(colors, "lineHighlight", shiftRgb(c.background, -10));
    c.marginBg = jsonColor(colors, "marginBg", shiftRgb(c.surfaceBg, -5));
    c.marginFg = jsonColor(colors, "marginFg", dimToGray(c.foreground, 0.4));
    c.foldMarginBg = jsonColor(colors, "foldMarginBg", shiftRgb(c.background, -3));
    c.foldMarginBgAlt = jsonColor(colors, "foldMarginBgAlt", shiftRgb(c.background, -8));
    c.comment = jsonColor(colors, "comment", dimToGray(c.foreground, 0.5));
    c.keyword = jsonColor(colors, "keyword", c.accent);
    c.string = jsonColor(colors, "string", c.foreground);
    c.number = jsonColor(colors, "number", c.foreground);
    c.operator_ = jsonColor(colors, "operator", c.foreground);
    c.function = jsonColor(colors, "function", c.foreground);
    c.preprocessor = jsonColor(colors, "preprocessor", c.keyword);

    c.resolve();
    return c;
}

QList<ThemeColors> loadCustomThemes()
{
    s_customThemeCache.clear();
    s_customThemeNames.clear();

    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir themeDir(configDir + "/themes");
    if (!themeDir.exists())
        return s_customThemeCache;

    QStringList jsonFiles = themeDir.entryList({"*.json"}, QDir::Files, QDir::Name);
    for (const QString& file : jsonFiles)
    {
        QFile f(themeDir.absoluteFilePath(file));
        if (!f.open(QFile::ReadOnly))
            continue;
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject())
            continue;

        ThemeColors colors = parseThemeJson(doc.object());
        s_customThemeCache.append(colors);
        s_customThemeNames.append(colors.name);
    }

    return s_customThemeCache;
}

QStringList customThemeNames()
{
    if (s_customThemeNames.isEmpty())
        loadCustomThemes();
    return s_customThemeNames;
}

ThemeColors getCustomThemeColors(const QString& name)
{
    if (s_customThemeCache.isEmpty())
        loadCustomThemes();

    for (const auto& c : s_customThemeCache)
    {
        if (c.name == name)
            return c;
    }
    return getThemeColors(ThemeId::Light);
}

// ── Accent overlay ───────────────────────────────────────────────

static void applyAccent(ThemeColors& colors)
{
    const auto& s = SettingsManager::instance();
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

// ── System theme detection ───────────────────────────────────────

static bool systemIsDarkImpl()
{
    auto* hints = QApplication::styleHints();
    if (hints)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        if (hints->colorScheme() == Qt::ColorScheme::Dark)
            return true;
        if (hints->colorScheme() == Qt::ColorScheme::Light)
            return false;
#endif
    }
    QColor windowColor = QApplication::palette().color(QPalette::Window);
    return windowColor.lightness() < 128;
}

static bool s_systemDarkCached = false;
static bool s_systemDarkValue = false;

class PaletteChangeFilter : public QObject
{
public:
    explicit PaletteChangeFilter(QObject* parent = nullptr) : QObject(parent)
    {
    }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::ApplicationPaletteChange)
            s_systemDarkCached = false;
        return QObject::eventFilter(obj, event);
    }
};

void initThemeSystem()
{
    s_systemDarkValue = systemIsDarkImpl();
    s_systemDarkCached = true;
    static PaletteChangeFilter filter;
    qApp->installEventFilter(&filter);
}

static bool systemIsDark()
{
    if (!s_systemDarkCached)
    {
        s_systemDarkValue = systemIsDarkImpl();
        s_systemDarkCached = true;
    }
    return s_systemDarkValue;
}

int builtInThemeCount()
{
    return static_cast<int>(ThemeId::Count);
}

QList<ThemeId> allBuiltInThemes()
{
    QList<ThemeId> ids;
    for (int i = 0; i < static_cast<int>(ThemeId::Count); ++i)
        ids.append(static_cast<ThemeId>(i));
    return ids;
}

ThemeColors getThemeColors(ThemeId themeId)
{
    static constexpr int NUM_THEMES = static_cast<int>(ThemeId::Count);
    static std::array<ThemeColors, NUM_THEMES> s_cache;
    static std::once_flag s_initFlag;
    std::call_once(s_initFlag,
                   []()
                   {
                       s_cache[static_cast<int>(ThemeId::Light)] = createLightTheme();
                       s_cache[static_cast<int>(ThemeId::Dark)] = createDarkTheme();
                       s_cache[static_cast<int>(ThemeId::Nord)] = createNordTheme();
                       s_cache[static_cast<int>(ThemeId::SolarizedLight)] = createSolarizedLightTheme();
                       s_cache[static_cast<int>(ThemeId::Monokai)] = createMonokaiTheme();
                       s_cache[static_cast<int>(ThemeId::GruvboxDark)] = createGruvboxDarkTheme();
                       s_cache[static_cast<int>(ThemeId::CatppuccinMocha)] = createCatppuccinMochaTheme();
                       s_cache[static_cast<int>(ThemeId::CatppuccinMacchiato)] = createCatppuccinMacchiatoTheme();
                       s_cache[static_cast<int>(ThemeId::CatppuccinFrappe)] = createCatppuccinFrappeTheme();
                       s_cache[static_cast<int>(ThemeId::CatppuccinLatte)] = createCatppuccinLatteTheme();
                       s_cache[static_cast<int>(ThemeId::TokyoNight)] = createTokyoNightTheme();
                       s_cache[static_cast<int>(ThemeId::TokyoNightStorm)] = createTokyoNightStormTheme();
                       s_cache[static_cast<int>(ThemeId::Dracula)] = createDraculaTheme();
                       s_cache[static_cast<int>(ThemeId::OneDark)] = createOneDarkTheme();
                       s_cache[static_cast<int>(ThemeId::AyuLight)] = createAyuLightTheme();
                       s_cache[static_cast<int>(ThemeId::AyuDark)] = createAyuDarkTheme();
                   });

    ThemeId resolved = themeId;
    if (themeId == ThemeId::System)
    {
        resolved = systemIsDark() ? ThemeId::Dark : ThemeId::Light;
    }

    int idx = static_cast<int>(resolved);
    if (idx < 0 || idx >= static_cast<int>(s_cache.size()))
        return s_cache[static_cast<int>(ThemeId::Light)];

    ThemeColors colors = s_cache[idx];
    applyThemePolish(colors, resolved);
    applyAccent(colors);
    return colors;
}

QPalette getThemePalette(const ThemeColors& colors)
{
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
    if (colors.isDark)
    {
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 120));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 120));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));
    }
    else
    {
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(160, 160, 160));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(160, 160, 160));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(160, 160, 160));
    }
    return palette;
}

QString themeDisplayName(ThemeId themeId)
{
    switch (themeId)
    {
    case ThemeId::Light:
        return QObject::tr("Light");
    case ThemeId::Dark:
        return QObject::tr("Dark");
    case ThemeId::System:
        return QObject::tr("System");
    case ThemeId::Nord:
        return QObject::tr("Nord");
    case ThemeId::SolarizedLight:
        return QObject::tr("Solarized Light");
    case ThemeId::Monokai:
        return QObject::tr("Monokai");
    case ThemeId::GruvboxDark:
        return QObject::tr("Gruvbox Dark");
    case ThemeId::CatppuccinMocha:
        return QObject::tr("Catppuccin Mocha");
    case ThemeId::CatppuccinMacchiato:
        return QObject::tr("Catppuccin Macchiato");
    case ThemeId::CatppuccinFrappe:
        return QObject::tr("Catppuccin Frappé");
    case ThemeId::CatppuccinLatte:
        return QObject::tr("Catppuccin Latte");
    case ThemeId::TokyoNight:
        return QObject::tr("Tokyo Night");
    case ThemeId::TokyoNightStorm:
        return QObject::tr("Tokyo Night Storm");
    case ThemeId::Dracula:
        return QObject::tr("Dracula");
    case ThemeId::OneDark:
        return QObject::tr("One Dark");
    case ThemeId::AyuLight:
        return QObject::tr("Ayu Light");
    case ThemeId::AyuDark:
        return QObject::tr("Ayu Dark");
    case ThemeId::Count:
        return {};
    }
    return QObject::tr("Light");
}

bool isThemeDark(ThemeId themeId)
{
    if (themeId == ThemeId::System)
        return systemIsDark();
    static constexpr std::array<bool, 17> darkMap = {
        false, // Light
        true,  // Dark
        false, // System (unused, resolved above)
        true,  // Nord
        false, // SolarizedLight
        true,  // Monokai
        true,  // GruvboxDark
        true,  // CatppuccinMocha
        true,  // CatppuccinMacchiato
        true,  // CatppuccinFrappe
        false, // CatppuccinLatte
        true,  // TokyoNight
        true,  // TokyoNightStorm
        true,  // Dracula
        true,  // OneDark
        false, // AyuLight
        true,  // AyuDark
    };
    int idx = static_cast<int>(themeId);
    if (idx < 0 || idx >= static_cast<int>(ThemeId::Count))
        return false;
    return darkMap[static_cast<size_t>(idx)];
}

bool prefersNativeStyling(ThemeId themeId)
{
    return themeId == ThemeId::System;
}
