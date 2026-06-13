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

#include <QMainWindow>
#include <QPointer>
#include <QTabWidget>
#include <QTimer>

class CodeEditor;
class EditorController;
class FindInFilesDialog;
class AboutDialog;
class OptionsDialog;
class FileManager;
class TabManager;
class SearchManager;
class ProjectPanel;
class TerminalPanel;
class ActionManager;
class SessionManager;
class EncodingManager;
class ThemeApplier;
class PrintManager;
class FileWatcherManager;
class SplitView;
class ExternalToolManager;
class RemoteFileService;

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
};

#endif
