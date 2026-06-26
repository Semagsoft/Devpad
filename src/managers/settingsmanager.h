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
#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include "theme.h"

#include <QFont>
#include <QHash>
#include <QObject>
#include <QSettings>
#include <QString>
#include <memory>
#include <type_traits>

class CodeEditor;

enum class StartupMode
{
    NewFile = 0,
    RestoreSession = 1,
    OpenLastFile = 2,
    Empty = 3
};
enum class CloseButtonMode
{
    Right = 0,
    Left = 1
};
enum class TabDisplayMode
{
    AlwaysShow = 0,
    ShowTwoPlus = 1,
    NeverShow = 2
};
enum class CursorStyle
{
    Line = 0,
    Block = 1,
    Underline = 2
};
enum class TabBarPosition
{
    Top = 0,
    Bottom = 1
};
enum class TerminalPanelPosition
{
    Bottom = 0,
    Right = 1,
    Tab = 2
};
enum class ProjectPanelPosition
{
    Left = 0,
    Right = 1
};

class SettingsManager : public QObject
{
    Q_OBJECT

public:
    static SettingsManager& instance();
    static void setTestingInstance(SettingsManager* instance);
    static std::unique_ptr<SettingsManager> createForTesting();

    // ── Settings cache ─────────────────────────────────────────
    struct EditorSettings
    {
        QString defaultFontFamily = "Monospace";
        int defaultFontSize = 12;
        bool showLineNumbers = true;
        bool scrollPastContent = false;
        bool codeCollapsing = false;
        bool wordWrap = false;
        ThemeId theme = ThemeId::Light;
        int defaultEncoding = 0;
        int defaultFormat = 0;
        bool showWhitespace = false;
        bool autoCompletionEnabled = true;
        int autoCompletionThreshold = 2;
        bool autoCompletionCaseSensitive = false;
        bool autoCloseBrackets = true;
        int tabWidth = 4;
        CursorStyle cursorStyle = CursorStyle::Line;
        bool cursorBlinking = true;
        bool highlightCurrentLine = true;
        bool verticalEdgeEnabled = false;
        int verticalEdgeColumn = 80;
        bool snippetsEnabled = true;
        bool predictiveSnippets = true;
    };

    struct UISettings
    {
        StartupMode startupMode = StartupMode::NewFile;
        CloseButtonMode closeButtonMode = CloseButtonMode::Right;
        TabDisplayMode tabDisplayMode = TabDisplayMode::ShowTwoPlus;
        TabBarPosition tabBarPosition = TabBarPosition::Top;
        bool showMenuBar = true;
        bool showToolbar = true;
        bool showStatusbar = true;
        QString uiFontFamily = "Sans Serif";
        int uiFontSize = 10;
    };

    struct TerminalSettings
    {
        TerminalPanelPosition terminalPanelPosition = TerminalPanelPosition::Bottom;
        int terminalPanelMinWidth = 350;
        QString terminalFontFamily = "Monospace";
        int terminalFontSize = 12;
        bool showTerminalPanel = false;
    };

    struct AutoSaveSettings
    {
        bool autoSaveEnabled = false;
        int autoSaveInterval = 60;
        bool autoSaveToOriginalFile = true;
    };

    struct ProjectSettings
    {
        bool showHiddenFiles = false;
        ProjectPanelPosition projectPanelPosition = ProjectPanelPosition::Left;
    };

    // ── LSP settings ───────────────────────────────────────────
    struct LspSettings
    {
        bool enabled = true;
        bool showErrorList = true;
        int completionTriggerChars = 2;
        QHash<QString, QString> serverCommands;
        QHash<QString, QStringList> serverArgs;
    };

    LspSettings lspSettings() const;
    bool lspEnabled() const;
    bool lspShowErrorList() const;
    int lspCompletionTriggerChars() const;
    QString lspServerCommand(const QString& language) const;
    QStringList lspServerArgs(const QString& language) const;
    void setLspEnabled(bool enabled);
    void setLspShowErrorList(bool visible);
    void setLspCompletionTriggerChars(int chars);
    void setLspServerCommand(const QString& language, const QString& command);
    void setLspServerArgs(const QString& language, const QStringList& args);

    // ── Editor settings ────────────────────────────────────────
    EditorSettings editorSettings() const;
    QString defaultFontFamily() const;
    int defaultFontSize() const;
    QFont defaultFont() const;
    bool showLineNumbers() const;
    bool scrollPastContent() const;
    bool codeCollapsing() const;
    bool wordWrap() const;
    ThemeId theme() const;
    bool isDarkTheme() const;
    ThemeColors currentThemeColors() const;
    QColor accentColor() const;
    bool hasAccentColor() const;
    void setAccentColor(const QColor &color);
    void clearAccentColor();
    int defaultEncoding() const;
    int defaultFormat() const;
    bool showWhitespace() const;
    bool autoCompletionEnabled() const;
    int autoCompletionThreshold() const;
    bool autoCompletionCaseSensitive() const;
    bool autoCloseBrackets() const;
    int tabWidth() const;
    CursorStyle cursorStyle() const;
    bool cursorBlinking() const;
    bool highlightCurrentLine() const;
    bool verticalEdgeEnabled() const;
    int verticalEdgeColumn() const;
    void setDefaultFontFamily(const QString& family);
    void setDefaultFontSize(int size);
    void setShowLineNumbers(bool visible);
    void setScrollPastContent(bool enabled);
    void setCodeCollapsing(bool enabled);
    void setWordWrap(bool enabled);
    void setTheme(ThemeId theme);
    void setDefaultEncoding(int encoding);
    void setDefaultFormat(int format);
    void setShowWhitespace(bool visible);
    void setAutoCompletionEnabled(bool enabled);
    void setAutoCompletionThreshold(int threshold);
    void setAutoCompletionCaseSensitive(bool sensitive);
    void setAutoCloseBrackets(bool enabled);
    void setTabWidth(int width);
    void setCursorStyle(CursorStyle style);
    bool snippetsEnabled() const;
    bool predictiveSnippets() const;
    void setSnippetsEnabled(bool enabled);
    void setPredictiveSnippets(bool enabled);
    void setCursorBlinking(bool enabled);
    void setHighlightCurrentLine(bool enabled);
    void setVerticalEdgeEnabled(bool enabled);
    void setVerticalEdgeColumn(int column);
    void applyToEditor(CodeEditor* editor) const;

    // ── UI settings ────────────────────────────────────────────
    UISettings uiSettings() const;
    StartupMode startupMode() const;
    CloseButtonMode closeButtonMode() const;
    TabDisplayMode tabDisplayMode() const;
    TabBarPosition tabBarPosition() const;
    bool showMenuBar() const;
    bool showToolbar() const;
    bool showStatusbar() const;
    QString uiFontFamily() const;
    int uiFontSize() const;
    QFont uiFont() const;
    void setStartupMode(StartupMode mode);
    void setCloseButtonMode(CloseButtonMode mode);
    void setTabDisplayMode(TabDisplayMode mode);
    void setTabBarPosition(TabBarPosition position);
    void setShowMenuBar(bool visible);
    void setShowToolbar(bool visible);
    void setShowStatusbar(bool visible);
    void setUiFontFamily(const QString& family);
    void setUiFontSize(int size);

    // ── Terminal settings ──────────────────────────────────────
    TerminalSettings terminalSettings() const;
    TerminalPanelPosition terminalPanelPosition() const;
    int terminalPanelMinWidth() const;
    QString terminalFontFamily() const;
    int terminalFontSize() const;
    QFont terminalFont() const;
    bool showTerminalPanel() const;
    void setTerminalPanelPosition(TerminalPanelPosition position);
    void setTerminalPanelMinWidth(int width);
    void setTerminalFontFamily(const QString& family);
    void setTerminalFontSize(int size);
    void setShowTerminalPanel(bool visible);

    // ── Auto-save settings ─────────────────────────────────────
    AutoSaveSettings autoSaveSettings() const;
    bool autoSaveEnabled() const;
    int autoSaveInterval() const;
    bool autoSaveToOriginalFile() const;
    void setAutoSaveEnabled(bool enabled);
    void setAutoSaveInterval(int seconds);
    void setAutoSaveToOriginalFile(bool enabled);

    // ── Project settings ───────────────────────────────────────
    ProjectSettings projectSettings() const;
    bool showHiddenFiles() const;
    ProjectPanelPosition projectPanelPosition() const;
    void setShowHiddenFiles(bool visible);
    void setProjectPanelPosition(ProjectPanelPosition position);

    // ── File type associations ─────────────────────────────────
    QString syntaxForExtension(const QString& ext) const;
    QString syntaxForFile(const QString& filePath) const;

    // ── External tools ─────────────────────────────────────────
    int externalToolCount() const;
    QString externalToolName(int index) const;
    QString externalToolCommand(int index) const;
    QString externalToolArguments(int index) const;
    QString externalToolWorkingDir(int index) const;
    QString externalToolShortcut(int index) const;
    bool externalToolRunInTerminal(int index) const;
    void setExternalTool(int index, const QString& name, const QString& command,
                         const QString& arguments, const QString& workingDir,
                         const QString& shortcut, bool runInTerminal);
    void addExternalTool(const QString& name, const QString& command,
                         const QString& arguments, const QString& workingDir,
                         const QString& shortcut, bool runInTerminal);
    void removeExternalTool(int index);

    // ── Recent files / folders ─────────────────────────────────
    void addRecentFile(const QString& filePath);
    QStringList recentFiles() const;
    void clearRecentFiles();
    void addRecentFolder(const QString& folderPath);
    QStringList recentFolders() const;
    void clearRecentFolders();

signals:
    void settingsChanged();

private:
    SettingsManager();
    Q_DISABLE_COPY(SettingsManager)

public:
    ~SettingsManager() = default;

private:

    static constexpr int CurrentSettingsVersion = 1;
    static const QHash<QString, QString>& extensionMap();
    static const QHash<QString, QString>& fileNameMap();
    static SettingsManager* s_testInstance;

    friend struct std::default_delete<SettingsManager>;
    void loadCache();
    void ensureSettingsVersion();
    
    struct Cache
    {
        EditorSettings editor;
        UISettings ui;
        TerminalSettings terminal;
        AutoSaveSettings autoSave;
        ProjectSettings project;
        LspSettings lsp;
        QColor accentColor;
        bool hasAccentColor = false;
    };

    template<typename T>
    void writeCached(const char* key, T& cacheField, T value)
    {
        if constexpr (std::is_enum_v<T>)
            m_settings.setValue(key, static_cast<int>(value));
        else
            m_settings.setValue(key, value);
        cacheField = value;
        settingsChanged();
    }

    Cache m_cache;
    QSettings m_settings;
};

#endif
