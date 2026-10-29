/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * NextgenActions: QML bridge for Menu/Toolbar/StatusBar.
 * Persists showToolbar/showStatusbar via SettingsManager (shared with Widgets).
 * Exposes recent files, actions, encoding.
 */

#ifndef NEXTGENACTIONS_H
#define NEXTGENACTIONS_H

#include "theme.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringList>

class PrimoEditor;
class ExternalToolManager;
class QTemporaryDir;

class NextgenActions : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool showToolbar READ showToolbar WRITE setShowToolbar NOTIFY showToolbarChanged)
    Q_PROPERTY(bool showStatusbar READ showStatusbar WRITE setShowStatusbar NOTIFY showStatusbarChanged)
    Q_PROPERTY(bool showMenuBar READ showMenuBar WRITE setShowMenuBar NOTIFY showMenuBarChanged)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QStringList recentFolders READ recentFolders NOTIFY recentFilesChanged)
    Q_PROPERTY(QString currentEncoding READ currentEncoding NOTIFY currentEncodingChanged)
    Q_PROPERTY(QObject* editor READ editorObj WRITE setEditorObj NOTIFY editorChanged)
    Q_PROPERTY(QStringList themeNames READ themeNames NOTIFY themeChanged)
    Q_PROPERTY(QString currentThemeName READ currentThemeName NOTIFY themeChanged)
    // ── Theme/UI options exposed to QML Options dialog ──
    Q_PROPERTY(bool checkForUpdatesEnabled READ checkForUpdatesEnabled WRITE setCheckForUpdatesEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool wordWrap READ wordWrap WRITE setWordWrap NOTIFY settingsChanged)
    Q_PROPERTY(bool showLineNumbers READ showLineNumbers WRITE setShowLineNumbers NOTIFY settingsChanged)
    Q_PROPERTY(bool autoCloseBrackets READ autoCloseBrackets WRITE setAutoCloseBrackets NOTIFY settingsChanged)
    Q_PROPERTY(bool snippetsEnabled READ snippetsEnabled WRITE setSnippetsEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool predictiveSnippets READ predictiveSnippets WRITE setPredictiveSnippets NOTIFY settingsChanged)
    Q_PROPERTY(bool autoCompletionEnabled READ autoCompletionEnabled WRITE setAutoCompletionEnabled NOTIFY settingsChanged)
    Q_PROPERTY(int tabWidth READ tabWidth WRITE setTabWidth NOTIFY settingsChanged)
    Q_PROPERTY(int defaultFontSize READ defaultFontSize WRITE setDefaultFontSize NOTIFY settingsChanged)
    Q_PROPERTY(QString defaultFontFamily READ defaultFontFamily WRITE setDefaultFontFamily NOTIFY settingsChanged)
    Q_PROPERTY(bool showHiddenFiles READ showHiddenFiles WRITE setShowHiddenFiles NOTIFY settingsChanged)
    Q_PROPERTY(bool useGitIgnore READ useGitIgnore WRITE setUseGitIgnore NOTIFY settingsChanged)
    Q_PROPERTY(bool autoSaveEnabled READ autoSaveEnabled WRITE setAutoSaveEnabled NOTIFY settingsChanged)
    Q_PROPERTY(int autoSaveInterval READ autoSaveInterval WRITE setAutoSaveInterval NOTIFY settingsChanged)
    Q_PROPERTY(int externalToolCount READ externalToolCount NOTIFY externalToolsChanged)

public:
    explicit NextgenActions(QObject* parent = nullptr);
    explicit NextgenActions(PrimoEditor* editor, QObject* parent = nullptr);

    bool showToolbar() const;
    Q_INVOKABLE void setShowToolbar(bool v);
    bool showStatusbar() const;
    Q_INVOKABLE void setShowStatusbar(bool v);
    bool showMenuBar() const;
    Q_INVOKABLE void setShowMenuBar(bool v);

    QStringList recentFiles() const;
    QStringList recentFolders() const;
    QString currentEncoding() const;

    QStringList themeNames() const;
    QString currentThemeName() const;
    Q_INVOKABLE void setThemeByName(const QString& name);
    Q_INVOKABLE void setThemeById(int id);

    Q_INVOKABLE void refreshRecent();
    Q_INVOKABLE void clearRecentFiles();

    QObject* editorObj() const;
    void setEditorObj(QObject* obj);

    // ── Settings getters/setters for QML Options dialog ──
    Q_INVOKABLE bool checkForUpdatesEnabled() const;
    Q_INVOKABLE void setCheckForUpdatesEnabled(bool enabled);
    Q_INVOKABLE bool wordWrap() const;
    Q_INVOKABLE void setWordWrap(bool enabled);
    Q_INVOKABLE bool showLineNumbers() const;
    Q_INVOKABLE void setShowLineNumbers(bool visible);
    Q_INVOKABLE bool autoCloseBrackets() const;
    Q_INVOKABLE void setAutoCloseBrackets(bool enabled);
    Q_INVOKABLE bool snippetsEnabled() const;
    Q_INVOKABLE void setSnippetsEnabled(bool enabled);
    Q_INVOKABLE bool predictiveSnippets() const;
    Q_INVOKABLE void setPredictiveSnippets(bool enabled);
    Q_INVOKABLE bool autoCompletionEnabled() const;
    Q_INVOKABLE void setAutoCompletionEnabled(bool enabled);
    Q_INVOKABLE int tabWidth() const;
    Q_INVOKABLE void setTabWidth(int width);
    Q_INVOKABLE int defaultFontSize() const;
    Q_INVOKABLE void setDefaultFontSize(int size);
    Q_INVOKABLE QString defaultFontFamily() const;
    Q_INVOKABLE void setDefaultFontFamily(const QString& family);
    Q_INVOKABLE bool showHiddenFiles() const;
    Q_INVOKABLE void setShowHiddenFiles(bool visible);
    Q_INVOKABLE bool useGitIgnore() const;
    Q_INVOKABLE void setUseGitIgnore(bool enabled);
    Q_INVOKABLE bool autoSaveEnabled() const;
    Q_INVOKABLE void setAutoSaveEnabled(bool enabled);
    Q_INVOKABLE int autoSaveInterval() const;
    Q_INVOKABLE void setAutoSaveInterval(int seconds);

    // ── External tools (QML model) ──
    Q_INVOKABLE int externalToolCount() const;
    Q_INVOKABLE QString externalToolName(int index) const;
    Q_INVOKABLE QString externalToolCommand(int index) const;
    Q_INVOKABLE QString externalToolArguments(int index) const;
    Q_INVOKABLE QString externalToolWorkingDir(int index) const;
    Q_INVOKABLE QString externalToolShortcut(int index) const;
    Q_INVOKABLE bool externalToolRunInTerminal(int index) const;
    Q_INVOKABLE void addExternalTool(const QString& name, const QString& command, const QString& arguments,
                                     const QString& workingDir, const QString& shortcut, bool runInTerminal);
    Q_INVOKABLE void removeExternalTool(int index);
    Q_INVOKABLE void runExternalTool(int index, const QString& filePath, const QString& projectDir,
                                     const QString& selectedText, int lineNumber);

    // ── Remote file support ──
    Q_INVOKABLE void openRemoteFile(const QString& url);
    Q_INVOKABLE bool isRemoteLoading() const;
    Q_INVOKABLE QString remoteLoadError() const;

public slots:
    void newFile();
    void openFile(const QString& path = QString());
    void openFileDialog();
    void openFolderDialog();
    void openRemoteDialog();
    void openRemoteFolderDialog();
    void saveFile();
    void saveFileAs();
    void saveFileAsDialog();
    void closeFile();
    void exitApp();
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void find();
    void replace();
    void findNext();
    void findPrevious();
    void goToLine();
    void toggleBookmark();
    void nextBookmark();
    void prevBookmark();
    void clearBookmarks();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void toggleFullScreen();
    void toggleWordWrap();
    void showAbout();
    void showOptions();
    void reopenWithEncoding(const QString& enc);
    void saveWithEncoding(const QString& enc);
    void openRecentFile(const QString& path);
    void toggleTerminal();
    void findInFiles(const QString& pattern = QString());
    QString runCommand(const QString& rawCmd);

signals:
    void showToolbarChanged();
    void showStatusbarChanged();
    void showMenuBarChanged();
    void recentFilesChanged();
    void currentEncodingChanged();
    void editorChanged();
    void themeChanged();
    void settingsChanged();
    void externalToolsChanged();
    void requestOpenFileDialog();
    void requestOpenFolderDialog();
    void requestOpenRemoteDialog();
    void requestOpenRemoteFolderDialog();
    void requestSaveAsDialog();
    void requestGoToLineDialog();
    void requestFindDialog();
    void requestReplaceDialog();
    void requestAboutDialog();
    void requestOptionsDialog();
    void requestTerminalToggle();
    void requestFindInFiles();
    void showMessage(const QString& msg);
    // Tab navigation (requested by command mode)
    void requestTabNext();
    void requestTabPrev();
    void requestTabFirst();
    void requestTabLast();
    void requestTabClose();
    // LSP server control (requested by command mode)
    void requestLspStart(const QString& command, const QStringList& args, const QString& rootUri);
    void requestLspStop();
    // Remote file signals
    void remoteLoadStarted(const QString& url);
    void remoteLoadProgress(const QString& url, qint64 bytesReceived, qint64 bytesTotal);
    void remoteLoadFinished(const QString& url, const QString& localPath, bool success, const QString& error);

private:
    PrimoEditor* m_editor = nullptr;
    ExternalToolManager* m_externalToolManager = nullptr;
    QNetworkAccessManager* m_networkManager = nullptr;
    QNetworkReply* m_currentReply = nullptr;
    bool m_remoteLoading = false;
    QString m_remoteLoadError;

    void onRemoteDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onRemoteDownloadFinished(const QString& tempPath, const QString& url);
    void onRemoteDownloadError(QNetworkReply::NetworkError code);
};

#endif // NEXTGENACTIONS_H
