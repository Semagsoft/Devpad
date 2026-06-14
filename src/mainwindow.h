#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "appstrings.h"

#include <QMainWindow>
#include <QPointer>

class ActionManager;
class CodeEditor;
class EditorController;
class EncodingManager;
class ExternalToolManager;
class FileManager;
class FileWatcherManager;
class FindInFilesDialog;
class PrintManager;
class ProjectPanel;
class RemoteFileService;
class SearchManager;
class SessionManager;
class SnippetManager;
class SplitView;
class TabManager;
class TerminalPanel;
class ThemeApplier;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void openFileFromPath(const QString& filePath);
    void openTransferFile(const QString& filePath);
    void openFolderFromPath(const QString& folderPath);
    void openRemoteFileFromData(const QString& urlStr, const QString& fileName, const QByteArray& data);

private slots:
    void newWindow();
    void openFile();
    void openRemote();
    void openFolder();
    void revertFile();

    void find();
    void replace();
    void findInFiles();
    void goToLine();
    void findNext();
    void findPrevious();

    void toggleFullScreen();
    void toggleProjectPanel();
    void toggleTerminalPanel();
    void showAbout();

    void showOptions();
    void runExternalTool(int index);
    void printFile();
    void printPreview();

    void updateStatusBar();
    void updateFileTypeLabel();
    void onTabChanged(int index);
    void onEditorCreated(CodeEditor* editor);
    void openFileFromProject(const QString& filePath);
    void onFileModifiedExternally(const QString& filePath);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void setupUI();
    void connectActions();
    void connectPanelSignals();
    void applyCloseButtonPosition();
    void applyTabBarPosition();
    void applyTerminalPanelPosition();
    void applySettings();
    void applyStartupMode();
    void applyAutoSaveSettings();

    void loadFile(const QString& fileName, const QString& encoding = QString());
    void updateSplitViewVisibility();
    QString getSyntaxForFile(const QString& displayName) const;
    void updateStatusBarLabelsVisibility();
    void updateRecentFileActions();
    void updateEncodingSelector();
    void reloadCurrentFileWithEncoding(const QString& encoding);
    void saveCurrentFileWithEncoding(const QString& encoding);

    QTabWidget* tabWidget;
    SplitView* splitView;
    ProjectPanel* projectPanel;
    TerminalPanel* terminalPanel;

    EditorController* editorController;
    ActionManager* actionManager;
    SessionManager* sessionManager;
    EncodingManager* encodingManager;
    ThemeApplier* themeApplier;
    PrintManager* printManager;

    FileManager* fileManager;
    TabManager* tabManager;
    SearchManager* searchManager;
    FileWatcherManager* fileWatcherManager;
    QTimer* autoSaveTimer;

    QPointer<FindInFilesDialog> findInFilesDialog;
    ExternalToolManager* m_externalToolManager;
    RemoteFileService* m_remoteFileService;
    SnippetManager* m_snippetManager;
};

#endif // MAINWINDOW_H
