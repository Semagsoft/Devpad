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
#include "externaltoolmanager.h"
#include "settingsmanager.h"
#include <QMenuBar>
#include <QDockWidget>

ActionManager::ActionManager(QObject *parent) : QObject(parent) {
}

void ActionManager::createActions() {
    m_newAct = createIconAction(":/icons/File/new.svg", tr("New"), QKeySequence::New, [this]() { emit newFileTriggered(); });
    m_newWindowAct = createIconAction(":/icons/File/newwindow.svg", tr("New Window"), QKeySequence("Ctrl+Shift+N"), [this]() { emit newWindowTriggered(); });
    m_openAct = createIconAction(":/icons/File/open.svg", tr("Open File..."), QKeySequence::Open, [this]() { emit openFileTriggered(); });
    m_openRemoteAct = createIconAction(":/icons/File/open.svg", tr("Open Remote..."), QKeySequence(), [this]() { emit openRemoteTriggered(); });
    m_openFolderAct = createIconAction(":/icons/File/openfolder.svg", tr("Open Folder..."), QKeySequence("Ctrl+Shift+O"), [this]() { emit openFolderTriggered(); });
    m_saveAct = createIconAction(":/icons/File/save.svg", tr("Save"), QKeySequence::Save, [this]() { emit saveFileTriggered(); });
    m_saveAsAct = createIconAction(":/icons/File/saveas.svg", tr("Save As..."), QKeySequence("Ctrl+Alt+S"), [this]() { emit saveFileAsTriggered(); });
    m_saveAllAct = createIconAction(":/icons/File/saveall.svg", tr("Save All"), QKeySequence("Ctrl+Shift+S"), [this]() { emit saveAllTriggered(); });
    m_revertAct = createIconAction(":/icons/File/revert.svg", tr("Revert"), QKeySequence(), [this]() { emit revertTriggered(); });
    m_closeAct = createIconAction(":/icons/File/close.svg", tr("Close Editor"), QKeySequence("Ctrl+W"), [this]() { emit closeCurrentTabTriggered(); });
    m_closeAllAct = createIconAction(":/icons/File/closeall.svg", tr("Close All"), QKeySequence("Ctrl+Alt+W"), [this]() { emit closeAllTabsTriggered(); });
m_exitAct = createIconAction(":/icons/File/closewindow.svg", tr("Close Window"), QKeySequence("Ctrl+Shift+W"), [this]() { emit exitTriggered(); });
    m_quitDevpadAct = createIconAction(":/icons/File/exit.svg", tr("Quit Devpad"), QKeySequence::Quit, [this]() { emit quitDevpadTriggered(); });
    m_undoAct = createIconAction(":/icons/Edit/undo.svg", tr("Undo"), QKeySequence::Undo, [this]() { emit undoTriggered(); });
    m_redoAct = createIconAction(":/icons/Edit/redo.svg", tr("Redo"), QKeySequence::Redo, [this]() { emit redoTriggered(); });
    m_cutAct = createIconAction(":/icons/Edit/cut.svg", tr("Cut"), QKeySequence::Cut, [this]() { emit cutTriggered(); });
    m_copyAct = createIconAction(":/icons/Edit/copy.svg", tr("Copy"), QKeySequence::Copy, [this]() { emit copyTriggered(); });
    m_pasteAct = createIconAction(":/icons/Edit/paste.svg", tr("Paste"), QKeySequence::Paste, [this]() { emit pasteTriggered(); });
    m_selectAllAct = createIconAction(":/icons/Edit/selectall.svg", tr("Select All"), QKeySequence::SelectAll, [this]() { emit selectAllTriggered(); });
    m_deleteAct = createIconAction(":/icons/Edit/delete.svg", tr("Delete"), QKeySequence::Delete, [this]() { emit deleteTriggered(); });
    m_findAct = createIconAction(":/icons/Edit/find.svg", tr("Find..."), QKeySequence::Find, [this]() { emit findTriggered(); });
    m_replaceAct = createIconAction(":/icons/Edit/replace.svg", tr("Replace..."), QKeySequence("Ctrl+H"), [this]() { emit replaceTriggered(); });
    m_findInFilesAct = createIconAction(":/icons/Edit/findinfiles.svg", tr("Find In Files..."), QKeySequence("Ctrl+Shift+F"), [this]() { emit findInFilesTriggered(); });
    m_goToAct = createIconAction(":/icons/Edit/goto.svg", tr("Go To Line..."), QKeySequence("Ctrl+G"), [this]() { emit goToLineTriggered(); });
    m_zoomInAct = createIconAction(":/icons/View/zoomin.svg", tr("Zoom In"), QKeySequence("Ctrl+="), [this]() { emit zoomInTriggered(); });
    m_zoomOutAct = createIconAction(":/icons/View/zoomout.svg", tr("Zoom Out"), QKeySequence("Ctrl+-"), [this]() { emit zoomOutTriggered(); });
    m_zoomResetAct = createIconAction(":/icons/View/zoomreset.svg", tr("Reset Zoom"), QKeySequence("Ctrl+0"), [this]() { emit zoomResetTriggered(); });
    m_fullScreenAct = createIconAction(":/icons/View/fullscreen.svg", tr("Fullscreen"), QKeySequence("F11"), [this]() { emit toggleFullScreenTriggered(); });
    m_projectPanelAct = new QAction(QIcon(":/icons/Common/folder.svg"), tr("Project Panel"), this);
    m_projectPanelAct->setCheckable(true);
    m_projectPanelAct->setChecked(false);
    m_projectPanelAct->setShortcut(QKeySequence("Ctrl+B"));
    connect(m_projectPanelAct, &QAction::triggered, this, [this]() { emit toggleProjectPanelTriggered(); });
    m_terminalPanelAct = new QAction(QIcon(":/icons/View/terminal.svg"), tr("Terminal Panel"), this);
    m_terminalPanelAct->setCheckable(true);
    m_terminalPanelAct->setChecked(false);
    m_terminalPanelAct->setShortcut(QKeySequence("Ctrl+`"));
    connect(m_terminalPanelAct, &QAction::triggered, this, [this]() { emit toggleTerminalPanelTriggered(); });
    m_toolBarAct = new QAction(QIcon(":/icons/View/ui.svg"), tr("Toolbar\tCtrl+Alt+T"), this);
    m_toolBarAct->setCheckable(true);
    connect(m_toolBarAct, &QAction::triggered, this, [this]() { emit toggleToolBarTriggered(); });
    m_statusBarAct = new QAction(QIcon(":/icons/View/ui.svg"), tr("Status Bar\tCtrl+Shift+Alt+S"), this);
    m_statusBarAct->setCheckable(true);
    connect(m_statusBarAct, &QAction::triggered, this, [this]() { emit toggleStatusBarTriggered(); });
    m_wordWrapAct = new QAction(tr("Word Wrap"), this);
    m_wordWrapAct->setCheckable(true);
    m_wordWrapAct->setShortcut(QKeySequence("Ctrl+E"));
    connect(m_wordWrapAct, &QAction::triggered, this, [this]() { emit toggleWordWrapTriggered(); });
    m_findNextAct = createIconAction(":/icons/Edit/find.svg", tr("Find Next"), QKeySequence("F3"), [this]() { emit findNextTriggered(); });
    m_findPrevAct = createIconAction(":/icons/Edit/find.svg", tr("Find Previous"), QKeySequence("Shift+F3"), [this]() { emit findPreviousTriggered(); });
    m_aboutAct = createIconAction(":/icons/Help/about.svg", tr("About..."), QKeySequence(), [this]() { emit aboutTriggered(); });
    m_optionsAct = createIconAction(":/icons/Tools/options.svg", tr("Options..."), QKeySequence(), [this]() { emit optionsTriggered(); });
    m_printAct = createIconAction(":/icons/File/print.svg", tr("Print..."), QKeySequence::Print, [this]() { emit printFileTriggered(); });
    m_printPreviewAct = createIconAction(":/icons/File/printpreview.svg", tr("Print Preview..."), QKeySequence(), [this]() { emit printPreviewTriggered(); });
    m_readOnlyAct = new QAction(QIcon(":/icons/File/readonly.svg"), tr("Read Only"), this);
    m_readOnlyAct->setCheckable(true);
    m_readOnlyAct->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(m_readOnlyAct, &QAction::triggered, this, [this]() { emit toggleReadOnlyTriggered(); });
    m_toggleBookmarkAct = new QAction(tr("Toggle Bookmark"), this);
    m_toggleBookmarkAct->setShortcut(QKeySequence("Ctrl+F2"));
    connect(m_toggleBookmarkAct, &QAction::triggered, this, [this]() { emit toggleBookmarkTriggered(); });
    m_nextBookmarkAct = new QAction(tr("Next Bookmark"), this);
    m_nextBookmarkAct->setShortcut(QKeySequence("F2"));
    connect(m_nextBookmarkAct, &QAction::triggered, this, [this]() { emit nextBookmarkTriggered(); });
    m_prevBookmarkAct = new QAction(tr("Previous Bookmark"), this);
    m_prevBookmarkAct->setShortcut(QKeySequence("Shift+F2"));
    connect(m_prevBookmarkAct, &QAction::triggered, this, [this]() { emit prevBookmarkTriggered(); });
    m_clearBookmarksAct = new QAction(tr("Clear All Bookmarks"), this);
    m_clearBookmarksAct->setShortcut(QKeySequence("Ctrl+Shift+F2"));
    connect(m_clearBookmarksAct, &QAction::triggered, this, [this]() { emit clearBookmarksTriggered(); });
    m_menuBarAct = new QAction(QIcon(":/icons/View/ui.svg"), tr("Menu Bar\tCtrl+Alt+M"), this);
    m_menuBarAct->setCheckable(true);
    connect(m_menuBarAct, &QAction::triggered, this, [this]() { emit toggleMenuBarTriggered(); });

}

void ActionManager::buildMenus(QMenuBar *menuBar) {
    QMenu *devpadMenu = menuBar->addMenu(tr("Devpad"));
    devpadMenu->addAction(m_quitDevpadAct);
    
    QAction *sep = new QAction(tr("|"), this);
    sep->setEnabled(false);
    menuBar->addAction(sep);
    
    QMenu *fileMenu = menuBar->addMenu(tr("File"));
    buildFileMenu(fileMenu);

    QMenu *editMenu = menuBar->addMenu(tr("Edit"));
    buildEditMenu(editMenu);

    QMenu *viewMenu = menuBar->addMenu(tr("View"));
    buildViewMenu(viewMenu);

    QMenu *toolsMenu = menuBar->addMenu(tr("Tools"));
    buildToolsMenu(toolsMenu);

    QMenu *helpMenu = menuBar->addMenu(tr("Help"));
    buildHelpMenu(helpMenu);
}

void ActionManager::buildFileMenu(QMenu *fileMenu) {
    fileMenu->addAction(m_newAct);
    fileMenu->addAction(m_newWindowAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_openAct);
    fileMenu->addAction(m_openFolderAct);
    fileMenu->addAction(m_openRemoteAct);
    fileMenu->addSeparator();

    m_recentFilesMenu = fileMenu->addMenu(QIcon(":/icons/File/recentfiles.svg"), tr("Recent Files"));
    for (int i = 0; i < 10; ++i) {
        m_recentFileActs[i] = new QAction(this);
        m_recentFileActs[i]->setVisible(false);
        connect(m_recentFileActs[i], &QAction::triggered, this, [this, i]() {
            if (m_recentFileActs[i]) {
                emit openRecentFileTriggered(m_recentFileActs[i]->data().toString());
            }
        });
        m_recentFilesMenu->addAction(m_recentFileActs[i]);
    }
    m_recentFilesMenu->addSeparator();
    m_clearRecentFilesAct = new QAction(QIcon(":/icons/Common/clear.svg"), tr("Clear Recent Files"), this);
    connect(m_clearRecentFilesAct, &QAction::triggered, this, [this]() { emit clearRecentFilesTriggered(); });
    m_recentFilesMenu->addAction(m_clearRecentFilesAct);

    fileMenu->addSeparator();
    fileMenu->addAction(m_saveAct);
    fileMenu->addAction(m_saveAsAct);
    fileMenu->addAction(m_saveAllAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_revertAct);
    fileMenu->addSeparator();

    fileMenu->addSeparator();
    QMenu *propertiesMenu = fileMenu->addMenu(QIcon(":/icons/File/properties.svg"), tr("Properties"));
    propertiesMenu->addAction(m_readOnlyAct);
    propertiesMenu->addSeparator();
    m_reopenEncodingMenu = propertiesMenu->addMenu(tr("Reopen with Encoding"));
    m_saveEncodingMenu = propertiesMenu->addMenu(tr("Save with Encoding"));
    fileMenu->addSeparator();
    fileMenu->addAction(m_printAct);
    fileMenu->addAction(m_printPreviewAct);
    fileMenu->addSeparator();
    fileMenu->addAction(m_closeAct);
    fileMenu->addAction(m_closeAllAct);
    fileMenu->addAction(m_exitAct);
}

void ActionManager::buildEditMenu(QMenu *editMenu) {
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
    editMenu->addAction(m_findNextAct);
    editMenu->addAction(m_findPrevAct);
    editMenu->addAction(m_goToAct);
    editMenu->addSeparator();
    QMenu *bookmarksMenu = editMenu->addMenu(tr("Bookmarks"));
    bookmarksMenu->addAction(m_toggleBookmarkAct);
    bookmarksMenu->addAction(m_nextBookmarkAct);
    bookmarksMenu->addAction(m_prevBookmarkAct);
    bookmarksMenu->addSeparator();
    bookmarksMenu->addAction(m_clearBookmarksAct);
}

void ActionManager::buildViewMenu(QMenu *viewMenu) {
    viewMenu->addAction(m_zoomInAct);
    viewMenu->addAction(m_zoomOutAct);
    viewMenu->addAction(m_zoomResetAct);
    viewMenu->addSeparator();
    viewMenu->addAction(m_projectPanelAct);
    viewMenu->addAction(m_terminalPanelAct);
    viewMenu->addSeparator();
    viewMenu->addAction(m_menuBarAct);
    viewMenu->addAction(m_toolBarAct);
    viewMenu->addAction(m_statusBarAct);
    viewMenu->addSeparator();
    viewMenu->addAction(m_fullScreenAct);
}

void ActionManager::buildToolsMenu(QMenu *toolsMenu) {
    m_toolsMenu = toolsMenu;
    rebuildExternalToolsMenu();
    toolsMenu->addSeparator();
    toolsMenu->addAction(m_optionsAct);
}

void ActionManager::rebuildExternalToolsMenu() {
    if (!m_toolsMenu) return;

    for (QAction* act : m_externalToolActs) {
        m_toolsMenu->removeAction(act);
        delete act;
    }
    m_externalToolActs.clear();

    int count = SettingsManager::instance().externalToolCount();
    for (int i = 0; i < count; ++i) {
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
    }

    if (count > 0)
        m_toolsMenu->addSeparator();

    QAction* configureAct = new QAction(tr("External Tools..."), this);
    connect(configureAct, &QAction::triggered, this, [this]() { emit configureExternalToolsTriggered(); });
    m_toolsMenu->addAction(configureAct);
    m_externalToolActs.append(configureAct);
}

void ActionManager::buildHelpMenu(QMenu *helpMenu) {
    helpMenu->addAction(m_aboutAct);
}

QToolBar* ActionManager::buildToolBar() {
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

void ActionManager::buildStatusBar(QStatusBar *statusBar, ProjectPanel *, TerminalPanel *) {
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
    m_projectPanelButton->setStyleSheet("QToolButton { border: none; padding: 2px 4px; } QToolButton:checked { background-color: palette(highlight); }");
    connect(m_projectPanelButton, &QToolButton::clicked, this, [this]() { emit toggleProjectPanelTriggered(); });
    statusBar->addWidget(m_projectPanelButton);

    m_terminalPanelButton = new QToolButton(statusBar);
    m_terminalPanelButton->setIcon(QIcon(":/icons/View/terminal.svg"));
    m_terminalPanelButton->setCheckable(true);
    m_terminalPanelButton->setCursor(Qt::PointingHandCursor);
    m_terminalPanelButton->setStyleSheet("QToolButton { border: none; padding: 2px 4px; } QToolButton:checked { background-color: palette(highlight); }");
    connect(m_terminalPanelButton, &QToolButton::clicked, this, [this]() { emit toggleTerminalPanelTriggered(); });
    statusBar->addWidget(m_terminalPanelButton);
}
