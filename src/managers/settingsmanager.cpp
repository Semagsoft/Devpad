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
#include "settingsmanager.h"

#include "codeeditor.h"
#include "theme.h"

#include <QFileInfo>

#include <Qsci/qsciscintilla.h>

static const QHash<QString, QString>& buildExtensionMap()
{
    static QHash<QString, QString> map = {
        {"cpp", "cpp"},
        {"c", "c"},
        {"h", "cpp"},
        {"hpp", "cpp"},
        {"cc", "cpp"},
        {"cxx", "cpp"},
        {"hxx", "cpp"},
        {"c++", "cpp"},
        {"h++", "cpp"},
        {"cs", "csharp"},
        {"java", "java"},
        {"py", "python"},
        {"pyw", "python"},
        {"pyi", "python"},
        {"js", "javascript"},
        {"mjs", "javascript"},
        {"cjs", "javascript"},
        {"html", "html"},
        {"htm", "html"},
        {"xhtml", "html"},
        {"css", "css"},
        {"xml", "xml"},
        {"xaml", "xml"},
        {"xsl", "xml"},
        {"xslt", "xml"},
        {"svg", "xml"},
        {"sql", "sql"},
        {"ddl", "sql"},
        {"ts", "typescript"},
        {"tsx", "typescript"},
        {"rs", "rust"},
        {"go", "go"},
        {"md", "markdown"},
        {"mdx", "markdown"},
        {"markdown", "markdown"},
        {"sh", "bash"},
        {"bash", "bash"},
        {"zsh", "bash"},
        {"ksh", "bash"},
        {"fish", "bash"},
        {"cmake", "cmake"},
        {"txt", "text"},
    };
    return map;
}

static const QHash<QString, QString>& buildFileNameMap()
{
    static QHash<QString, QString> map = {
        {"cmakelists.txt", "cmake"},
        {"cmakelists.txt.in", "cmake"},
        {"cmakecache.txt", "cmake"},
    };
    return map;
}

const QHash<QString, QString>& SettingsManager::extensionMap()
{
    return buildExtensionMap();
}

const QHash<QString, QString>& SettingsManager::fileNameMap()
{
    return buildFileNameMap();
}

SettingsManager* SettingsManager::s_testInstance = nullptr;

void SettingsManager::setTestingInstance(SettingsManager* instance)
{
    s_testInstance = instance;
}

SettingsManager* SettingsManager::createForTesting()
{
    return new SettingsManager();
}

void SettingsManager::destroyForTesting(SettingsManager* instance)
{
    delete instance;
}

SettingsManager& SettingsManager::instance()
{
    if (s_testInstance)
    {
        return *s_testInstance;
    }
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager()
{
    ensureSettingsVersion();
    loadCache();
}

void SettingsManager::ensureSettingsVersion()
{
    int savedVersion = m_settings.value("SettingsVersion", 0).toInt();
    if (savedVersion < CurrentSettingsVersion)
    {
        // Future: add per-version migration steps here
        m_settings.setValue("SettingsVersion", CurrentSettingsVersion);
    }
}

void SettingsManager::loadCache()
{
    QMutexLocker locker(&m_cacheMutex);
    m_cache.editor.defaultFontFamily = m_settings.value("Options_DefaultFont", "Monospace").toString();
    m_cache.editor.defaultFontSize = m_settings.value("Options_DefaultFontSize", 12).toInt();
    m_cache.editor.showLineNumbers = m_settings.value("Options_ShowLineNumbers", true).toBool();
    m_cache.editor.scrollPastContent = m_settings.value("Options_ScrollPastContent", false).toBool();
    m_cache.editor.codeCollapsing = m_settings.value("Options_CodeCollapsing", false).toBool();
    m_cache.editor.wordWrap = m_settings.value("Options_WordWrap", false).toBool();
    m_cache.editor.theme = static_cast<ThemeId>(m_settings.value("Options_Theme", static_cast<int>(ThemeId::Light)).toInt());
    m_cache.editor.defaultEncoding = m_settings.value("Options_DefaultEncoding", 0).toInt();
    m_cache.editor.defaultFormat = m_settings.value("Options_DefaultFormat", 0).toInt();
    m_cache.editor.showWhitespace = m_settings.value("Options_ShowWhitespace", false).toBool();
    m_cache.editor.autoCompletionEnabled = m_settings.value("Options_AutoCompletion", true).toBool();
    m_cache.editor.autoCompletionThreshold = m_settings.value("Options_AutoCompletionThreshold", 2).toInt();
    m_cache.editor.autoCompletionCaseSensitive = m_settings.value("Options_AutoCompletionCaseSensitive", false).toBool();
    m_cache.editor.autoCloseBrackets = m_settings.value("Options_AutoCloseBrackets", true).toBool();
    m_cache.editor.tabWidth = m_settings.value("Options_TabWidth", 4).toInt();
    m_cache.editor.cursorStyle = static_cast<CursorStyle>(m_settings.value("Options_CursorStyle", static_cast<int>(CursorStyle::Line)).toInt());
    m_cache.editor.cursorBlinking = m_settings.value("Options_CursorBlinking", true).toBool();
    m_cache.editor.highlightCurrentLine = m_settings.value("Options_HighlightCurrentLine", true).toBool();
    m_cache.editor.verticalEdgeEnabled = m_settings.value("Options_VerticalEdgeEnabled", false).toBool();
    m_cache.editor.verticalEdgeColumn = m_settings.value("Options_VerticalEdgeColumn", 80).toInt();
    m_cache.editor.snippetsEnabled = m_settings.value("Options_SnippetsEnabled", true).toBool();
    m_cache.editor.predictiveSnippets = m_settings.value("Options_PredictiveSnippets", true).toBool();

    m_cache.ui.startupMode = static_cast<StartupMode>(m_settings.value("Options_StartupMode", static_cast<int>(StartupMode::NewFile)).toInt());
    m_cache.ui.closeButtonMode =
        static_cast<CloseButtonMode>(m_settings.value("Options_CloseButtonMode", static_cast<int>(CloseButtonMode::Right)).toInt());
    m_cache.ui.tabDisplayMode =
        static_cast<TabDisplayMode>(m_settings.value("Options_TabDisplayMode", static_cast<int>(TabDisplayMode::ShowTwoPlus)).toInt());
    m_cache.ui.tabBarPosition =
        static_cast<TabBarPosition>(m_settings.value("Options_TabBarPosition", static_cast<int>(TabBarPosition::Top)).toInt());
    m_cache.ui.showMenuBar = m_settings.value("Options_ShowMenuBar", true).toBool();
    m_cache.ui.showToolbar = m_settings.value("Options_ShowToolbar", true).toBool();
    m_cache.ui.showStatusbar = m_settings.value("Options_ShowStatusbar", true).toBool();
    m_cache.ui.uiFontFamily = m_settings.value("Options_UIFont", "Sans Serif").toString();
    m_cache.ui.uiFontSize = m_settings.value("Options_UIFontSize", 10).toInt();

    m_cache.terminal.terminalPanelPosition = static_cast<TerminalPanelPosition>(
        m_settings.value("Options_TerminalPanelPosition", static_cast<int>(TerminalPanelPosition::Bottom)).toInt());
    m_cache.terminal.terminalPanelMinWidth = m_settings.value("Options_TerminalPanelMinWidth", 350).toInt();
    m_cache.terminal.terminalFontFamily = m_settings.value("Options_TerminalFont", "Monospace").toString();
    m_cache.terminal.terminalFontSize = m_settings.value("Options_TerminalFontSize", 12).toInt();
    m_cache.terminal.showTerminalPanel = m_settings.value("Options_ShowTerminalPanel", false).toBool();

    m_cache.autoSave.autoSaveEnabled = m_settings.value("Options_AutoSaveEnabled", false).toBool();
    m_cache.autoSave.autoSaveInterval = m_settings.value("Options_AutoSaveInterval", 60).toInt();
    m_cache.autoSave.autoSaveToOriginalFile = m_settings.value("Options_AutoSaveToOriginalFile", true).toBool();

    m_cache.project.showHiddenFiles = m_settings.value("Options_ShowHiddenFiles", false).toBool();
    m_cache.project.projectPanelPosition = static_cast<ProjectPanelPosition>(
        m_settings.value("Options_ProjectPanelPosition", static_cast<int>(ProjectPanelPosition::Left)).toInt());
}

SettingsManager::EditorSettings SettingsManager::editorSettings() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor;
}

SettingsManager::UISettings SettingsManager::uiSettings() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.ui;
}

SettingsManager::TerminalSettings SettingsManager::terminalSettings() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.terminal;
}

SettingsManager::AutoSaveSettings SettingsManager::autoSaveSettings() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.autoSave;
}

QString SettingsManager::defaultFontFamily() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.defaultFontFamily;
}

int SettingsManager::defaultFontSize() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.defaultFontSize;
}

QFont SettingsManager::defaultFont() const
{
    return QFont(defaultFontFamily(), defaultFontSize());
}

bool SettingsManager::showLineNumbers() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.showLineNumbers;
}

bool SettingsManager::scrollPastContent() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.scrollPastContent;
}

bool SettingsManager::codeCollapsing() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.codeCollapsing;
}

bool SettingsManager::wordWrap() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.wordWrap;
}

ThemeId SettingsManager::theme() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.theme;
}

bool SettingsManager::isDarkTheme() const
{
    return isThemeDark(theme());
}

ThemeColors SettingsManager::currentThemeColors() const
{
    return getThemeColors(theme());
}

int SettingsManager::defaultEncoding() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.defaultEncoding;
}

int SettingsManager::defaultFormat() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.defaultFormat;
}

bool SettingsManager::showWhitespace() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.showWhitespace;
}

bool SettingsManager::autoCompletionEnabled() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.autoCompletionEnabled;
}

int SettingsManager::autoCompletionThreshold() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.autoCompletionThreshold;
}

bool SettingsManager::autoCompletionCaseSensitive() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.autoCompletionCaseSensitive;
}

bool SettingsManager::autoCloseBrackets() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.autoCloseBrackets;
}

int SettingsManager::tabWidth() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.tabWidth;
}

CursorStyle SettingsManager::cursorStyle() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.cursorStyle;
}

bool SettingsManager::cursorBlinking() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.cursorBlinking;
}

bool SettingsManager::highlightCurrentLine() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.highlightCurrentLine;
}

bool SettingsManager::verticalEdgeEnabled() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.verticalEdgeEnabled;
}

int SettingsManager::verticalEdgeColumn() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.verticalEdgeColumn;
}

CloseButtonMode SettingsManager::closeButtonMode() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.ui.closeButtonMode;
}

bool SettingsManager::showMenuBar() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.ui.showMenuBar;
}

bool SettingsManager::showToolbar() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.ui.showToolbar;
}

bool SettingsManager::showStatusbar() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.ui.showStatusbar;
}

TabDisplayMode SettingsManager::tabDisplayMode() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.ui.tabDisplayMode;
}

TabBarPosition SettingsManager::tabBarPosition() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.ui.tabBarPosition;
}

QString SettingsManager::uiFontFamily() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.ui.uiFontFamily;
}

int SettingsManager::uiFontSize() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.ui.uiFontSize;
}

QFont SettingsManager::uiFont() const
{
    return QFont(uiFontFamily(), uiFontSize());
}

StartupMode SettingsManager::startupMode() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.ui.startupMode;
}

TerminalPanelPosition SettingsManager::terminalPanelPosition() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.terminal.terminalPanelPosition;
}

int SettingsManager::terminalPanelMinWidth() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.terminal.terminalPanelMinWidth;
}

QString SettingsManager::terminalFontFamily() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.terminal.terminalFontFamily;
}

int SettingsManager::terminalFontSize() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.terminal.terminalFontSize;
}

QFont SettingsManager::terminalFont() const
{
    return QFont(terminalFontFamily(), terminalFontSize());
}

bool SettingsManager::showTerminalPanel() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.terminal.showTerminalPanel;
}

bool SettingsManager::autoSaveEnabled() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.autoSave.autoSaveEnabled;
}

int SettingsManager::autoSaveInterval() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.autoSave.autoSaveInterval;
}

bool SettingsManager::autoSaveToOriginalFile() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.autoSave.autoSaveToOriginalFile;
}

void SettingsManager::applyToEditor(CodeEditor* editor) const
{
    if (!editor)
        return;

    editor->applyFont(defaultFont());
    editor->setLineNumbersVisible(showLineNumbers());
    editor->setScrollPastEnd(scrollPastContent());
    editor->setCodeFolding(codeCollapsing());
    editor->setWrapMode(wordWrap() ? QsciScintilla::WrapWord : QsciScintilla::WrapNone);
    editor->setWhitespaceVisible(showWhitespace());
    editor->applyTheme(theme());
    editor->setupAutoCompletion(autoCompletionEnabled(), autoCompletionThreshold(), autoCompletionCaseSensitive());
    editor->setAutoCloseBrackets(autoCloseBrackets());
    editor->setTabWidth(tabWidth());
    editor->setCursorStyle(cursorStyle());
    editor->setCursorBlinking(cursorBlinking());
    editor->setHighlightCurrentLine(highlightCurrentLine());
    editor->setVerticalEdge(verticalEdgeEnabled(), verticalEdgeColumn());
}

void SettingsManager::setDefaultFontFamily(const QString& family)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_DefaultFont", family);
    m_cache.editor.defaultFontFamily = family;
}

void SettingsManager::setDefaultFontSize(int size)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_DefaultFontSize", size);
    m_cache.editor.defaultFontSize = size;
}

void SettingsManager::setShowLineNumbers(bool visible)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_ShowLineNumbers", visible);
    m_cache.editor.showLineNumbers = visible;
}

void SettingsManager::setScrollPastContent(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_ScrollPastContent", enabled);
    m_cache.editor.scrollPastContent = enabled;
}

void SettingsManager::setCodeCollapsing(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_CodeCollapsing", enabled);
    m_cache.editor.codeCollapsing = enabled;
}

void SettingsManager::setWordWrap(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_WordWrap", enabled);
    m_cache.editor.wordWrap = enabled;
}

void SettingsManager::setTheme(ThemeId theme)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_Theme", static_cast<int>(theme));
    m_cache.editor.theme = theme;
}

QColor SettingsManager::accentColor() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_settings.value("Options_AccentColor", QColor()).value<QColor>();
}

bool SettingsManager::hasAccentColor() const
{
    QMutexLocker locker(&m_cacheMutex);
    QVariant v = m_settings.value("Options_AccentColor");
    return v.isValid() && v.value<QColor>().isValid();
}

void SettingsManager::setAccentColor(const QColor &color)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_AccentColor", color);
}

void SettingsManager::clearAccentColor()
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.remove("Options_AccentColor");
}

void SettingsManager::setDefaultEncoding(int encoding)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_DefaultEncoding", encoding);
    m_cache.editor.defaultEncoding = encoding;
}

void SettingsManager::setDefaultFormat(int format)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_DefaultFormat", format);
    m_cache.editor.defaultFormat = format;
}

void SettingsManager::setShowWhitespace(bool visible)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_ShowWhitespace", visible);
    m_cache.editor.showWhitespace = visible;
}

void SettingsManager::setAutoCompletionEnabled(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_AutoCompletion", enabled);
    m_cache.editor.autoCompletionEnabled = enabled;
}

void SettingsManager::setAutoCompletionThreshold(int threshold)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_AutoCompletionThreshold", threshold);
    m_cache.editor.autoCompletionThreshold = threshold;
}

void SettingsManager::setAutoCompletionCaseSensitive(bool sensitive)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_AutoCompletionCaseSensitive", sensitive);
    m_cache.editor.autoCompletionCaseSensitive = sensitive;
}

void SettingsManager::setAutoCloseBrackets(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_AutoCloseBrackets", enabled);
    m_cache.editor.autoCloseBrackets = enabled;
}

void SettingsManager::setTabWidth(int width)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_TabWidth", width);
    m_cache.editor.tabWidth = width;
}

void SettingsManager::setCursorStyle(CursorStyle style)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_CursorStyle", static_cast<int>(style));
    m_cache.editor.cursorStyle = style;
}

void SettingsManager::setCursorBlinking(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_CursorBlinking", enabled);
    m_cache.editor.cursorBlinking = enabled;
}

void SettingsManager::setHighlightCurrentLine(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_HighlightCurrentLine", enabled);
    m_cache.editor.highlightCurrentLine = enabled;
}

void SettingsManager::setVerticalEdgeEnabled(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_VerticalEdgeEnabled", enabled);
    m_cache.editor.verticalEdgeEnabled = enabled;
}

void SettingsManager::setVerticalEdgeColumn(int column)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_VerticalEdgeColumn", column);
    m_cache.editor.verticalEdgeColumn = column;
}

bool SettingsManager::snippetsEnabled() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.snippetsEnabled;
}

bool SettingsManager::predictiveSnippets() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.editor.predictiveSnippets;
}

void SettingsManager::setSnippetsEnabled(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_SnippetsEnabled", enabled);
    m_cache.editor.snippetsEnabled = enabled;
}

void SettingsManager::setPredictiveSnippets(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_PredictiveSnippets", enabled);
    m_cache.editor.predictiveSnippets = enabled;
}

void SettingsManager::setStartupMode(StartupMode mode)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_StartupMode", static_cast<int>(mode));
    m_cache.ui.startupMode = mode;
}

void SettingsManager::setCloseButtonMode(CloseButtonMode mode)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_CloseButtonMode", static_cast<int>(mode));
    m_cache.ui.closeButtonMode = mode;
}

void SettingsManager::setTabDisplayMode(TabDisplayMode mode)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_TabDisplayMode", static_cast<int>(mode));
    m_cache.ui.tabDisplayMode = mode;
}

void SettingsManager::setTabBarPosition(TabBarPosition position)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_TabBarPosition", static_cast<int>(position));
    m_cache.ui.tabBarPosition = position;
}

void SettingsManager::setShowMenuBar(bool visible)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_ShowMenuBar", visible);
    m_cache.ui.showMenuBar = visible;
}

void SettingsManager::setShowToolbar(bool visible)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_ShowToolbar", visible);
    m_cache.ui.showToolbar = visible;
}

void SettingsManager::setShowStatusbar(bool visible)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_ShowStatusbar", visible);
    m_cache.ui.showStatusbar = visible;
}

void SettingsManager::setUiFontFamily(const QString& family)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_UIFont", family);
    m_cache.ui.uiFontFamily = family;
}

void SettingsManager::setUiFontSize(int size)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_UIFontSize", size);
    m_cache.ui.uiFontSize = size;
}

void SettingsManager::setTerminalPanelPosition(TerminalPanelPosition position)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_TerminalPanelPosition", static_cast<int>(position));
    m_cache.terminal.terminalPanelPosition = position;
}

void SettingsManager::setTerminalPanelMinWidth(int width)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_TerminalPanelMinWidth", width);
    m_cache.terminal.terminalPanelMinWidth = width;
}

void SettingsManager::setTerminalFontFamily(const QString& family)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_TerminalFont", family);
    m_cache.terminal.terminalFontFamily = family;
}

void SettingsManager::setTerminalFontSize(int size)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_TerminalFontSize", size);
    m_cache.terminal.terminalFontSize = size;
}

void SettingsManager::setShowTerminalPanel(bool visible)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_ShowTerminalPanel", visible);
    m_cache.terminal.showTerminalPanel = visible;
}

void SettingsManager::setAutoSaveEnabled(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_AutoSaveEnabled", enabled);
    m_cache.autoSave.autoSaveEnabled = enabled;
}

void SettingsManager::setAutoSaveInterval(int seconds)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_AutoSaveInterval", seconds);
    m_cache.autoSave.autoSaveInterval = seconds;
}

void SettingsManager::setAutoSaveToOriginalFile(bool enabled)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_AutoSaveToOriginalFile", enabled);
    m_cache.autoSave.autoSaveToOriginalFile = enabled;
}

SettingsManager::ProjectSettings SettingsManager::projectSettings() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.project;
}

bool SettingsManager::showHiddenFiles() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.project.showHiddenFiles;
}

ProjectPanelPosition SettingsManager::projectPanelPosition() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_cache.project.projectPanelPosition;
}

void SettingsManager::setShowHiddenFiles(bool visible)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_ShowHiddenFiles", visible);
    m_cache.project.showHiddenFiles = visible;
}

void SettingsManager::setProjectPanelPosition(ProjectPanelPosition position)
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.setValue("Options_ProjectPanelPosition", static_cast<int>(position));
    m_cache.project.projectPanelPosition = position;
}

QString SettingsManager::syntaxForExtension(const QString& ext) const
{
    return extensionMap().value(ext.toLower());
}

QString SettingsManager::syntaxForFile(const QString& filePath) const
{
    QString fileName = QFileInfo(filePath).fileName();
    QString lang = fileNameMap().value(fileName.toLower());
    if (!lang.isEmpty())
        return lang;
    return syntaxForExtension(QFileInfo(filePath).suffix());
}

int SettingsManager::externalToolCount() const {
    QMutexLocker locker(&m_cacheMutex);
    return m_settings.value("ExternalTools/Count", 0).toInt();
}

QString SettingsManager::externalToolName(int index) const {
    QMutexLocker locker(&m_cacheMutex);
    return m_settings.value(QString("ExternalTools/%1/Name").arg(index)).toString();
}

QString SettingsManager::externalToolCommand(int index) const {
    QMutexLocker locker(&m_cacheMutex);
    return m_settings.value(QString("ExternalTools/%1/Command").arg(index)).toString();
}

QString SettingsManager::externalToolArguments(int index) const {
    QMutexLocker locker(&m_cacheMutex);
    return m_settings.value(QString("ExternalTools/%1/Arguments").arg(index)).toString();
}

QString SettingsManager::externalToolWorkingDir(int index) const {
    QMutexLocker locker(&m_cacheMutex);
    return m_settings.value(QString("ExternalTools/%1/WorkingDir").arg(index)).toString();
}

QString SettingsManager::externalToolShortcut(int index) const {
    QMutexLocker locker(&m_cacheMutex);
    return m_settings.value(QString("ExternalTools/%1/Shortcut").arg(index)).toString();
}

bool SettingsManager::externalToolRunInTerminal(int index) const {
    QMutexLocker locker(&m_cacheMutex);
    return m_settings.value(QString("ExternalTools/%1/RunInTerminal").arg(index), true).toBool();
}

void SettingsManager::setExternalTool(int index, const QString& name, const QString& command,
                                       const QString& arguments, const QString& workingDir,
                                       const QString& shortcut, bool runInTerminal) {
    QMutexLocker locker(&m_cacheMutex);
    QString prefix = QString("ExternalTools/%1/").arg(index);
    m_settings.setValue(prefix + "Name", name);
    m_settings.setValue(prefix + "Command", command);
    m_settings.setValue(prefix + "Arguments", arguments);
    m_settings.setValue(prefix + "WorkingDir", workingDir);
    m_settings.setValue(prefix + "Shortcut", shortcut);
    m_settings.setValue(prefix + "RunInTerminal", runInTerminal);
}

void SettingsManager::addExternalTool(const QString& name, const QString& command,
                                       const QString& arguments, const QString& workingDir,
                                       const QString& shortcut, bool runInTerminal) {
    QMutexLocker locker(&m_cacheMutex);
    int count = m_settings.value("ExternalTools/Count", 0).toInt();
    m_settings.setValue("ExternalTools/Count", count + 1);
    QString prefix = QString("ExternalTools/%1/").arg(count);
    m_settings.setValue(prefix + "Name", name);
    m_settings.setValue(prefix + "Command", command);
    m_settings.setValue(prefix + "Arguments", arguments);
    m_settings.setValue(prefix + "WorkingDir", workingDir);
    m_settings.setValue(prefix + "Shortcut", shortcut);
    m_settings.setValue(prefix + "RunInTerminal", runInTerminal);
}

void SettingsManager::removeExternalTool(int index) {
    QMutexLocker locker(&m_cacheMutex);
    int count = m_settings.value("ExternalTools/Count", 0).toInt();
    if (index < 0 || index >= count) return;

    QString prefix = QString("ExternalTools/%1/").arg(index);
    m_settings.remove(prefix + "Name");
    m_settings.remove(prefix + "Command");
    m_settings.remove(prefix + "Arguments");
    m_settings.remove(prefix + "WorkingDir");
    m_settings.remove(prefix + "Shortcut");
    m_settings.remove(prefix + "RunInTerminal");

    for (int i = index + 1; i < count; ++i) {
        QString oldPrefix = QString("ExternalTools/%1/").arg(i);
        QString newPrefix = QString("ExternalTools/%1/").arg(i - 1);
        m_settings.setValue(newPrefix + "Name", m_settings.value(oldPrefix + "Name"));
        m_settings.setValue(newPrefix + "Command", m_settings.value(oldPrefix + "Command"));
        m_settings.setValue(newPrefix + "Arguments", m_settings.value(oldPrefix + "Arguments"));
        m_settings.setValue(newPrefix + "WorkingDir", m_settings.value(oldPrefix + "WorkingDir"));
        m_settings.setValue(newPrefix + "Shortcut", m_settings.value(oldPrefix + "Shortcut"));
        m_settings.setValue(newPrefix + "RunInTerminal", m_settings.value(oldPrefix + "RunInTerminal"));

        m_settings.remove(oldPrefix + "Name");
        m_settings.remove(oldPrefix + "Command");
        m_settings.remove(oldPrefix + "Arguments");
        m_settings.remove(oldPrefix + "WorkingDir");
        m_settings.remove(oldPrefix + "Shortcut");
        m_settings.remove(oldPrefix + "RunInTerminal");
    }
    m_settings.setValue("ExternalTools/Count", count - 1);
}

void SettingsManager::addRecentFile(const QString& filePath)
{
    QMutexLocker locker(&m_cacheMutex);
    QStringList recentFiles = m_settings.value("recentFiles").toStringList();
    recentFiles.removeAll(filePath);
    recentFiles.prepend(filePath);
    while (recentFiles.size() > 10)
    {
        recentFiles.removeLast();
    }
    m_settings.setValue("recentFiles", recentFiles);
}

QStringList SettingsManager::recentFiles() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_settings.value("recentFiles").toStringList();
}

void SettingsManager::clearRecentFiles()
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.remove("recentFiles");
}

void SettingsManager::addRecentFolder(const QString& folderPath)
{
    QMutexLocker locker(&m_cacheMutex);
    QStringList recentFolders = m_settings.value("recentFolders").toStringList();
    recentFolders.removeAll(folderPath);
    recentFolders.prepend(folderPath);
    while (recentFolders.size() > 10)
    {
        recentFolders.removeLast();
    }
    m_settings.setValue("recentFolders", recentFolders);
}

QStringList SettingsManager::recentFolders() const
{
    QMutexLocker locker(&m_cacheMutex);
    return m_settings.value("recentFolders").toStringList();
}

void SettingsManager::clearRecentFolders()
{
    QMutexLocker locker(&m_cacheMutex);
    m_settings.remove("recentFolders");
}

