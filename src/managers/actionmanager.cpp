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
#include "actionmanager.h"

#include "dialogs/externaltoolsdialog.h"
#include "editorcontroller.h"
#include "encodingmanager.h"
#include "externaltoolmanager.h"
#include "mainwindow.h"
#include "settingsmanager.h"
#include "splitview.h"

#include <QAction>
#include <QComboBox>
#include <QDesktopServices>
#include <QDockWidget>
#include <QFile>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>
#include <algorithm>

ActionManager::ActionManager(QObject* parent) : QObject(parent)
{
    m_recentFilesHelper = new RecentFilesHelper(this);
    m_encodingHelper = new EncodingMenuHelper(this);
}

void ActionManager::createActions()
{
    createFileActions();
    createEditActions();
    createBookmarkActions();
    createLspActions();
    createDebugActions();
    createViewActions();
    createHelpActions();

    applyStoredShortcuts();
}

void ActionManager::createFileActions()
{
    m_quitDevpadAct = createIconAction(
        ":/icons/File/exit.svg", tr("Quit Devpad"), QKeySequence::Quit,
        [this]()
        {
            emit quitDevpadTriggered();
            emit fileActionTriggered(FileAction::Quit);
        },
        "File.Quit");

    m_newAct = createIconAction(
        ":/icons/File/newfile.svg", tr("New File"), QKeySequence::New,
        [this]()
        {
            emit newFileTriggered();
            emit fileActionTriggered(FileAction::New);
        },
        "File.New");
    m_newWindowAct = createIconAction(
        ":/icons/File/newwindow.svg", tr("New Window"), QKeySequence("Ctrl+Shift+N"),
        [this]()
        {
            emit newWindowTriggered();
            emit fileActionTriggered(FileAction::NewWindow);
        },
        "File.NewWindow");
    m_openAct = createIconAction(
        ":/icons/File/open.svg", tr("Open File..."), QKeySequence::Open,
        [this]()
        {
            emit openFileTriggered();
            emit fileActionTriggered(FileAction::Open);
        },
        "File.Open");
    m_openFolderAct = createIconAction(
        ":/icons/File/openfolder.svg", tr("Open Folder..."), QKeySequence("Ctrl+Shift+O"),
        [this]()
        {
            emit openFolderTriggered();
            emit fileActionTriggered(FileAction::OpenFolder);
        },
        "File.OpenFolder");
    m_openRemoteAct = createIconAction(
        ":/icons/File/open.svg", tr("Open Remote File..."), QKeySequence(),
        [this]()
        {
            emit openRemoteTriggered();
            emit fileActionTriggered(FileAction::OpenRemote);
        },
        "File.OpenRemote");
    m_openRemoteFolderAct = createIconAction(
        ":/icons/File/openfolder.svg", tr("Open Remote Folder..."), QKeySequence(),
        [this]()
        {
            emit openRemoteFolderTriggered();
            emit fileActionTriggered(FileAction::OpenRemoteFolder);
        },
        "File.OpenRemoteFolder");
    m_saveAct = createIconAction(
        ":/icons/File/save.svg", tr("Save"), QKeySequence::Save,
        [this]()
        {
            emit saveFileTriggered();
            emit fileActionTriggered(FileAction::Save);
        },
        "File.Save");
    m_saveAsAct = createIconAction(
        ":/icons/File/saveas.svg", tr("Save As..."), QKeySequence("Ctrl+Alt+S"),
        [this]()
        {
            emit saveFileAsTriggered();
            emit fileActionTriggered(FileAction::SaveAs);
        },
        "File.SaveAs");
    m_saveAllAct = createIconAction(
        ":/icons/File/saveall.svg", tr("Save All"), QKeySequence("Ctrl+Shift+S"),
        [this]()
        {
            emit saveAllTriggered();
            emit fileActionTriggered(FileAction::SaveAll);
        },
        "File.SaveAll");
    m_revertAct = createIconAction(
        ":/icons/File/revert.svg", tr("Revert"), QKeySequence(),
        [this]()
        {
            emit revertTriggered();
            emit fileActionTriggered(FileAction::Revert);
        },
        "File.Revert");
    m_readOnlyAct = new QAction(QIcon(":/icons/File/readonly.svg"), tr("Read Only"), this);
    m_readOnlyAct->setCheckable(true);
    connect(m_readOnlyAct, &QAction::triggered, this,
            [this]()
            {
                emit toggleReadOnlyTriggered();
                emit documentActionTriggered(DocumentAction::ReadOnly);
            });
    registerAction(m_readOnlyAct, "File.ReadOnly");
    m_printAct = createIconAction(":/icons/File/print.svg", tr("Print"), QKeySequence::Print, [this]() { emit printFileTriggered(); }, "File.Print");
    m_pageSetupAct = createIconAction(
        ":/icons/File/pagesetup.svg", tr("Page Setup..."), QKeySequence(),
        [this]()
        {
            emit pageSetupTriggered();
            emit documentActionTriggered(DocumentAction::PageSetup);
        },
        "File.PageSetup");
    m_printPreviewAct = createIconAction(
        ":/icons/File/printpreview.svg", tr("Print Preview..."), QKeySequence(),
        [this]()
        {
            emit printPreviewTriggered();
            emit documentActionTriggered(DocumentAction::PrintPreview);
        },
        "File.PrintPreview");
    m_closeAct = createIconAction(
        ":/icons/File/close.svg", tr("Close Editor"), QKeySequence("Ctrl+W"),
        [this]()
        {
            emit closeCurrentTabTriggered();
            emit fileActionTriggered(FileAction::CloseTab);
        },
        "File.CloseTab");
    m_closeAllAct = createIconAction(
        ":/icons/File/closeall.svg", tr("Close All"), QKeySequence("Ctrl+Alt+W"),
        [this]()
        {
            emit closeAllTabsTriggered();
            emit fileActionTriggered(FileAction::CloseAllTabs);
        },
        "File.CloseAllTabs");
    m_closeProjectAct = new QAction(QIcon(":/icons/File/closefolder.svg"), tr("Close Folder"), this);
    connect(m_closeProjectAct, &QAction::triggered, this,
            [this]()
            {
                emit closeProjectTriggered();
                emit fileActionTriggered(FileAction::CloseProject);
            });
    registerAction(m_closeProjectAct, "File.CloseProject");
    m_exitAct = createIconAction(
        ":/icons/File/closewindow.svg", tr("Close Window"), QKeySequence("Ctrl+Shift+W"),
        [this]()
        {
            emit exitTriggered();
            emit fileActionTriggered(FileAction::Exit);
        },
        "File.Exit");
}

void ActionManager::createEditActions()
{
    m_undoAct = createIconAction(
        ":/icons/Edit/undo.svg", tr("Undo"), QKeySequence::Undo,
        [this]()
        {
            emit undoTriggered();
            emit editActionTriggered(EditAction::Undo);
        },
        "Edit.Undo");
    m_redoAct = createIconAction(
        ":/icons/Edit/redo.svg", tr("Redo"), QKeySequence::Redo,
        [this]()
        {
            emit redoTriggered();
            emit editActionTriggered(EditAction::Redo);
        },
        "Edit.Redo");
    m_cutAct = createIconAction(
        ":/icons/Edit/cut.svg", tr("Cut"), QKeySequence::Cut,
        [this]()
        {
            emit cutTriggered();
            emit editActionTriggered(EditAction::Cut);
        },
        "Edit.Cut");
    m_copyAct = createIconAction(
        ":/icons/Edit/copy.svg", tr("Copy"), QKeySequence::Copy,
        [this]()
        {
            emit copyTriggered();
            emit editActionTriggered(EditAction::Copy);
        },
        "Edit.Copy");
    m_pasteAct = createIconAction(
        ":/icons/Edit/paste.svg", tr("Paste"), QKeySequence::Paste,
        [this]()
        {
            emit pasteTriggered();
            emit editActionTriggered(EditAction::Paste);
        },
        "Edit.Paste");
    m_deleteAct = createIconAction(
        ":/icons/Edit/delete.svg", tr("Delete"), QKeySequence::Delete,
        [this]()
        {
            emit deleteTriggered();
            emit editActionTriggered(EditAction::Delete);
        },
        "Edit.Delete");
    m_selectAllAct = createIconAction(
        ":/icons/Edit/selectall.svg", tr("Select All"), QKeySequence::SelectAll,
        [this]()
        {
            emit selectAllTriggered();
            emit editActionTriggered(EditAction::SelectAll);
        },
        "Edit.SelectAll");
    m_findAct = createIconAction(
        ":/icons/Edit/find.svg", tr("Find..."), QKeySequence::Find,
        [this]()
        {
            emit findTriggered();
            emit editActionTriggered(EditAction::Find);
        },
        "Edit.Find");
    m_replaceAct = createIconAction(
        ":/icons/Edit/replace.svg", tr("Replace..."), QKeySequence("Ctrl+H"),
        [this]()
        {
            emit replaceTriggered();
            emit editActionTriggered(EditAction::Replace);
        },
        "Edit.Replace");
    m_findInFilesAct = createIconAction(
        ":/icons/Edit/findinfiles.svg", tr("Find In Files..."), QKeySequence("Ctrl+Shift+F"),
        [this]()
        {
            emit findInFilesTriggered();
            emit editActionTriggered(EditAction::FindInFiles);
        },
        "Edit.FindInFiles");
    m_goToAct = createIconAction(
        ":/icons/Edit/goto.svg", tr("Go To Line..."), QKeySequence("Ctrl+G"),
        [this]()
        {
            emit goToLineTriggered();
            emit editActionTriggered(EditAction::GoToLine);
        },
        "Edit.GoToLine");
    m_formatSelectionAct = createIconAction(
        ":/icons/Edit/goto.svg", tr("Format Selection"), QKeySequence("Ctrl+Shift+F"),
        [this]()
        {
            emit formatSelectionTriggered();
            emit editActionTriggered(EditAction::FormatSelection);
        },
        "Edit.FormatSelection");
    m_insertSnippetAct = createIconAction(
        ":/icons/Edit/insertsnippet.svg", tr("Insert Snippet..."), QKeySequence("Ctrl+Shift+I"),
        [this]()
        {
            emit insertSnippetTriggered();
            emit documentActionTriggered(DocumentAction::InsertSnippet);
        },
        "Edit.InsertSnippet");
    m_toggleCommentAct = createIconAction(
        ":/icons/Edit/togglecomment.svg", tr("Toggle Comment"), QKeySequence("Ctrl+/"),
        [this]()
        {
            emit toggleCommentTriggered();
            emit editActionTriggered(EditAction::ToggleComment);
        },
        "Edit.ToggleComment");
    // Both need an editable editor; EditorController::updateUndoRedoState() enables
    // them once one is available.
    m_insertSnippetAct->setEnabled(false);
    m_toggleCommentAct->setEnabled(false);
}

void ActionManager::createBookmarkActions()
{
    m_toggleBookmarkAct = new QAction(QIcon(":/icons/Edit/togglebookmark.svg"), tr("Toggle Bookmark"), this);
    m_toggleBookmarkAct->setShortcut(QKeySequence("Ctrl+F2"));
    m_actionsWithShortcuts.append(m_toggleBookmarkAct);
    registerAction(m_toggleBookmarkAct, "Bookmark.Toggle");
    connect(m_toggleBookmarkAct, &QAction::triggered, this,
            [this]()
            {
                emit toggleBookmarkTriggered();
                emit documentActionTriggered(DocumentAction::ToggleBookmark);
            });
    m_nextBookmarkAct = new QAction(QIcon(":/icons/Edit/bookmarknext.svg"), tr("Next Bookmark"), this);
    m_nextBookmarkAct->setShortcut(QKeySequence("F2"));
    m_actionsWithShortcuts.append(m_nextBookmarkAct);
    registerAction(m_nextBookmarkAct, "Bookmark.Next");
    connect(m_nextBookmarkAct, &QAction::triggered, this,
            [this]()
            {
                emit nextBookmarkTriggered();
                emit documentActionTriggered(DocumentAction::NextBookmark);
            });
    m_prevBookmarkAct = new QAction(QIcon(":/icons/Edit/bookmarkprevious.svg"), tr("Previous Bookmark"), this);
    m_prevBookmarkAct->setShortcut(QKeySequence("Shift+F2"));
    m_actionsWithShortcuts.append(m_prevBookmarkAct);
    registerAction(m_prevBookmarkAct, "Bookmark.Previous");
    connect(m_prevBookmarkAct, &QAction::triggered, this,
            [this]()
            {
                emit prevBookmarkTriggered();
                emit documentActionTriggered(DocumentAction::PrevBookmark);
            });
    m_clearBookmarksAct = new QAction(QIcon(":/icons/Common/clear.svg"), tr("Clear All Bookmarks"), this);
    m_clearBookmarksAct->setShortcut(QKeySequence("Ctrl+Shift+F2"));
    m_actionsWithShortcuts.append(m_clearBookmarksAct);
    registerAction(m_clearBookmarksAct, "Bookmark.Clear");
    connect(m_clearBookmarksAct, &QAction::triggered, this,
            [this]()
            {
                emit clearBookmarksTriggered();
                emit documentActionTriggered(DocumentAction::ClearBookmarks);
            });
}

void ActionManager::createLspActions()
{
    m_goToDefinitionAct = createIconAction(
        ":/icons/Edit/goto.svg", tr("Go to Definition"), QKeySequence("F12"),
        [this]()
        {
            emit goToDefinitionTriggered();
            emit lspActionTriggered(LspAction::GoToDefinition);
        },
        "LSP.GoToDefinition");
    m_goToTypeDefinitionAct = createIconAction(
        ":/icons/Edit/goto.svg", tr("Go to Type Definition"), QKeySequence("Ctrl+F12"),
        [this]()
        {
            emit goToTypeDefinitionTriggered();
            emit lspActionTriggered(LspAction::GoToTypeDefinition);
        },
        "LSP.GoToTypeDefinition");
    m_goToDeclarationAct = createIconAction(
        ":/icons/Edit/goto.svg", tr("Go to Declaration"), QKeySequence("Ctrl+Shift+F12"),
        [this]()
        {
            emit goToDeclarationTriggered();
            emit lspActionTriggered(LspAction::GoToDeclaration);
        },
        "LSP.GoToDeclaration");
    m_findReferencesAct = createIconAction(
        ":/icons/Edit/find.svg", tr("Find References"), QKeySequence("Shift+F12"),
        [this]()
        {
            emit findReferencesTriggered();
            emit lspActionTriggered(LspAction::FindReferences);
        },
        "LSP.FindReferences");
    m_triggerCompletionAct = createIconAction(
        ":/icons/Edit/find.svg", tr("Trigger Completion"), QKeySequence("Ctrl+Space"),
        [this]()
        {
            emit triggerCompletionTriggered();
            emit lspActionTriggered(LspAction::TriggerCompletion);
        },
        "LSP.TriggerCompletion");
    m_renameSymbolAct = createIconAction(
        ":/icons/Common/rename.svg", tr("Rename Symbol"), QKeySequence("Ctrl+Shift+R"),
        [this]()
        {
            emit renameSymbolTriggered();
            emit lspActionTriggered(LspAction::RenameSymbol);
        },
        "LSP.RenameSymbol");
    m_codeActionAct = createIconAction(
        ":/icons/Edit/find.svg", tr("Code Action"), QKeySequence("Alt+Enter"), [this]() { emit codeActionTriggered(); }, "LSP.CodeAction");

    m_findSymbolsAct = new QAction(tr("Find Symbol..."), this);
    connect(m_findSymbolsAct, &QAction::triggered, this, [this]() { emit findSymbolsTriggered(); });
    registerAction(m_findSymbolsAct, "LSP.FindSymbols");

    m_expandSelectionAct = new QAction(tr("Expand Selection"), this);
    m_expandSelectionAct->setShortcut(QKeySequence("Ctrl+Shift+="));
    m_actionsWithShortcuts.append(m_expandSelectionAct);
    registerAction(m_expandSelectionAct, "LSP.ExpandSelection");
    connect(m_expandSelectionAct, &QAction::triggered, this, [this]() { emit expandSelectionTriggered(); });

    m_shrinkSelectionAct = new QAction(tr("Shrink Selection"), this);
    m_shrinkSelectionAct->setShortcut(QKeySequence("Ctrl+Shift+-"));
    m_actionsWithShortcuts.append(m_shrinkSelectionAct);
    registerAction(m_shrinkSelectionAct, "LSP.ShrinkSelection");
    connect(m_shrinkSelectionAct, &QAction::triggered, this, [this]() { emit shrinkSelectionTriggered(); });
}

void ActionManager::createDebugActions()
{
    m_startDebuggingAct = createIconAction(
        ":/icons/Debug/start.svg", tr("Start Debugging"), QKeySequence("F5"),
        [this]()
        {
            emit startDebuggingTriggered();
            emit debugActionTriggered(DebugAction::StartDebugging);
        },
        "Debug.Start");
    m_stopDebuggingAct = createIconAction(
        ":/icons/Debug/stop.svg", tr("Stop Debugging"), QKeySequence("Shift+F5"),
        [this]()
        {
            emit stopDebuggingTriggered();
            emit debugActionTriggered(DebugAction::StopDebugging);
        },
        "Debug.Stop");
    m_continueAct = createIconAction(
        ":/icons/Debug/continue.svg", tr("Continue"), QKeySequence("F5"),
        [this]()
        {
            emit continueTriggered();
            emit debugActionTriggered(DebugAction::Continue);
        },
        "Debug.Continue");
    m_stepOverAct = createIconAction(
        ":/icons/Debug/stepover.svg", tr("Step Over"), QKeySequence("F10"),
        [this]()
        {
            emit stepOverTriggered();
            emit debugActionTriggered(DebugAction::StepOver);
        },
        "Debug.StepOver");
    m_stepIntoAct = createIconAction(
        ":/icons/Debug/stepinto.svg", tr("Step Into"), QKeySequence("F11"),
        [this]()
        {
            emit stepIntoTriggered();
            emit debugActionTriggered(DebugAction::StepInto);
        },
        "Debug.StepInto");
    m_stepOutAct = createIconAction(
        ":/icons/Debug/stepout.svg", tr("Step Out"), QKeySequence("Shift+F11"),
        [this]()
        {
            emit stepOutTriggered();
            emit debugActionTriggered(DebugAction::StepOut);
        },
        "Debug.StepOut");
    m_restartAct = createIconAction(
        ":/icons/Debug/restart.svg", tr("Restart"), QKeySequence("Ctrl+Shift+F5"),
        [this]()
        {
            emit restartTriggered();
            emit debugActionTriggered(DebugAction::Restart);
        },
        "Debug.Restart");
}

void ActionManager::createViewActions()
{
    m_zoomInAct = createIconAction(
        ":/icons/View/zoomin.svg", tr("Zoom In"), QKeySequence("Ctrl+="),
        [this]()
        {
            emit zoomInTriggered();
            emit viewActionTriggered(ViewAction::ZoomIn);
        },
        "View.ZoomIn");
    m_zoomOutAct = createIconAction(
        ":/icons/View/zoomout.svg", tr("Zoom Out"), QKeySequence("Ctrl+-"),
        [this]()
        {
            emit zoomOutTriggered();
            emit viewActionTriggered(ViewAction::ZoomOut);
        },
        "View.ZoomOut");
    m_zoomResetAct = createIconAction(
        ":/icons/View/zoomreset.svg", tr("Reset Zoom"), QKeySequence("Ctrl+0"),
        [this]()
        {
            emit zoomResetTriggered();
            emit viewActionTriggered(ViewAction::ZoomReset);
        },
        "View.ZoomReset");
    m_fullScreenAct = createIconAction(
        ":/icons/View/fullscreen.svg", tr("Fullscreen"),
#ifdef Q_OS_MACOS
        QKeySequence(),
#else
        QKeySequence("F11"),
#endif
        [this]()
        {
            emit toggleFullScreenTriggered();
            emit viewActionTriggered(ViewAction::FullScreen);
        },
        "View.Fullscreen");
    m_projectPanelAct = new QAction(QIcon(":/icons/Common/folder.svg"), tr("Project Panel"), this);
    m_projectPanelAct->setCheckable(true);
    m_projectPanelAct->setChecked(false);
    m_projectPanelAct->setShortcut(QKeySequence("Ctrl+B"));
    m_actionsWithShortcuts.append(m_projectPanelAct);
    registerAction(m_projectPanelAct, "View.ProjectPanel");
    connect(m_projectPanelAct, &QAction::triggered, this,
            [this]()
            {
                emit toggleProjectPanelTriggered();
                emit viewActionTriggered(ViewAction::ProjectPanel);
            });
    m_terminalPanelAct = new QAction(QIcon(":/icons/View/terminal.svg"), tr("Terminal Panel"), this);
    m_terminalPanelAct->setCheckable(true);
    m_terminalPanelAct->setChecked(false);
    m_terminalPanelAct->setVisible(false); // Shown only when Terminal plugin is loaded
    m_terminalPanelAct->setShortcut(QKeySequence("Ctrl+`"));
    m_actionsWithShortcuts.append(m_terminalPanelAct);
    registerAction(m_terminalPanelAct, "View.TerminalPanel");
    connect(m_terminalPanelAct, &QAction::triggered, this,
            [this]()
            {
                emit toggleTerminalPanelTriggered();
                emit viewActionTriggered(ViewAction::TerminalPanel);
            });
    m_errorListPanelAct = new QAction(QIcon(":/icons/View/error.svg"), tr("Error List"), this);
    m_errorListPanelAct->setCheckable(true);
    m_errorListPanelAct->setChecked(false);
    registerAction(m_errorListPanelAct, "View.ErrorList");
    connect(m_errorListPanelAct, &QAction::triggered, this, [this]() { emit toggleErrorListPanelTriggered(); });

    m_referencesPanelAct = new QAction(QIcon(":/icons/Edit/find.svg"), tr("References"), this);
    m_referencesPanelAct->setCheckable(true);
    m_referencesPanelAct->setChecked(false);
    registerAction(m_referencesPanelAct, "View.References");
    connect(m_referencesPanelAct, &QAction::triggered, this, [this]() { emit toggleReferencesPanelTriggered(); });

    m_debugPanelAct = new QAction(QIcon(":/icons/Debug/start.svg"), tr("Debug Panel"), this);
    m_debugPanelAct->setCheckable(true);
    m_debugPanelAct->setChecked(false);
    registerAction(m_debugPanelAct, "View.DebugPanel");
    connect(m_debugPanelAct, &QAction::triggered, this, [this]() { emit toggleDebugPanelTriggered(); });

    m_menuBarAct = new QAction(QIcon(":/icons/View/ui.svg"), tr("Menu Bar\tCtrl+Alt+M"), this);
    m_menuBarAct->setCheckable(true);
    registerAction(m_menuBarAct, "View.MenuBar");
    connect(m_menuBarAct, &QAction::triggered, this,
            [this]()
            {
                emit toggleMenuBarTriggered();
                emit viewActionTriggered(ViewAction::MenuBar);
            });

    m_toolBarAct = new QAction(QIcon(":/icons/View/ui.svg"), tr("Toolbar"), this);
    m_toolBarAct->setCheckable(true);
    registerAction(m_toolBarAct, "View.ToolBar");
    connect(m_toolBarAct, &QAction::triggered, this,
            [this]()
            {
                emit toggleToolBarTriggered();
                emit viewActionTriggered(ViewAction::ToolBar);
            });

    m_statusBarAct = new QAction(QIcon(":/icons/View/ui.svg"), tr("Status Bar"), this);
    m_statusBarAct->setCheckable(true);
    registerAction(m_statusBarAct, "View.StatusBar");
    connect(m_statusBarAct, &QAction::triggered, this,
            [this]()
            {
                emit toggleStatusBarTriggered();
                emit viewActionTriggered(ViewAction::StatusBar);
            });

#ifdef Q_OS_WIN
    m_menuTitlebarAct = new QAction(QIcon(":/icons/View/ui.svg"), tr("Menu in Titlebar"), this);
    m_menuTitlebarAct->setCheckable(true);
    m_menuTitlebarAct->setChecked(SettingsManager::instance().showMenuInTitlebar());
    registerAction(m_menuTitlebarAct, "View.MenuTitlebar");
    connect(m_menuTitlebarAct, &QAction::triggered, this, [this]() { emit toggleMenuTitlebarTriggered(); });
#endif
}

void ActionManager::createHelpActions()
{
    m_optionsAct =
        createIconAction(":/icons/Tools/options.svg", tr("Options"), QKeySequence(), [this]() { emit optionsTriggered(); }, "Help.Options");
#ifdef Q_OS_MACOS
    m_optionsAct->setMenuRole(QAction::PreferencesRole);
#endif

    m_donateAct = createIconAction(":/icons/Help/donate.svg", tr("Donate"), QKeySequence(), [this]() { emit donateTriggered(); }, "Help.Donate");
    m_websiteAct = createIconAction(":/icons/Help/website.svg", tr("Website"), QKeySequence(), [this]() { emit websiteTriggered(); }, "Help.Website");
    m_aboutAct = createIconAction(":/icons/Help/about.svg", tr("About"), QKeySequence(), [this]() { emit aboutTriggered(); }, "Help.About");
    m_checkForUpdatesAct = createIconAction(
        ":/icons/Help/update.svg", tr("Check for Updates"), QKeySequence(), [this]() { emit checkForUpdatesTriggered(); }, "Help.CheckForUpdates");
}

void ActionManager::registerAction(QAction* action, const QString& actionId)
{
    if (!action || actionId.isEmpty())
        return;

    // Remove any previous registration for this action or ID.
    if (m_actionIds.contains(action))
    {
        QString oldId = m_actionIds.value(action);
        m_actionsById.remove(oldId);
    }
    m_actionsById.remove(actionId);

    m_actionIds.insert(action, actionId);
    m_actionsById.insert(actionId, action);
}

void ActionManager::applyStoredShortcuts()
{
    for (auto it = m_actionsById.constBegin(); it != m_actionsById.constEnd(); ++it)
    {
        const QString& actionId = it.key();
        QAction* action = it.value();
        QString stored = SettingsManager::instance().shortcutForAction(actionId);
        if (!stored.isEmpty())
        {
            QKeySequence ks(stored);
            action->setShortcut(ks);
            if (!ks.isEmpty())
            {
                m_actionsWithShortcuts.removeAll(action);
                m_actionsWithShortcuts.append(action);
            }
        }
    }
    emit actionsWithShortcutsChanged();
}

QString ActionManager::actionId(QAction* action) const
{
    return m_actionIds.value(action, QString());
}

QAction* ActionManager::actionForId(const QString& actionId) const
{
    return m_actionsById.value(actionId, nullptr);
}

QList<QPair<QString, QKeySequence>> ActionManager::shortcuts() const
{
    QList<QPair<QString, QKeySequence>> result;
    for (auto it = m_actionsById.constBegin(); it != m_actionsById.constEnd(); ++it)
        result.append({it.key(), it.value()->shortcut()});
    std::sort(result.begin(), result.end(),
              [](const QPair<QString, QKeySequence>& a, const QPair<QString, QKeySequence>& b) { return a.first < b.first; });
    return result;
}

void ActionManager::setShortcut(const QString& actionId, const QKeySequence& shortcut)
{
    QAction* action = m_actionsById.value(actionId);
    if (!action)
        return;

    action->setShortcut(shortcut);
    if (!shortcut.isEmpty())
        SettingsManager::instance().setShortcutForAction(actionId, shortcut.toString());
    else
        SettingsManager::instance().resetShortcutForAction(actionId);

    // Refresh the m_actionsWithShortcuts list.
    m_actionsWithShortcuts.removeAll(action);
    if (!shortcut.isEmpty())
        m_actionsWithShortcuts.append(action);

    emit actionsWithShortcutsChanged();
    emit shortcutChanged(actionId, shortcut);
}

void ActionManager::resetShortcut(const QString& actionId)
{
    QAction* action = m_actionsById.value(actionId);
    if (!action)
        return;

    SettingsManager::instance().resetShortcutForAction(actionId);

    // Restore the built-in default shortcut.
    QKeySequence def = m_defaultShortcuts.value(actionId);
    action->setShortcut(def);

    m_actionsWithShortcuts.removeAll(action);
    if (!def.isEmpty())
        m_actionsWithShortcuts.append(action);

    emit actionsWithShortcutsChanged();
    emit shortcutChanged(actionId, def);
}

void ActionManager::buildMenus(QMenuBar* menuBar)
{
#ifdef Q_OS_MACOS
    m_checkForUpdatesAct->setMenuRole(QAction::ApplicationSpecificRole);
    m_quitDevpadAct->setMenuRole(QAction::QuitRole);
    m_aboutAct->setMenuRole(QAction::AboutRole);
#endif

    QMenu* devpadMenu = menuBar->addMenu(tr("Devpad"));
    devpadMenu->addAction(m_checkForUpdatesAct);
    devpadMenu->addSeparator();
    devpadMenu->addAction(m_quitDevpadAct);

#ifndef Q_OS_MACOS
    QAction* sep = new QAction(tr("|"), this);
    sep->setEnabled(false);
    menuBar->addAction(sep);
#endif

    QMenu* fileMenu = menuBar->addMenu(tr("File"));
    buildFileMenu(fileMenu);

    QMenu* editMenu = menuBar->addMenu(tr("Edit"));
    buildEditMenu(editMenu);

    QMenu* viewMenu = menuBar->addMenu(tr("View"));
    buildViewMenu(viewMenu);

    QMenu* toolsMenu = menuBar->addMenu(tr("Tools"));
    buildToolsMenu(toolsMenu);

    QMenu* helpMenu = menuBar->addMenu(tr("Help"));
    buildHelpMenu(helpMenu);
}

void ActionManager::buildFileMenu(QMenu* fileMenu)
{
    QMenu* newMenu = fileMenu->addMenu(QIcon(":/icons/File/new.svg"), tr("New"));
    newMenu->addAction(m_newAct);
    newMenu->addAction(m_newWindowAct);
    fileMenu->addSeparator();
    QMenu* openMenu = fileMenu->addMenu(QIcon(":/icons/File/open.svg"), tr("Open"));
    openMenu->addAction(m_openAct);
    openMenu->addAction(m_openFolderAct);
    openMenu->addAction(m_openRemoteAct);
    openMenu->addAction(m_openRemoteFolderAct);
    fileMenu->addSeparator();

    fileMenu->addMenu(m_recentFilesHelper->createMenu(fileMenu));

    fileMenu->addSeparator();
    fileMenu->addAction(m_saveAct);
    fileMenu->addAction(m_saveAsAct);
    fileMenu->addAction(m_saveAllAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_revertAct);
    fileMenu->addSeparator();
    QMenu* propertiesMenu = fileMenu->addMenu(QIcon(":/icons/File/properties.svg"), tr("Properties"));
    propertiesMenu->addAction(m_readOnlyAct);
    propertiesMenu->addSeparator();
    propertiesMenu->addMenu(m_encodingHelper->createReopenMenu(propertiesMenu));
    propertiesMenu->addMenu(m_encodingHelper->createSaveMenu(propertiesMenu));
    fileMenu->addSeparator();
    QMenu* printMenu = fileMenu->addMenu(QIcon(":/icons/File/print.svg"), tr("Print"));
    printMenu->addAction(m_printAct);
    printMenu->addAction(m_pageSetupAct);
    printMenu->addAction(m_printPreviewAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_closeAct);
    fileMenu->addAction(m_closeAllAct);
    fileMenu->addAction(m_closeProjectAct);
    fileMenu->addAction(m_exitAct);
}

void ActionManager::buildEditMenu(QMenu* editMenu)
{
    editMenu->addAction(m_undoAct);
    editMenu->addAction(m_redoAct);
    editMenu->addSeparator();
    editMenu->addAction(m_cutAct);
    editMenu->addAction(m_copyAct);
    editMenu->addAction(m_pasteAct);
    editMenu->addAction(m_deleteAct);
    editMenu->addSeparator();
    editMenu->addAction(m_selectAllAct);
    editMenu->addSeparator();
    editMenu->addAction(m_findAct);
    editMenu->addAction(m_replaceAct);
    editMenu->addAction(m_findInFilesAct);
    editMenu->addAction(m_goToAct);
    editMenu->addSeparator();
    editMenu->addAction(m_insertSnippetAct);
    editMenu->addAction(m_toggleCommentAct);
    editMenu->addSeparator();
    QMenu* bookmarksMenu = editMenu->addMenu(tr("Bookmarks"));
    bookmarksMenu->setIcon(QIcon(":/icons/Edit/bookmarks.svg"));
    bookmarksMenu->addAction(m_toggleBookmarkAct);
    bookmarksMenu->addAction(m_nextBookmarkAct);
    bookmarksMenu->addAction(m_prevBookmarkAct);
    bookmarksMenu->addSeparator();
    bookmarksMenu->addAction(m_clearBookmarksAct);
    editMenu->addSeparator();
    QMenu* lspMenu = editMenu->addMenu(tr("Language Server"));
    lspMenu->setIcon(QIcon(":/icons/Tools/options.svg"));
    lspMenu->addAction(m_goToDefinitionAct);
    lspMenu->addAction(m_goToTypeDefinitionAct);
    lspMenu->addAction(m_goToDeclarationAct);
    lspMenu->addAction(m_findReferencesAct);
    lspMenu->addAction(m_triggerCompletionAct);
    lspMenu->addAction(m_renameSymbolAct);
    lspMenu->addAction(m_codeActionAct);
    lspMenu->addAction(m_findSymbolsAct);
    lspMenu->addAction(m_expandSelectionAct);
    lspMenu->addAction(m_shrinkSelectionAct);
    lspMenu->addSeparator();
    lspMenu->addAction(m_formatSelectionAct);

    QMenu* debugMenu = editMenu->addMenu(tr("Debug"));
    debugMenu->setIcon(QIcon(":/icons/Debug/start.svg"));
    debugMenu->addAction(m_startDebuggingAct);
    debugMenu->addAction(m_stopDebuggingAct);
    debugMenu->addSeparator();
    debugMenu->addAction(m_continueAct);
    debugMenu->addAction(m_stepOverAct);
    debugMenu->addAction(m_stepIntoAct);
    debugMenu->addAction(m_stepOutAct);
    debugMenu->addSeparator();
    debugMenu->addAction(m_restartAct);
}

void ActionManager::buildViewMenu(QMenu* viewMenu)
{
    viewMenu->addAction(m_zoomInAct);
    viewMenu->addAction(m_zoomOutAct);
    viewMenu->addAction(m_zoomResetAct);
    viewMenu->addSeparator();
    viewMenu->addAction(m_projectPanelAct);
    viewMenu->addAction(m_terminalPanelAct);
    viewMenu->addAction(m_errorListPanelAct);
    viewMenu->addAction(m_referencesPanelAct);
    viewMenu->addAction(m_debugPanelAct);
    viewMenu->addSeparator();
    viewMenu->addAction(m_menuBarAct);
    viewMenu->addAction(m_toolBarAct);
    viewMenu->addAction(m_statusBarAct);
#ifdef Q_OS_WIN
    viewMenu->addAction(m_menuTitlebarAct);
#endif
    viewMenu->addSeparator();
#ifndef Q_OS_MACOS
    viewMenu->addAction(m_fullScreenAct);
#endif
} // createViewMenu

void ActionManager::buildToolsMenu(QMenu* toolsMenu)
{
    m_toolsMenu = toolsMenu;
    rebuildExternalToolsMenu();
}

void ActionManager::rebuildExternalToolsMenu()
{
    if (!m_toolsMenu)
        return;

    for (QAction* act : m_externalToolActs)
    {
        m_actionsWithShortcuts.removeAll(act);
        m_toolsMenu->removeAction(act);
        delete act;
    }
    m_externalToolActs.clear();

    int count = SettingsManager::instance().externalToolCount();
    for (int i = 0; i < count; ++i)
    {
        QString name = SettingsManager::instance().externalToolName(i);
        QString shortcutStr = SettingsManager::instance().externalToolShortcut(i);
        QKeySequence ks(shortcutStr);

        QAction* act = new QAction(name, this);
        if (!ks.isEmpty())
            act->setShortcut(ks);
        int index = i;
        connect(act, &QAction::triggered, this, [this, index]() { emit externalToolTriggered(index); });
        m_toolsMenu->addAction(act);
        m_externalToolActs.append(act);
        if (!ks.isEmpty())
            m_actionsWithShortcuts.append(act);
    }

    if (count > 0)
        m_toolsMenu->addSeparator();

    QAction* configureAct = new QAction(QIcon(":/icons/Tools/externaltools.svg"), tr("External Tools..."), this);
    connect(configureAct, &QAction::triggered, this,
            [this]()
            {
                emit configureExternalToolsTriggered();
                emit helpActionTriggered(HelpAction::ConfigureExternalTools);
            });
    m_toolsMenu->addAction(configureAct);
    m_externalToolActs.append(configureAct);

    m_toolsMenu->addSeparator();
    m_toolsMenu->removeAction(m_optionsAct);
    m_toolsMenu->addAction(m_optionsAct);

    emit actionsWithShortcutsChanged();
}

void ActionManager::buildHelpMenu(QMenu* helpMenu)
{
    helpMenu->addAction(m_donateAct);
    helpMenu->addSeparator();
    helpMenu->addAction(m_websiteAct);
    helpMenu->addSeparator();
    helpMenu->addAction(m_aboutAct);
}

QToolBar* ActionManager::buildToolBar()
{
    m_toolBar = new QToolBar(tr("Main"));
    m_toolBar->addAction(m_newAct);
    m_toolBar->addAction(m_openAct);
    m_toolBar->addAction(m_saveAct);
    m_toolBar->addAction(m_saveAllAct);
    m_toolBar->addAction(m_printAct);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_undoAct);
    m_toolBar->addAction(m_redoAct);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_cutAct);
    m_toolBar->addAction(m_copyAct);
    m_toolBar->addAction(m_pasteAct);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_findAct);
    m_toolBar->addAction(m_replaceAct);
    m_toolBar->addAction(m_findInFilesAct);
    m_toolBar->addAction(m_goToAct);
    return m_toolBar;
}

void ActionManager::wireConnections(const ActionTargets& t)
{
    wireFileConnections(t);
    wireEditConnections(t);
    wireViewConnections(t);
    wireDocumentConnections(t);
    wireLspConnections(t);
    wireDebugConnections(t);
    wireHelpConnections(t);
    wireMiscConnections(t);
}

void ActionManager::wireFileConnections(const ActionTargets& t)
{
    auto* ec = t.editorController;
    auto* mw = t.mainWindow;

    connect(this, &ActionManager::newFileTriggered, ec, &EditorController::newFile);
    connect(this, &ActionManager::newWindowTriggered, mw, &MainWindow::newWindow);
    connect(this, &ActionManager::openFileTriggered, mw, &MainWindow::openFile);
    connect(this, &ActionManager::openRemoteTriggered, mw, &MainWindow::openRemote);
    connect(this, &ActionManager::openRemoteFolderTriggered, mw, &MainWindow::openRemoteFolder);
    connect(this, &ActionManager::openFolderTriggered, mw, &MainWindow::openFolder);
    connect(this, &ActionManager::closeProjectTriggered, mw, &MainWindow::closeProject);
    connect(this, &ActionManager::saveFileTriggered, ec, &EditorController::saveFile);
    connect(this, &ActionManager::saveFileAsTriggered, ec, &EditorController::saveFileAs);
    connect(this, &ActionManager::saveAllTriggered, ec, &EditorController::saveAll);
    connect(this, &ActionManager::revertTriggered, mw, &MainWindow::revertFile);
    connect(this, &ActionManager::closeCurrentTabTriggered, ec, &EditorController::closeCurrentTab);
    connect(this, &ActionManager::closeAllTabsTriggered, ec, &EditorController::closeAllTabs);
    connect(this, &ActionManager::exitTriggered, mw, &QWidget::close);
    connect(this, &ActionManager::quitDevpadTriggered, mw, &MainWindow::quitDevpad);
}

void ActionManager::wireEditConnections(const ActionTargets& t)
{
    auto* ec = t.editorController;
    auto* mw = t.mainWindow;

    connect(this, &ActionManager::undoTriggered, ec, &EditorController::undo);
    connect(this, &ActionManager::redoTriggered, ec, &EditorController::redo);
    connect(this, &ActionManager::cutTriggered, ec, &EditorController::cut);
    connect(this, &ActionManager::copyTriggered, mw, &MainWindow::copy);
    connect(this, &ActionManager::pasteTriggered, mw, &MainWindow::paste);
    connect(this, &ActionManager::selectAllTriggered, ec, &EditorController::selectAll);
    connect(this, &ActionManager::deleteTriggered, ec, &EditorController::deleteText);
    connect(this, &ActionManager::findTriggered, mw, &MainWindow::find);
    connect(this, &ActionManager::replaceTriggered, mw, &MainWindow::replace);
    connect(this, &ActionManager::findInFilesTriggered, mw, &MainWindow::findInFiles);
    connect(this, &ActionManager::goToLineTriggered, mw, &MainWindow::goToLine);
    connect(this, &ActionManager::findNextTriggered, mw, &MainWindow::findNext);
    connect(this, &ActionManager::findPreviousTriggered, mw, &MainWindow::findPrevious);
}

void ActionManager::wireViewConnections(const ActionTargets& t)
{
    auto* ec = t.editorController;
    auto* mw = t.mainWindow;

    connect(this, &ActionManager::zoomInTriggered, ec, &EditorController::zoomIn);
    connect(this, &ActionManager::zoomOutTriggered, ec, &EditorController::zoomOut);
    connect(this, &ActionManager::zoomResetTriggered, ec, &EditorController::zoomReset);
    connect(this, &ActionManager::toggleFullScreenTriggered, mw, &MainWindow::toggleFullScreen);
    connect(this, &ActionManager::toggleProjectPanelTriggered, mw, &MainWindow::toggleProjectPanel);
    connect(this, &ActionManager::toggleTerminalPanelTriggered, mw, &MainWindow::toggleTerminalPanel);
    connect(this, &ActionManager::toggleErrorListPanelTriggered, mw, &MainWindow::toggleErrorListPanel);
    connect(this, &ActionManager::toggleDebugPanelTriggered, mw, &MainWindow::toggleDebugPanel);
    connect(this, &ActionManager::toggleReadOnlyTriggered, ec, &EditorController::toggleReadOnly);
    connect(this, &ActionManager::toggleWordWrapTriggered, ec, &EditorController::toggleWordWrap);

    connect(this, &ActionManager::toggleMenuBarTriggered, this,
            [this, mw]()
            {
                bool visible = !mw->menuBarWidget()->isVisible();
                mw->menuBarWidget()->setVisible(visible);
                m_menuBarAct->setChecked(visible);
                SettingsManager::instance().setShowMenuBar(visible);
            });
#ifdef Q_OS_WIN
    connect(this, &ActionManager::toggleMenuTitlebarTriggered, this,
            [this, mw]()
            {
                bool enabled = !SettingsManager::instance().showMenuInTitlebar();
                SettingsManager::instance().setShowMenuInTitlebar(enabled);
                mw->applyTitleBarMode();
                m_menuTitlebarAct->setChecked(enabled);
            });
#endif
    connect(this, &ActionManager::toggleToolBarTriggered, this,
            [this]()
            {
                bool visible = m_toolBar->isVisible();
                m_toolBar->setVisible(!visible);
                m_toolBarAct->setChecked(!visible);
                SettingsManager::instance().setShowToolbar(!visible);
            });
    connect(this, &ActionManager::toggleStatusBarTriggered, this,
            [this, mw]()
            {
                bool visible = mw->statusBar()->isVisible();
                mw->statusBar()->setVisible(!visible);
                m_statusBarAct->setChecked(!visible);
                SettingsManager::instance().setShowStatusbar(!visible);
            });
}

void ActionManager::wireDocumentConnections(const ActionTargets& t)
{
    auto* ec = t.editorController;
    auto* mw = t.mainWindow;

    connect(this, &ActionManager::toggleBookmarkTriggered, ec, &EditorController::toggleBookmark);
    connect(this, &ActionManager::nextBookmarkTriggered, ec, &EditorController::nextBookmark);
    connect(this, &ActionManager::prevBookmarkTriggered, ec, &EditorController::prevBookmark);
    connect(this, &ActionManager::clearBookmarksTriggered, ec, &EditorController::clearBookmarks);
    connect(this, &ActionManager::insertSnippetTriggered, ec, &EditorController::insertSnippet);
    connect(this, &ActionManager::toggleCommentTriggered, ec, &EditorController::toggleComment);
    connect(this, &ActionManager::formatSelectionTriggered, mw, &MainWindow::formatSelection);
    connect(this, &ActionManager::printFileTriggered, mw, &MainWindow::printFile);
    connect(this, &ActionManager::pageSetupTriggered, mw, &MainWindow::pageSetup);
    connect(this, &ActionManager::printPreviewTriggered, mw, &MainWindow::printPreview);

    connect(t.splitView, &SplitView::closeAllTabsRequested, ec, &EditorController::closeAllTabs);
    connect(t.splitView, &SplitView::tabPinToggled, this, t.tabPinToggled);
}

void ActionManager::wireLspConnections(const ActionTargets& t)
{
    auto* mw = t.mainWindow;

    connect(this, &ActionManager::goToDefinitionTriggered, mw, &MainWindow::goToDefinition);
    connect(this, &ActionManager::goToTypeDefinitionTriggered, mw, &MainWindow::goToTypeDefinition);
    connect(this, &ActionManager::goToDeclarationTriggered, mw, &MainWindow::goToDeclaration);
    connect(this, &ActionManager::findReferencesTriggered, mw, &MainWindow::findReferences);
    connect(this, &ActionManager::triggerCompletionTriggered, mw, &MainWindow::triggerCompletion);
    connect(this, &ActionManager::renameSymbolTriggered, mw, &MainWindow::renameSymbol);
    connect(this, &ActionManager::findSymbolsTriggered, mw, &MainWindow::findSymbols);
    connect(this, &ActionManager::expandSelectionTriggered, mw, &MainWindow::expandSelection);
    connect(this, &ActionManager::shrinkSelectionTriggered, mw, &MainWindow::shrinkSelection);
    connect(this, &ActionManager::codeActionTriggered, mw, &MainWindow::requestCodeActions);
    connect(this, &ActionManager::toggleReferencesPanelTriggered, mw, &MainWindow::toggleReferencesPanel);
}

void ActionManager::wireDebugConnections(const ActionTargets& t)
{
    auto* mw = t.mainWindow;

    connect(this, &ActionManager::startDebuggingTriggered, mw, &MainWindow::startDebugging);
    connect(this, &ActionManager::stopDebuggingTriggered, mw, &MainWindow::stopDebugging);
    connect(this, &ActionManager::continueTriggered, mw, &MainWindow::debugContinue);
    connect(this, &ActionManager::stepOverTriggered, mw, &MainWindow::debugStepOver);
    connect(this, &ActionManager::stepIntoTriggered, mw, &MainWindow::debugStepInto);
    connect(this, &ActionManager::stepOutTriggered, mw, &MainWindow::debugStepOut);
    connect(this, &ActionManager::restartTriggered, mw, &MainWindow::debugRestart);
}

void ActionManager::wireHelpConnections(const ActionTargets& t)
{
    auto* mw = t.mainWindow;

    connect(this, &ActionManager::optionsTriggered, mw, &MainWindow::showOptions);
    connect(this, &ActionManager::configureExternalToolsTriggered, this,
            [this, mw]()
            {
                ExternalToolsDialog dlg(mw);
                if (dlg.exec() == QDialog::Accepted)
                    rebuildExternalToolsMenu();
            });
    connect(this, &ActionManager::aboutTriggered, mw, &MainWindow::showAbout);
    connect(this, &ActionManager::checkForUpdatesTriggered, mw, &MainWindow::checkForUpdates);
    connect(this, &ActionManager::donateTriggered, this,
            []() { QDesktopServices::openUrl(QUrl("https://www.paypal.com/ncp/payment/RFY5Z3KJ8UY8W")); });
    connect(this, &ActionManager::websiteTriggered, this, []() { QDesktopServices::openUrl(QUrl("https://semagsoft.com")); });
    connect(this, &ActionManager::externalToolTriggered, mw, &MainWindow::runExternalTool);
}

void ActionManager::wireMiscConnections(const ActionTargets& t)
{
    auto* ec = t.editorController;
    auto* mw = t.mainWindow;

    connect(m_recentFilesHelper, &RecentFilesHelper::openRecentFileTriggered, this, &ActionManager::openRecentFileTriggered);
    connect(m_recentFilesHelper, &RecentFilesHelper::clearRecentFilesTriggered, this, &ActionManager::clearRecentFilesTriggered);

    connect(this, &ActionManager::openRecentFileTriggered, this,
            [t](const QString& filePath)
            {
                if (t.openRecentFile && !filePath.isEmpty() && QFile::exists(filePath))
                    t.openRecentFile(filePath);
            });
    connect(this, &ActionManager::clearRecentFilesTriggered, this,
            [t]()
            {
                SettingsManager::instance().clearRecentFiles();
                if (t.updateRecentFileActions)
                    t.updateRecentFileActions();
            });

    if (t.encodingManager)
    {
        connect(t.encodingManager, &EncodingManager::encodingSelected, this, t.encodingSelected);
        t.encodingManager->populateEncodingMenu(m_encodingHelper->reopenMenu(), true);
        t.encodingManager->populateEncodingMenu(m_encodingHelper->saveMenu(), false);
    }

    connect(ec, &EditorController::fileSaved, this,
            [t](const QString&)
            {
                if (t.updateRecentFileActions)
                    t.updateRecentFileActions();
            });

    connect(this, &ActionManager::actionsWithShortcutsChanged, this, [this, mw]() { mw->addActions(actionsWithShortcuts()); });
}

void ActionManager::buildStatusBar(QStatusBar* statusBar, ProjectPanel* projectPanel, TerminalPanel* terminalPanel)
{
    m_projectPanel = projectPanel;
    m_terminalPanel = terminalPanel;
    m_lineColLabel = new QLabel(tr("Line: 1, Col: 1"));
    m_encodingComboBox = new QComboBox(statusBar);
    m_encodingComboBox->setFocusPolicy(Qt::NoFocus);
    m_encodingComboBox->setFixedWidth(100);
    m_encodingComboBox->setStyleSheet("QComboBox { border: none; padding: 0 2px; }");
    m_fileTypeLabel = new QLabel(tr("Plain Text"));
    m_lineColLabel->setVisible(false);
    m_encodingComboBox->setVisible(false);
    m_fileTypeLabel->setVisible(false);
    statusBar->addPermanentWidget(m_lineColLabel);
    statusBar->addPermanentWidget(m_fileTypeLabel);
    statusBar->addPermanentWidget(m_encodingComboBox);

    m_projectPanelButton = new QToolButton(statusBar);
    m_projectPanelButton->setIcon(QIcon(":/icons/Common/folder.svg"));
    m_projectPanelButton->setCheckable(true);
    m_projectPanelButton->setCursor(Qt::PointingHandCursor);
    m_projectPanelButton->setStyleSheet(
        "QToolButton { border: none; padding: 2px 4px; } QToolButton:checked { background-color: palette(highlight); }");
    connect(m_projectPanelButton, &QToolButton::clicked, this, [this]() { emit toggleProjectPanelTriggered(); });
    statusBar->addWidget(m_projectPanelButton);

    m_terminalPanelButton = new QToolButton(statusBar);
    m_terminalPanelButton->setIcon(QIcon(":/icons/View/terminal.svg"));
    m_terminalPanelButton->setCheckable(true);
    m_terminalPanelButton->setCursor(Qt::PointingHandCursor);
    m_terminalPanelButton->setStyleSheet(
        "QToolButton { border: none; padding: 2px 4px; } QToolButton:checked { background-color: palette(highlight); }");
    connect(m_terminalPanelButton, &QToolButton::clicked, this, [this]() { emit toggleTerminalPanelTriggered(); });
    statusBar->addWidget(m_terminalPanelButton);

    m_errorListPanelButton = new QToolButton(statusBar);
    m_errorListPanelButton->setIcon(QIcon(":/icons/View/error.svg"));
    m_errorListPanelButton->setCheckable(true);
    m_errorListPanelButton->setCursor(Qt::PointingHandCursor);
    m_errorListPanelButton->setStyleSheet(
        "QToolButton { border: none; padding: 2px 4px; } QToolButton:checked { background-color: palette(highlight); }");
    connect(m_errorListPanelButton, &QToolButton::clicked, this, [this]() { emit toggleErrorListPanelTriggered(); });
    statusBar->addWidget(m_errorListPanelButton);

    m_referencesPanelButton = new QToolButton(statusBar);
    m_referencesPanelButton->setIcon(QIcon(":/icons/Edit/find.svg"));
    m_referencesPanelButton->setCheckable(true);
    m_referencesPanelButton->setCursor(Qt::PointingHandCursor);
    m_referencesPanelButton->setStyleSheet(
        "QToolButton { border: none; padding: 2px 4px; } QToolButton:checked { background-color: palette(highlight); }");
    connect(m_referencesPanelButton, &QToolButton::clicked, this, [this]() { emit toggleReferencesPanelTriggered(); });
    statusBar->addWidget(m_referencesPanelButton);
}
