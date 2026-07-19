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

std::unique_ptr<SettingsManager> SettingsManager::createForTesting()
{
    return std::unique_ptr<SettingsManager>(new SettingsManager());
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

SettingsManager::SettingsManager() : QObject(nullptr)
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
    m_cache.editor.defaultFontFamily = m_settings.value("Options_DefaultFont", "Monospace").toString();
    m_cache.editor.defaultFontSize = m_settings.value("Options_DefaultFontSize", 12).toInt();
    m_cache.editor.showLineNumbers = m_settings.value("Options_ShowLineNumbers", true).toBool();
    m_cache.editor.scrollPastContent = m_settings.value("Options_ScrollPastContent", false).toBool();
    m_cache.editor.codeCollapsing = m_settings.value("Options_CodeCollapsing", false).toBool();
    m_cache.editor.wordWrap = m_settings.value("Options_WordWrap", false).toBool();
    {
        int themeVal = m_settings.value("Options_Theme", static_cast<int>(ThemeId::Light)).toInt();
        if (themeVal < 0 || themeVal >= static_cast<int>(ThemeId::Count))
            themeVal = static_cast<int>(ThemeId::Light);
        m_cache.editor.theme = static_cast<ThemeId>(themeVal);
    }
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
    m_cache.project.useGitIgnore = m_settings.value("Options_UseGitIgnore", true).toBool();
    m_cache.project.projectPanelPosition =
        static_cast<ProjectPanelPosition>(m_settings.value("Options_ProjectPanelPosition", static_cast<int>(ProjectPanelPosition::Left)).toInt());

    m_cache.lsp.enabled = m_settings.value("LSP/Enabled", true).toBool();
    m_cache.lsp.showErrorList = m_settings.value("LSP/ShowErrorList", true).toBool();
    m_cache.lsp.completionTriggerChars = m_settings.value("LSP/CompletionTriggerChars", 2).toInt();

    QVariant accentVar = m_settings.value("Options_AccentColor");
    m_cache.hasAccentColor = accentVar.isValid() && accentVar.value<QColor>().isValid();
    if (m_cache.hasAccentColor)
        m_cache.accentColor = accentVar.value<QColor>();
}

SettingsManager::EditorSettings SettingsManager::editorSettings() const
{
    return m_cache.editor;
}

SettingsManager::UISettings SettingsManager::uiSettings() const
{
    return m_cache.ui;
}

SettingsManager::TerminalSettings SettingsManager::terminalSettings() const
{
    return m_cache.terminal;
}

SettingsManager::AutoSaveSettings SettingsManager::autoSaveSettings() const
{
    return m_cache.autoSave;
}

QString SettingsManager::defaultFontFamily() const
{
    return m_cache.editor.defaultFontFamily;
}
int SettingsManager::defaultFontSize() const
{
    return m_cache.editor.defaultFontSize;
}
QFont SettingsManager::defaultFont() const
{
    return QFont(defaultFontFamily(), defaultFontSize());
}
bool SettingsManager::showLineNumbers() const
{
    return m_cache.editor.showLineNumbers;
}
bool SettingsManager::scrollPastContent() const
{
    return m_cache.editor.scrollPastContent;
}
bool SettingsManager::codeCollapsing() const
{
    return m_cache.editor.codeCollapsing;
}
bool SettingsManager::wordWrap() const
{
    return m_cache.editor.wordWrap;
}
ThemeId SettingsManager::theme() const
{
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
    return m_cache.editor.defaultEncoding;
}
int SettingsManager::defaultFormat() const
{
    return m_cache.editor.defaultFormat;
}
bool SettingsManager::showWhitespace() const
{
    return m_cache.editor.showWhitespace;
}
bool SettingsManager::autoCompletionEnabled() const
{
    return m_cache.editor.autoCompletionEnabled;
}
int SettingsManager::autoCompletionThreshold() const
{
    return m_cache.editor.autoCompletionThreshold;
}
bool SettingsManager::autoCompletionCaseSensitive() const
{
    return m_cache.editor.autoCompletionCaseSensitive;
}
bool SettingsManager::autoCloseBrackets() const
{
    return m_cache.editor.autoCloseBrackets;
}
int SettingsManager::tabWidth() const
{
    return m_cache.editor.tabWidth;
}
CursorStyle SettingsManager::cursorStyle() const
{
    return m_cache.editor.cursorStyle;
}
bool SettingsManager::cursorBlinking() const
{
    return m_cache.editor.cursorBlinking;
}
bool SettingsManager::highlightCurrentLine() const
{
    return m_cache.editor.highlightCurrentLine;
}
bool SettingsManager::verticalEdgeEnabled() const
{
    return m_cache.editor.verticalEdgeEnabled;
}
int SettingsManager::verticalEdgeColumn() const
{
    return m_cache.editor.verticalEdgeColumn;
}

CloseButtonMode SettingsManager::closeButtonMode() const
{
    return m_cache.ui.closeButtonMode;
}
bool SettingsManager::showMenuBar() const
{
    return m_cache.ui.showMenuBar;
}
bool SettingsManager::showToolbar() const
{
    return m_cache.ui.showToolbar;
}
bool SettingsManager::showStatusbar() const
{
    return m_cache.ui.showStatusbar;
}
TabDisplayMode SettingsManager::tabDisplayMode() const
{
    return m_cache.ui.tabDisplayMode;
}
TabBarPosition SettingsManager::tabBarPosition() const
{
    return m_cache.ui.tabBarPosition;
}
QString SettingsManager::uiFontFamily() const
{
    return m_cache.ui.uiFontFamily;
}
int SettingsManager::uiFontSize() const
{
    return m_cache.ui.uiFontSize;
}
QFont SettingsManager::uiFont() const
{
    return QFont(uiFontFamily(), uiFontSize());
}
StartupMode SettingsManager::startupMode() const
{
    return m_cache.ui.startupMode;
}
TerminalPanelPosition SettingsManager::terminalPanelPosition() const
{
    return m_cache.terminal.terminalPanelPosition;
}
int SettingsManager::terminalPanelMinWidth() const
{
    return m_cache.terminal.terminalPanelMinWidth;
}
QString SettingsManager::terminalFontFamily() const
{
    return m_cache.terminal.terminalFontFamily;
}
int SettingsManager::terminalFontSize() const
{
    return m_cache.terminal.terminalFontSize;
}
QFont SettingsManager::terminalFont() const
{
    return QFont(terminalFontFamily(), terminalFontSize());
}
bool SettingsManager::showTerminalPanel() const
{
    return m_cache.terminal.showTerminalPanel;
}
bool SettingsManager::autoSaveEnabled() const
{
    return m_cache.autoSave.autoSaveEnabled;
}
int SettingsManager::autoSaveInterval() const
{
    return m_cache.autoSave.autoSaveInterval;
}
bool SettingsManager::autoSaveToOriginalFile() const
{
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
    writeCached("Options_DefaultFont", m_cache.editor.defaultFontFamily, family);
}
void SettingsManager::setDefaultFontSize(int size)
{
    writeCached("Options_DefaultFontSize", m_cache.editor.defaultFontSize, size);
}
void SettingsManager::setShowLineNumbers(bool visible)
{
    writeCached("Options_ShowLineNumbers", m_cache.editor.showLineNumbers, visible);
}
void SettingsManager::setScrollPastContent(bool enabled)
{
    writeCached("Options_ScrollPastContent", m_cache.editor.scrollPastContent, enabled);
}
void SettingsManager::setCodeCollapsing(bool enabled)
{
    writeCached("Options_CodeCollapsing", m_cache.editor.codeCollapsing, enabled);
}
void SettingsManager::setWordWrap(bool enabled)
{
    writeCached("Options_WordWrap", m_cache.editor.wordWrap, enabled);
}
void SettingsManager::setTheme(ThemeId theme)
{
    writeCached("Options_Theme", m_cache.editor.theme, theme);
}
QColor SettingsManager::accentColor() const
{
    return m_cache.accentColor;
}

bool SettingsManager::hasAccentColor() const
{
    return m_cache.hasAccentColor;
}

void SettingsManager::setAccentColor(const QColor& color)
{
    m_cache.accentColor = color;
    m_cache.hasAccentColor = color.isValid();
    m_settings.setValue("Options_AccentColor", color);
    settingsChanged();
}

void SettingsManager::clearAccentColor()
{
    m_cache.accentColor = QColor();
    m_cache.hasAccentColor = false;
    m_settings.remove("Options_AccentColor");
}
void SettingsManager::setDefaultEncoding(int encoding)
{
    writeCached("Options_DefaultEncoding", m_cache.editor.defaultEncoding, encoding);
}
void SettingsManager::setDefaultFormat(int format)
{
    writeCached("Options_DefaultFormat", m_cache.editor.defaultFormat, format);
}

void SettingsManager::setShowWhitespace(bool visible)
{
    writeCached("Options_ShowWhitespace", m_cache.editor.showWhitespace, visible);
}
void SettingsManager::setAutoCompletionEnabled(bool enabled)
{
    writeCached("Options_AutoCompletion", m_cache.editor.autoCompletionEnabled, enabled);
}
void SettingsManager::setAutoCompletionThreshold(int threshold)
{
    writeCached("Options_AutoCompletionThreshold", m_cache.editor.autoCompletionThreshold, threshold);
}
void SettingsManager::setAutoCompletionCaseSensitive(bool sensitive)
{
    writeCached("Options_AutoCompletionCaseSensitive", m_cache.editor.autoCompletionCaseSensitive, sensitive);
}
void SettingsManager::setAutoCloseBrackets(bool enabled)
{
    writeCached("Options_AutoCloseBrackets", m_cache.editor.autoCloseBrackets, enabled);
}
void SettingsManager::setTabWidth(int width)
{
    writeCached("Options_TabWidth", m_cache.editor.tabWidth, width);
}
void SettingsManager::setCursorStyle(CursorStyle style)
{
    writeCached("Options_CursorStyle", m_cache.editor.cursorStyle, style);
}
void SettingsManager::setCursorBlinking(bool enabled)
{
    writeCached("Options_CursorBlinking", m_cache.editor.cursorBlinking, enabled);
}
void SettingsManager::setHighlightCurrentLine(bool enabled)
{
    writeCached("Options_HighlightCurrentLine", m_cache.editor.highlightCurrentLine, enabled);
}
void SettingsManager::setVerticalEdgeEnabled(bool enabled)
{
    writeCached("Options_VerticalEdgeEnabled", m_cache.editor.verticalEdgeEnabled, enabled);
}
void SettingsManager::setVerticalEdgeColumn(int column)
{
    writeCached("Options_VerticalEdgeColumn", m_cache.editor.verticalEdgeColumn, column);
}
bool SettingsManager::snippetsEnabled() const
{
    return m_cache.editor.snippetsEnabled;
}
bool SettingsManager::predictiveSnippets() const
{
    return m_cache.editor.predictiveSnippets;
}
void SettingsManager::setSnippetsEnabled(bool enabled)
{
    writeCached("Options_SnippetsEnabled", m_cache.editor.snippetsEnabled, enabled);
}
void SettingsManager::setPredictiveSnippets(bool enabled)
{
    writeCached("Options_PredictiveSnippets", m_cache.editor.predictiveSnippets, enabled);
}
void SettingsManager::setStartupMode(StartupMode mode)
{
    writeCached("Options_StartupMode", m_cache.ui.startupMode, mode);
}
void SettingsManager::setCloseButtonMode(CloseButtonMode mode)
{
    writeCached("Options_CloseButtonMode", m_cache.ui.closeButtonMode, mode);
}
void SettingsManager::setTabDisplayMode(TabDisplayMode mode)
{
    writeCached("Options_TabDisplayMode", m_cache.ui.tabDisplayMode, mode);
}
void SettingsManager::setTabBarPosition(TabBarPosition position)
{
    writeCached("Options_TabBarPosition", m_cache.ui.tabBarPosition, position);
}
void SettingsManager::setShowMenuBar(bool visible)
{
    writeCached("Options_ShowMenuBar", m_cache.ui.showMenuBar, visible);
}
void SettingsManager::setShowToolbar(bool visible)
{
    writeCached("Options_ShowToolbar", m_cache.ui.showToolbar, visible);
}
void SettingsManager::setShowStatusbar(bool visible)
{
    writeCached("Options_ShowStatusbar", m_cache.ui.showStatusbar, visible);
}
void SettingsManager::setUiFontFamily(const QString& family)
{
    writeCached("Options_UIFont", m_cache.ui.uiFontFamily, family);
}
void SettingsManager::setUiFontSize(int size)
{
    writeCached("Options_UIFontSize", m_cache.ui.uiFontSize, size);
}
void SettingsManager::setTerminalPanelPosition(TerminalPanelPosition position)
{
    writeCached("Options_TerminalPanelPosition", m_cache.terminal.terminalPanelPosition, position);
}
void SettingsManager::setTerminalPanelMinWidth(int width)
{
    writeCached("Options_TerminalPanelMinWidth", m_cache.terminal.terminalPanelMinWidth, width);
}
void SettingsManager::setTerminalFontFamily(const QString& family)
{
    writeCached("Options_TerminalFont", m_cache.terminal.terminalFontFamily, family);
}
void SettingsManager::setTerminalFontSize(int size)
{
    writeCached("Options_TerminalFontSize", m_cache.terminal.terminalFontSize, size);
}
void SettingsManager::setShowTerminalPanel(bool visible)
{
    writeCached("Options_ShowTerminalPanel", m_cache.terminal.showTerminalPanel, visible);
}

void SettingsManager::setAutoSaveEnabled(bool enabled)
{
    writeCached("Options_AutoSaveEnabled", m_cache.autoSave.autoSaveEnabled, enabled);
}
void SettingsManager::setAutoSaveInterval(int seconds)
{
    writeCached("Options_AutoSaveInterval", m_cache.autoSave.autoSaveInterval, seconds);
}
void SettingsManager::setAutoSaveToOriginalFile(bool enabled)
{
    writeCached("Options_AutoSaveToOriginalFile", m_cache.autoSave.autoSaveToOriginalFile, enabled);
}
SettingsManager::ProjectSettings SettingsManager::projectSettings() const
{
    return m_cache.project;
}
bool SettingsManager::showHiddenFiles() const
{
    return m_cache.project.showHiddenFiles;
}
bool SettingsManager::useGitIgnore() const
{
    return m_cache.project.useGitIgnore;
}
ProjectPanelPosition SettingsManager::projectPanelPosition() const
{
    return m_cache.project.projectPanelPosition;
}
void SettingsManager::setShowHiddenFiles(bool visible)
{
    writeCached("Options_ShowHiddenFiles", m_cache.project.showHiddenFiles, visible);
}
void SettingsManager::setUseGitIgnore(bool enabled)
{
    writeCached("Options_UseGitIgnore", m_cache.project.useGitIgnore, enabled);
}
void SettingsManager::setProjectPanelPosition(ProjectPanelPosition position)
{
    writeCached("Options_ProjectPanelPosition", m_cache.project.projectPanelPosition, position);
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

int SettingsManager::externalToolCount() const
{
    return m_settings.value("ExternalTools/Count", 0).toInt();
}

QString SettingsManager::externalToolName(int index) const
{
    return m_settings.value(QString("ExternalTools/%1/Name").arg(index)).toString();
}

QString SettingsManager::externalToolCommand(int index) const
{
    return m_settings.value(QString("ExternalTools/%1/Command").arg(index)).toString();
}

QString SettingsManager::externalToolArguments(int index) const
{
    return m_settings.value(QString("ExternalTools/%1/Arguments").arg(index)).toString();
}

QString SettingsManager::externalToolWorkingDir(int index) const
{
    return m_settings.value(QString("ExternalTools/%1/WorkingDir").arg(index)).toString();
}

QString SettingsManager::externalToolShortcut(int index) const
{
    return m_settings.value(QString("ExternalTools/%1/Shortcut").arg(index)).toString();
}

bool SettingsManager::externalToolRunInTerminal(int index) const
{
    return m_settings.value(QString("ExternalTools/%1/RunInTerminal").arg(index), true).toBool();
}

void SettingsManager::setExternalTool(int index, const QString& name, const QString& command, const QString& arguments, const QString& workingDir,
                                      const QString& shortcut, bool runInTerminal)
{
    QString prefix = QString("ExternalTools/%1/").arg(index);
    m_settings.setValue(prefix + "Name", name);
    m_settings.setValue(prefix + "Command", command);
    m_settings.setValue(prefix + "Arguments", arguments);
    m_settings.setValue(prefix + "WorkingDir", workingDir);
    m_settings.setValue(prefix + "Shortcut", shortcut);
    m_settings.setValue(prefix + "RunInTerminal", runInTerminal);
}

void SettingsManager::addExternalTool(const QString& name, const QString& command, const QString& arguments, const QString& workingDir,
                                      const QString& shortcut, bool runInTerminal)
{
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

void SettingsManager::removeExternalTool(int index)
{
    int count = m_settings.value("ExternalTools/Count", 0).toInt();
    if (index < 0 || index >= count)
        return;

    QString prefix = QString("ExternalTools/%1/").arg(index);
    m_settings.remove(prefix + "Name");
    m_settings.remove(prefix + "Command");
    m_settings.remove(prefix + "Arguments");
    m_settings.remove(prefix + "WorkingDir");
    m_settings.remove(prefix + "Shortcut");
    m_settings.remove(prefix + "RunInTerminal");

    for (int i = index + 1; i < count; ++i)
    {
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

// ── LSP settings ───────────────────────────────────────────────

SettingsManager::LspSettings SettingsManager::lspSettings() const
{
    return m_cache.lsp;
}

bool SettingsManager::lspEnabled() const
{
    return m_cache.lsp.enabled;
}
bool SettingsManager::lspShowErrorList() const
{
    return m_cache.lsp.showErrorList;
}
int SettingsManager::lspCompletionTriggerChars() const
{
    return m_cache.lsp.completionTriggerChars;
}

QString SettingsManager::lspServerCommand(const QString& language) const
{
    auto it = m_cache.lsp.serverCommands.find(language);
    if (it != m_cache.lsp.serverCommands.end() && !it->isEmpty())
        return *it;
    return m_settings.value(QString("LSP/Server/%1/Command").arg(language)).toString();
}

QStringList SettingsManager::lspServerArgs(const QString& language) const
{
    auto it = m_cache.lsp.serverArgs.find(language);
    if (it != m_cache.lsp.serverArgs.end())
        return *it;
    return m_settings.value(QString("LSP/Server/%1/Args").arg(language)).toStringList();
}

void SettingsManager::setLspEnabled(bool enabled)
{
    writeCached("LSP/Enabled", m_cache.lsp.enabled, enabled);
}

void SettingsManager::setLspShowErrorList(bool visible)
{
    writeCached("LSP/ShowErrorList", m_cache.lsp.showErrorList, visible);
}

void SettingsManager::setLspCompletionTriggerChars(int chars)
{
    writeCached("LSP/CompletionTriggerChars", m_cache.lsp.completionTriggerChars, chars);
}

void SettingsManager::setLspServerCommand(const QString& language, const QString& command)
{
    m_cache.lsp.serverCommands[language] = command;
    m_settings.setValue(QString("LSP/Server/%1/Command").arg(language), command);
    settingsChanged();
}

void SettingsManager::setLspServerArgs(const QString& language, const QStringList& args)
{
    m_cache.lsp.serverArgs[language] = args;
    m_settings.setValue(QString("LSP/Server/%1/Args").arg(language), args);
    settingsChanged();
}

void SettingsManager::addRecentFile(const QString& filePath)
{
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
    return m_settings.value("recentFiles").toStringList();
}

void SettingsManager::clearRecentFiles()
{
    m_settings.remove("recentFiles");
}

void SettingsManager::addRecentFolder(const QString& folderPath)
{
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
    return m_settings.value("recentFolders").toStringList();
}

void SettingsManager::clearRecentFolders()
{
    m_settings.remove("recentFolders");
}
