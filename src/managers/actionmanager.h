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

#include "encodingmenuhelper.h"
#include "recentfileshelper.h"

#include <QObject>
#include <cstdint>
#include <functional>

class CodeEditor;
class EditorController;
class EncodingManager;
class MainWindow;
class ProjectPanel;
class SplitView;
class TerminalPanel;

class QAction;
class QComboBox;
class QLabel;
class QMenu;
class QMenuBar;
class QStatusBar;
class QTabWidget;
class QToolBar;
class QToolButton;

struct ActionTargets
{
    EditorController* editorController = nullptr;
    TerminalPanel* terminalPanel = nullptr;
    SplitView* splitView = nullptr;
    EncodingManager* encodingManager = nullptr;
    MainWindow* mainWindow = nullptr;
    std::function<void(const QString&)> openRecentFile;
    std::function<void()> clearRecentFiles;
    std::function<void()> updateRecentFileActions;
    std::function<void(const QString&, bool)> encodingSelected;
    std::function<void(int, QTabWidget*)> tabPinToggled;
};

class ActionManager : public QObject
{
    Q_OBJECT

public:
    enum class FileAction : std::uint8_t
    {
        New,
        NewWindow,
        Open,
        OpenRemote,
        OpenFolder,
        CloseProject,
        Save,
        SaveAs,
        SaveAll,
        Revert,
        CloseTab,
        CloseAllTabs,
        Exit,
        Quit
    };
    enum class EditAction : std::uint8_t
    {
        Undo,
        Redo,
        Cut,
        Copy,
        Paste,
        SelectAll,
        Delete,
        Find,
        Replace,
        FindInFiles,
        GoToLine,
        FindNext,
        FindPrevious,
        ToggleComment,
        FormatSelection,
        ExpandSelection,
        ShrinkSelection
    };
    enum class ViewAction : std::uint8_t
    {
        ZoomIn,
        ZoomOut,
        ZoomReset,
        FullScreen,
        ProjectPanel,
        TerminalPanel,
        ToolBar,
        StatusBar,
        MenuBar,
        WordWrap
    };
    enum class DocumentAction : std::uint8_t
    {
        Print,
        PrintPreview,
        PageSetup,
        ReadOnly,
        ToggleBookmark,
        NextBookmark,
        PrevBookmark,
        ClearBookmarks,
        InsertSnippet
    };
    enum class LspAction : std::uint8_t
    {
        GoToDefinition,
        GoToTypeDefinition,
        GoToDeclaration,
        FindReferences,
        TriggerCompletion,
        RenameSymbol
    };
    enum class HelpAction : std::uint8_t
    {
        Options,
        ConfigureExternalTools,
        About,
        Donate,
        Website
    };

    explicit ActionManager(QObject* parent = nullptr);

    void createActions();
    void buildMenus(QMenuBar* menuBar);
    QToolBar* buildToolBar();
    void buildStatusBar(QStatusBar* statusBar, ProjectPanel* projectPanel, TerminalPanel* terminalPanel);

    QAction* newAct() const
    {
        return m_newAct;
    }
    QAction* quitDevpadAct() const
    {
        return m_quitDevpadAct;
    }
    QAction* undoAct() const
    {
        return m_undoAct;
    }

    QAction* redoAct() const
    {
        return m_redoAct;
    }
    QAction* cutAct() const
    {
        return m_cutAct;
    }
    QAction* copyAct() const
    {
        return m_copyAct;
    }
    QAction* pasteAct() const
    {
        return m_pasteAct;
    }
    QAction* selectAllAct() const
    {
        return m_selectAllAct;
    }
    QAction* deleteAct() const
    {
        return m_deleteAct;
    }
    QAction* findAct() const
    {
        return m_findAct;
    }
    QAction* replaceAct() const
    {
        return m_replaceAct;
    }
    QAction* findInFilesAct() const
    {
        return m_findInFilesAct;
    }
    QAction* goToAct() const
    {
        return m_goToAct;
    }
    QAction* findNextAct() const
    {
        return m_findNextAct;
    }
    QAction* findPrevAct() const
    {
        return m_findPrevAct;
    }
    QAction* zoomInAct() const
    {
        return m_zoomInAct;
    }
    QAction* zoomOutAct() const
    {
        return m_zoomOutAct;
    }
    QAction* zoomResetAct() const
    {
        return m_zoomResetAct;
    }
    QAction* fullScreenAct() const
    {
        return m_fullScreenAct;
    }
    QAction* projectPanelAct() const
    {
        return m_projectPanelAct;
    }
    QAction* terminalPanelAct() const
    {
        return m_terminalPanelAct;
    }
    QAction* toolBarAct() const
    {
        return m_toolBarAct;
    }
    QAction* statusBarAct() const
    {
        return m_statusBarAct;
    }
    QAction* wordWrapAct() const
    {
        return m_wordWrapAct;
    }
    QAction* printAct() const
    {
        return m_printAct;
    }
    QAction* printPreviewAct() const
    {
        return m_printPreviewAct;
    }
    QAction* pageSetupAct() const
    {
        return m_pageSetupAct;
    }
    QAction* readOnlyAct() const
    {
        return m_readOnlyAct;
    }
    QAction* toggleBookmarkAct() const
    {
        return m_toggleBookmarkAct;
    }
    QAction* nextBookmarkAct() const
    {
        return m_nextBookmarkAct;
    }
    QAction* prevBookmarkAct() const
    {
        return m_prevBookmarkAct;
    }
    QAction* clearBookmarksAct() const
    {
        return m_clearBookmarksAct;
    }
    QAction* insertSnippetAct() const
    {
        return m_insertSnippetAct;
    }
    QAction* formatSelectionAct() const
    {
        return m_formatSelectionAct;
    }
    QAction* goToDefinitionAct() const
    {
        return m_goToDefinitionAct;
    }
    QAction* goToTypeDefinitionAct() const
    {
        return m_goToTypeDefinitionAct;
    }
    QAction* goToDeclarationAct() const
    {
        return m_goToDeclarationAct;
    }
    QAction* findReferencesAct() const
    {
        return m_findReferencesAct;
    }
    QAction* triggerCompletionAct() const
    {
        return m_triggerCompletionAct;
    }
    QAction* renameSymbolAct() const
    {
        return m_renameSymbolAct;
    }
    QAction* findSymbolsAct() const
    {
        return m_findSymbolsAct;
    }
    QAction* expandSelectionAct() const
    {
        return m_expandSelectionAct;
    }
    QAction* shrinkSelectionAct() const
    {
        return m_shrinkSelectionAct;
    }
    QAction* optionsAct() const
    {
        return m_optionsAct;
    }
    QAction* aboutAct() const
    {
        return m_aboutAct;
    }
    QAction* donateAct() const
    {
        return m_donateAct;
    }
    QAction* websiteAct() const
    {
        return m_websiteAct;
    }
    QAction* menuBarAct() const
    {
        return m_menuBarAct;
    }
    QMenu* recentFilesMenu() const
    {
        return m_recentFilesHelper ? m_recentFilesHelper->menu() : nullptr;
    }
    QAction* recentFileAct(int index) const
    {
        return m_recentFilesHelper ? m_recentFilesHelper->fileAction(index) : nullptr;
    }
    QAction* clearRecentFilesAct() const
    {
        return m_recentFilesHelper ? m_recentFilesHelper->clearAction() : nullptr;
    }
    QMenu* reopenEncodingMenu() const
    {
        return m_encodingHelper ? m_encodingHelper->reopenMenu() : nullptr;
    }
    QMenu* saveEncodingMenu() const
    {
        return m_encodingHelper ? m_encodingHelper->saveMenu() : nullptr;
    }
    QToolBar* toolBar() const
    {
        return m_toolBar;
    }
    QLabel* lineColLabel() const
    {
        return m_lineColLabel;
    }
    QComboBox* encodingComboBox() const
    {
        return m_encodingComboBox;
    }
    QLabel* fileTypeLabel() const
    {
        return m_fileTypeLabel;
    }
    QAction* errorListPanelAct() const
    {
        return m_errorListPanelAct;
    }
    QToolButton* projectPanelButton() const
    {
        return m_projectPanelButton;
    }
    QToolButton* terminalPanelButton() const
    {
        return m_terminalPanelButton;
    }
    QToolButton* errorListPanelButton() const
    {
        return m_errorListPanelButton;
    }

    void rebuildExternalToolsMenu();
    void wireConnections(const ActionTargets& targets);
    QList<QAction*> actionsWithShortcuts() const
    {
        return m_actionsWithShortcuts;
    }

signals:
    // ── Individual signals (existing) ──────────────────────────
    void newFileTriggered();
    void newWindowTriggered();
    void openFileTriggered();
    void openRemoteTriggered();
    void openFolderTriggered();
    void closeProjectTriggered();
    void saveFileTriggered();
    void saveFileAsTriggered();
    void saveAllTriggered();
    void revertTriggered();
    void closeCurrentTabTriggered();
    void closeAllTabsTriggered();
    void exitTriggered();
    void quitDevpadTriggered();

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
    void toggleErrorListPanelTriggered();
    void toggleToolBarTriggered();
    void toggleStatusBarTriggered();
    void toggleMenuBarTriggered();
    void toggleWordWrapTriggered();
    void printFileTriggered();
    void printPreviewTriggered();
    void pageSetupTriggered();
    void toggleReadOnlyTriggered();
    void toggleBookmarkTriggered();
    void nextBookmarkTriggered();
    void prevBookmarkTriggered();
    void clearBookmarksTriggered();
    void insertSnippetTriggered();
    void toggleCommentTriggered();
    void formatSelectionTriggered();
    void goToDefinitionTriggered();
    void goToTypeDefinitionTriggered();
    void goToDeclarationTriggered();
    void findReferencesTriggered();
    void triggerCompletionTriggered();
    void renameSymbolTriggered();
    void expandSelectionTriggered();
    void shrinkSelectionTriggered();
    void findSymbolsTriggered();
    void optionsTriggered();
    void configureExternalToolsTriggered();
    void aboutTriggered();
    void donateTriggered();
    void websiteTriggered();
    void externalToolTriggered(int index);

    void openRecentFileTriggered(const QString& filePath);
    void clearRecentFilesTriggered();
    void actionsWithShortcutsChanged();

    // ── Grouped signals ────────────────────────────────────────
    void fileActionTriggered(FileAction action);
    void editActionTriggered(EditAction action);
    void viewActionTriggered(ViewAction action);
    void documentActionTriggered(DocumentAction action);
    void lspActionTriggered(LspAction action);
    void helpActionTriggered(HelpAction action);

private:
    template <typename Functor> QAction* createIconAction(const QString& iconPath, const QString& text, const QKeySequence& shortcut, Functor slot)
    {
        QIcon icon(iconPath);
        auto* action = new QAction(icon, text, this);
        action->setShortcut(shortcut);
        if (!shortcut.isEmpty())
            m_actionsWithShortcuts.append(action);
        connect(action, &QAction::triggered, this, slot);
        return action;
    }

    void buildFileMenu(QMenu* fileMenu);
    void buildEditMenu(QMenu* editMenu);
    void buildViewMenu(QMenu* viewMenu);
    void buildToolsMenu(QMenu* toolsMenu);
    void buildHelpMenu(QMenu* helpMenu);

    QAction* m_menuBarAct = nullptr;
    QAction* m_newAct = nullptr;
    QAction* m_newWindowAct = nullptr;
    QAction* m_openAct = nullptr;
    QAction* m_openRemoteAct = nullptr;
    QAction* m_openFolderAct = nullptr;
    QAction* m_closeProjectAct = nullptr;
    QAction* m_revertAct = nullptr;
    QAction* m_saveAct = nullptr;
    QAction* m_saveAsAct = nullptr;
    QAction* m_saveAllAct = nullptr;
    QAction* m_closeAct = nullptr;
    QAction* m_closeAllAct = nullptr;
    QAction* m_exitAct = nullptr;
    QAction* m_quitDevpadAct = nullptr;
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
    QAction* m_errorListPanelAct = nullptr;
    QAction* m_toolBarAct = nullptr;
    QAction* m_statusBarAct = nullptr;
    QAction* m_wordWrapAct = nullptr;
    QAction* m_printAct = nullptr;
    QAction* m_printPreviewAct = nullptr;
    QAction* m_pageSetupAct = nullptr;
    QAction* m_readOnlyAct = nullptr;
    QAction* m_toggleBookmarkAct = nullptr;
    QAction* m_nextBookmarkAct = nullptr;
    QAction* m_prevBookmarkAct = nullptr;
    QAction* m_clearBookmarksAct = nullptr;
    QAction* m_insertSnippetAct = nullptr;
    QAction* m_toggleCommentAct = nullptr;
    QAction* m_formatSelectionAct = nullptr;
    QAction* m_goToDefinitionAct = nullptr;
    QAction* m_goToTypeDefinitionAct = nullptr;
    QAction* m_goToDeclarationAct = nullptr;
    QAction* m_findReferencesAct = nullptr;
    QAction* m_triggerCompletionAct = nullptr;
    QAction* m_renameSymbolAct = nullptr;
    QAction* m_findSymbolsAct = nullptr;
    QAction* m_expandSelectionAct = nullptr;
    QAction* m_shrinkSelectionAct = nullptr;
    QAction* m_optionsAct = nullptr;
    QAction* m_donateAct = nullptr;
    QAction* m_websiteAct = nullptr;
    QAction* m_aboutAct = nullptr;
    QList<QAction*> m_externalToolActs;
    RecentFilesHelper* m_recentFilesHelper = nullptr;
    EncodingMenuHelper* m_encodingHelper = nullptr;
    QMenu* m_toolsMenu = nullptr;
    QList<QAction*> m_actionsWithShortcuts;

    QToolBar* m_toolBar = nullptr;
    QLabel* m_lineColLabel = nullptr;
    QComboBox* m_encodingComboBox = nullptr;
    QLabel* m_fileTypeLabel = nullptr;
    QToolButton* m_projectPanelButton = nullptr;
    QToolButton* m_terminalPanelButton = nullptr;
    QToolButton* m_errorListPanelButton = nullptr;
};

#endif
