#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "appstrings.h"

#include <QLocalServer>
#include <QMainWindow>
#include <QPointer>

class ActionManager;
class CodeEditor;
class ErrorListPanel;
class FindSymbolsDialog;
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

namespace lsp {
class LspServerManager;
} // namespace lsp

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

public slots:
    void newWindow();
    void openFile();
    void openRemote();
    void openFolder();
    void closeProject();
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
    void toggleErrorListPanel();
    void findSymbols();
    void expandSelection();
    void shrinkSelection();
    void showAbout();
    void showOptions();
    void runExternalTool(int index);
    void goToDefinition();
    void goToTypeDefinition();
    void goToDeclaration();
    void formatSelection();
    void findReferences();
    void triggerCompletion();
    void renameSymbol();
    void printFile();
    void printPreview();
    void pageSetup();
    void quitDevpad();

private slots:
    void updateStatusBar();
    void updateFileTypeLabel();
    void onTabChanged(int index);
    void onEditorCreated(CodeEditor* editor);
    void openFileFromProject(const QString& filePath);
    void onFileModifiedExternally(const QString& filePath);
    void onNavigateToLocation(const QString& filePath, int line, int column);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void setupUI();
    void wireActions();
    void connectPanelSignals();
    void setupEditorConnections();
    void setupLspConnections();
    void applyCloseButtonPosition();
    void applyTabBarPosition();
    void applyTerminalPanelPosition();
    void handleQuitRequest();
    bool signalOtherInstancesToQuit();
    bool saveAllModifiedTabs();
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

    // Owned: child widgets (parent=this), destroyed by Qt parent mechanism
    QTabWidget* m_tabWidget;
    SplitView* m_splitView;
    ProjectPanel* m_projectPanel;
    TerminalPanel* m_terminalPanel;

    // Owned: QObject children (parent=this), destroyed automatically
    EditorController* m_editorController;
    ActionManager* m_actionManager;
    SessionManager* m_sessionManager;
    EncodingManager* m_encodingManager;
    ThemeApplier* m_themeApplier;
    PrintManager* m_printManager;

    // Owned: QObject children (parent=this), destroyed automatically
    FileManager* m_fileManager;
    TabManager* m_tabManager;
    SearchManager* m_searchManager;
    FileWatcherManager* m_fileWatcherManager;
    QTimer* m_autoSaveTimer;

    // QPointer: auto-nulls when dialog is destroyed externally
    QPointer<FindInFilesDialog> m_findInFilesDialog;

    // Owned: QObject children (parent=this), destroyed automatically
    ExternalToolManager* m_externalToolManager;
    RemoteFileService* m_remoteFileService;
    SnippetManager* m_snippetManager;
    lsp::LspServerManager* m_lspServerManager;
    ErrorListPanel* m_errorListPanel;
    QPointer<FindSymbolsDialog> m_findSymbolsDialog;

    QLocalServer* m_localServer = nullptr;
    bool m_quitRequested = false;
};

#endif // MAINWINDOW_H
