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

namespace lsp
{
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
    QTabWidget* m_tabWidget = nullptr;
    SplitView* m_splitView = nullptr;
    ProjectPanel* m_projectPanel = nullptr;
    TerminalPanel* m_terminalPanel = nullptr;

    // Owned: QObject children (parent=this), destroyed automatically
    EditorController* m_editorController = nullptr;
    ActionManager* m_actionManager = nullptr;
    SessionManager* m_sessionManager = nullptr;
    EncodingManager* m_encodingManager = nullptr;
    ThemeApplier* m_themeApplier = nullptr;
    PrintManager* m_printManager = nullptr;

    // Owned: QObject children (parent=this), destroyed automatically
    FileManager* m_fileManager = nullptr;
    TabManager* m_tabManager = nullptr;
    SearchManager* m_searchManager = nullptr;
    FileWatcherManager* m_fileWatcherManager = nullptr;
    QTimer* m_autoSaveTimer = nullptr;

    // QPointer: auto-nulls when dialog is destroyed externally
    QPointer<FindInFilesDialog> m_findInFilesDialog;

    // Owned: QObject children (parent=this), destroyed automatically
    ExternalToolManager* m_externalToolManager = nullptr;
    RemoteFileService* m_remoteFileService = nullptr;
    SnippetManager* m_snippetManager = nullptr;
    lsp::LspServerManager* m_lspServerManager = nullptr;
    ErrorListPanel* m_errorListPanel = nullptr;
    QPointer<FindSymbolsDialog> m_findSymbolsDialog;

    QLocalServer* m_localServer = nullptr;
    bool m_quitRequested = false;
};

#endif // MAINWINDOW_H
