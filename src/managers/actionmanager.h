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
#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include <QObject>
#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>

class CodeEditor;
class ProjectPanel;
class TerminalPanel;

class ActionManager : public QObject {
    Q_OBJECT

public:
    explicit ActionManager(QObject *parent = nullptr);

    void createActions();
    void buildMenus(QMenuBar *menuBar);
    QToolBar* buildToolBar();
    void buildStatusBar(QStatusBar *statusBar, ProjectPanel *projectPanel, TerminalPanel *terminalPanel);

    QAction* newAct() const { return m_newAct; }
    QAction* newWindowAct() const { return m_newWindowAct; }
    QAction* openAct() const { return m_openAct; }
    QAction* openRemoteAct() const { return m_openRemoteAct; }
    QAction* openFolderAct() const { return m_openFolderAct; }
    QAction* revertAct() const { return m_revertAct; }
    QAction* saveAct() const { return m_saveAct; }
    QAction* saveAsAct() const { return m_saveAsAct; }
    QAction* saveAllAct() const { return m_saveAllAct; }
    QAction* closeAct() const { return m_closeAct; }
    QAction* closeAllAct() const { return m_closeAllAct; }
    QAction* exitAct() const { return m_exitAct; }

    QAction* undoAct() const { return m_undoAct; }
    QAction* redoAct() const { return m_redoAct; }
    QAction* cutAct() const { return m_cutAct; }
    QAction* copyAct() const { return m_copyAct; }
    QAction* pasteAct() const { return m_pasteAct; }
    QAction* selectAllAct() const { return m_selectAllAct; }
    QAction* deleteAct() const { return m_deleteAct; }
    QAction* findAct() const { return m_findAct; }
    QAction* replaceAct() const { return m_replaceAct; }
    QAction* findInFilesAct() const { return m_findInFilesAct; }
    QAction* goToAct() const { return m_goToAct; }
    QAction* findNextAct() const { return m_findNextAct; }
    QAction* findPrevAct() const { return m_findPrevAct; }

    QAction* zoomInAct() const { return m_zoomInAct; }
    QAction* zoomOutAct() const { return m_zoomOutAct; }
    QAction* zoomResetAct() const { return m_zoomResetAct; }
    QAction* fullScreenAct() const { return m_fullScreenAct; }
    QAction* projectPanelAct() const { return m_projectPanelAct; }
    QAction* terminalPanelAct() const { return m_terminalPanelAct; }
    QAction* toolBarAct() const { return m_toolBarAct; }
    QAction* statusBarAct() const { return m_statusBarAct; }
    QAction* wordWrapAct() const { return m_wordWrapAct; }
    QAction* printAct() const { return m_printAct; }
    QAction* printPreviewAct() const { return m_printPreviewAct; }
    QAction* readOnlyAct() const { return m_readOnlyAct; }
    QAction* toggleBookmarkAct() const { return m_toggleBookmarkAct; }
    QAction* nextBookmarkAct() const { return m_nextBookmarkAct; }
    QAction* prevBookmarkAct() const { return m_prevBookmarkAct; }
    QAction* clearBookmarksAct() const { return m_clearBookmarksAct; }
    QAction* optionsAct() const { return m_optionsAct; }
    QAction* aboutAct() const { return m_aboutAct; }

    QMenu* recentFilesMenu() const { return m_recentFilesMenu; }
    QAction* recentFileAct(int index) const { return (index >= 0 && index < 10) ? m_recentFileActs[index] : nullptr; }
    QAction* clearRecentFilesAct() const { return m_clearRecentFilesAct; }
    QMenu* reopenEncodingMenu() const { return m_reopenEncodingMenu; }
    QMenu* saveEncodingMenu() const { return m_saveEncodingMenu; }

    QToolBar* toolBar() const { return m_toolBar; }
    QLabel* lineColLabel() const { return m_lineColLabel; }
    QComboBox* encodingComboBox() const { return m_encodingComboBox; }
    QLabel* fileTypeLabel() const { return m_fileTypeLabel; }
    QToolButton* projectPanelButton() const { return m_projectPanelButton; }
    QToolButton* terminalPanelButton() const { return m_terminalPanelButton; }

signals:
    void newFileTriggered();
    void newWindowTriggered();
    void openFileTriggered();
    void openRemoteTriggered();
    void openFolderTriggered();
    void saveFileTriggered();
    void saveFileAsTriggered();
    void saveAllTriggered();
    void revertTriggered();
    void closeCurrentTabTriggered();
    void closeAllTabsTriggered();
    void exitTriggered();

    void undoTriggered();
    void redoTriggered();
    void cutTriggered();
    void copyTriggered();
    void pasteTriggered();
    void selectAllTriggered();
    void deleteTriggered();
    void findTriggered();
    void replaceTriggered();
    void findInFilesTriggered();
    void goToLineTriggered();
    void findNextTriggered();
    void findPreviousTriggered();

    void zoomInTriggered();
    void zoomOutTriggered();
    void zoomResetTriggered();
    void toggleFullScreenTriggered();
    void toggleProjectPanelTriggered();
    void toggleTerminalPanelTriggered();
    void toggleToolBarTriggered();
    void toggleStatusBarTriggered();
    void toggleWordWrapTriggered();
    void printFileTriggered();
    void printPreviewTriggered();
    void toggleReadOnlyTriggered();
    void toggleBookmarkTriggered();
    void nextBookmarkTriggered();
    void prevBookmarkTriggered();
    void clearBookmarksTriggered();
    void optionsTriggered();
    void aboutTriggered();

    void openRecentFileTriggered(const QString &filePath);
    void clearRecentFilesTriggered();

private:
    template<typename Functor>
    QAction* createIconAction(const QString &iconPath, const QString &text, const QKeySequence &shortcut, Functor slot) {
        QIcon icon(iconPath);
        QAction *action = new QAction(icon, text, this);
        action->setShortcut(shortcut);
        connect(action, &QAction::triggered, this, slot);
        return action;
    }

    void buildFileMenu(QMenu *fileMenu);
    void buildEditMenu(QMenu *editMenu);
    void buildViewMenu(QMenu *viewMenu);
    void buildToolsMenu(QMenu *toolsMenu);
    void buildHelpMenu(QMenu *helpMenu);

    QAction* m_newAct = nullptr;
    QAction* m_newWindowAct = nullptr;
    QAction* m_openAct = nullptr;
    QAction* m_openRemoteAct = nullptr;
    QAction* m_openFolderAct = nullptr;
    QAction* m_revertAct = nullptr;
    QAction* m_saveAct = nullptr;
    QAction* m_saveAsAct = nullptr;
    QAction* m_saveAllAct = nullptr;
    QAction* m_closeAct = nullptr;
    QAction* m_closeAllAct = nullptr;
    QAction* m_exitAct = nullptr;

    QAction* m_undoAct = nullptr;
    QAction* m_redoAct = nullptr;
    QAction* m_cutAct = nullptr;
    QAction* m_copyAct = nullptr;
    QAction* m_pasteAct = nullptr;
    QAction* m_selectAllAct = nullptr;
    QAction* m_deleteAct = nullptr;
    QAction* m_findAct = nullptr;
    QAction* m_replaceAct = nullptr;
    QAction* m_findInFilesAct = nullptr;
    QAction* m_goToAct = nullptr;
    QAction* m_findNextAct = nullptr;
    QAction* m_findPrevAct = nullptr;

    QAction* m_zoomInAct = nullptr;
    QAction* m_zoomOutAct = nullptr;
    QAction* m_zoomResetAct = nullptr;
    QAction* m_fullScreenAct = nullptr;
    QAction* m_projectPanelAct = nullptr;
    QAction* m_terminalPanelAct = nullptr;
    QAction* m_toolBarAct = nullptr;
    QAction* m_statusBarAct = nullptr;
    QAction* m_wordWrapAct = nullptr;
    QAction* m_printAct = nullptr;
    QAction* m_printPreviewAct = nullptr;
    QAction* m_readOnlyAct = nullptr;
    QAction* m_toggleBookmarkAct = nullptr;
    QAction* m_nextBookmarkAct = nullptr;
    QAction* m_prevBookmarkAct = nullptr;
    QAction* m_clearBookmarksAct = nullptr;
    QAction* m_optionsAct = nullptr;
    QAction* m_aboutAct = nullptr;

    QMenu* m_recentFilesMenu = nullptr;
    QAction* m_recentFileActs[10] = {};
    QAction* m_clearRecentFilesAct = nullptr;
    QMenu* m_reopenEncodingMenu = nullptr;
    QMenu* m_saveEncodingMenu = nullptr;

    QToolBar* m_toolBar = nullptr;
    QLabel* m_lineColLabel = nullptr;
    QComboBox* m_encodingComboBox = nullptr;
    QLabel* m_fileTypeLabel = nullptr;
    QToolButton* m_projectPanelButton = nullptr;
    QToolButton* m_terminalPanelButton = nullptr;
};

#endif
