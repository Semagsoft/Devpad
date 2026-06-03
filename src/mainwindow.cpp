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
#include "mainwindow.h"

#include "aboutdialog.h"
#include "actionmanager.h"
#include "backupmanager.h"
#include "codeeditor.h"
#include "editorcontroller.h"
#include "encodingmanager.h"
#include "encodingutils.h"
#include "filemanager.h"
#include "filewatchermanager.h"
#include "finddialog.h"
#include "findinfilesdialog.h"
#include "logger.h"
#include "optionsdialog.h"
#include "printmanager.h"
#include "projectpanel.h"
#include "remotefileservice.h"
#include "replacedialog.h"
#include "searchmanager.h"
#include "sessionmanager.h"
#include "settingsmanager.h"
#include "splitview.h"
#include "tabmanager.h"
#include "terminalpanel.h"
#include "theme.h"
#include "themeapplier.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QProcess>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTabBar>
#include <QTextDocument>
#include <QToolButton>

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(Strings::AppName);
    setMinimumSize(800, 600);
    QIcon windowIcon(":/icons/devpad.svg");
    if (!windowIcon.isNull())
    {
        setWindowIcon(windowIcon);
    }
    setAcceptDrops(true);

    splitView = new SplitView(this);
    tabWidget = splitView->primaryTabWidget();
    tabWidget->setDocumentMode(true);
    tabWidget->tabBar()->setExpanding(false);

    splitView->setPaneCallbacks([this](QTabWidget* pane) { tabManager->addPane(pane); }, [this](QTabWidget* pane) { tabManager->removePane(pane); },
                                [this](QTabWidget* pane, int index)
                                { tabManager->updateCloseButton(index, pane, SettingsManager::instance().closeButtonMode()); });

    connect(splitView, &SplitView::activeTabWidgetChanged, this,
            [this](QTabWidget* tw)
            {
                tabManager->setActivePane(tw);
                updateStatusBar();
                editorController->updateUndoRedoState();
                updateFileTypeLabel();
                updateStatusBarLabelsVisibility();
                updateEncodingSelector();
                editorController->updateReadOnlyActionState();
            });

    connect(splitView, &SplitView::tabDetachedToWindow, this,
            [](const QString& filePath)
            {
                QString appPath = QApplication::applicationFilePath();
                if (!filePath.isEmpty() && filePath[0] == QChar(1))
                {
                    QString realPath = filePath.mid(1);
                    if (!QProcess::startDetached(appPath, QStringList() << "--transfer" << realPath))
                    {
                        Logger::instance().error(QString("Failed to start detached process for: %1").arg(realPath));
                    }
                }
                else
                {
                    if (!QProcess::startDetached(appPath, QStringList() << filePath))
                    {
                        Logger::instance().error(QString("Failed to start detached process for: %1").arg(filePath));
                    }
                }
            });

    projectPanel = new ProjectPanel(this);
    {
        auto pos = SettingsManager::instance().projectPanelPosition();
        Qt::DockWidgetArea area = (pos == ProjectPanelPosition::Right)
            ? Qt::RightDockWidgetArea : Qt::LeftDockWidgetArea;
        addDockWidget(area, projectPanel);
    }
    projectPanel->hide();

    terminalPanel = new TerminalPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, terminalPanel);
    terminalPanel->hide();

    tabManager = new TabManager(tabWidget, this);
    searchManager = new SearchManager(this);

    actionManager = new ActionManager(this);
    sessionManager = new SessionManager(this);
    encodingManager = new EncodingManager(this);
    themeApplier = new ThemeApplier(this);
    printManager = new PrintManager(this);
    fileManager = new FileManager(this);
    fileWatcherManager = new FileWatcherManager(this);

    editorController = new EditorController(tabManager, fileManager, fileWatcherManager, actionManager, tabWidget, this);

    m_remoteFileService = new RemoteFileService(this);

    connect(fileWatcherManager, &FileWatcherManager::fileModifiedExternally, this, &MainWindow::onFileModifiedExternally);

    connect(m_remoteFileService, &RemoteFileService::fileDownloaded, this,
            [this](const QString& url, const QString& fileName, const QByteArray& data)
            {
                statusBar()->clearMessage();
                openRemoteFileFromData(url, fileName, data);
            });
    connect(m_remoteFileService, &RemoteFileService::downloadFailed, this,
            [this](const QString& url, const QString& error)
            {
                statusBar()->clearMessage();
                QMessageBox::warning(this, tr("Download Failed"), tr("Failed to download \"%1\":\n%2").arg(url, error));
            });
    connect(m_remoteFileService, &RemoteFileService::downloadProgress, this,
            [this](const QString&, qint64 received, qint64 total)
            {
                if (total > 0)
                {
                    statusBar()->showMessage(tr("Downloading... %1%").arg(static_cast<int>(received * 100 / total)));
                }
            });
    connect(m_remoteFileService, &RemoteFileService::statusMessage, this, [this](const QString& message) { statusBar()->showMessage(message); });

    setupUI();
    connectActions();
    connectPanelSignals();

    setCentralWidget(splitView);

    actionManager->toolBarAct()->setChecked(SettingsManager::instance().showToolbar());
    actionManager->statusBarAct()->setChecked(SettingsManager::instance().showStatusbar());
    addToolBar(actionManager->toolBar());
    actionManager->toolBar()->setVisible(SettingsManager::instance().showToolbar());
    statusBar()->setVisible(SettingsManager::instance().showStatusbar());

    actionManager->terminalPanelAct()->setChecked(SettingsManager::instance().showTerminalPanel());
    actionManager->terminalPanelButton()->setChecked(SettingsManager::instance().showTerminalPanel());

    applyStartupMode();
    updateRecentFileActions();

    if (SettingsManager::instance().showTerminalPanel())
    {
        terminalPanel->toggle(tabWidget, this);
        actionManager->terminalPanelAct()->setChecked(true);
        actionManager->terminalPanelButton()->setChecked(true);
    }
    else
    {
        terminalPanel->applyPosition(SettingsManager::instance().terminalPanelPosition(), tabWidget, this);
    }

    applySettings();

    connect(tabManager, &TabManager::currentChanged, this, &MainWindow::onTabChanged);
    connect(editorController, &EditorController::editorConnected, this,
            [this](CodeEditor* editor)
            {
                connect(editor, &QsciScintilla::textChanged, this, &MainWindow::updateStatusBar);
                connect(editor, &QsciScintilla::cursorPositionChanged, this, [this]() { updateStatusBar(); });
            });

    autoSaveTimer = new QTimer(this);
    connect(autoSaveTimer, &QTimer::timeout, editorController, &EditorController::autoSave);
    applyAutoSaveSettings();
    updateSplitViewVisibility();
}

void MainWindow::setupUI()
{
    actionManager->createActions();
    actionManager->buildMenus(menuBar());
    actionManager->buildToolBar();
    actionManager->buildStatusBar(statusBar(), projectPanel, terminalPanel);

    actionManager->toolBarAct()->setChecked(SettingsManager::instance().showToolbar());
    actionManager->statusBarAct()->setChecked(SettingsManager::instance().showStatusbar());
}

void MainWindow::connectActions()
{
    connect(actionManager, &ActionManager::newFileTriggered, editorController, &EditorController::newFile);
    connect(actionManager, &ActionManager::newWindowTriggered, this, &MainWindow::newWindow);
    connect(actionManager, &ActionManager::openFileTriggered, this, &MainWindow::openFile);
    connect(actionManager, &ActionManager::openRemoteTriggered, this, &MainWindow::openRemote);
    connect(actionManager, &ActionManager::openFolderTriggered, this, &MainWindow::openFolder);
    connect(actionManager, &ActionManager::saveFileTriggered, editorController, &EditorController::saveFile);
    connect(actionManager, &ActionManager::saveFileAsTriggered, editorController, &EditorController::saveFileAs);
    connect(actionManager, &ActionManager::saveAllTriggered, editorController, &EditorController::saveAll);
    connect(actionManager, &ActionManager::revertTriggered, this, &MainWindow::revertFile);
    connect(actionManager, &ActionManager::closeCurrentTabTriggered, editorController, &EditorController::closeCurrentTab);
    connect(actionManager, &ActionManager::closeAllTabsTriggered, editorController, &EditorController::closeAllTabs);
    connect(actionManager, &ActionManager::exitTriggered, this, &QWidget::close);

    connect(actionManager, &ActionManager::undoTriggered, editorController, &EditorController::undo);
    connect(actionManager, &ActionManager::redoTriggered, editorController, &EditorController::redo);
    connect(actionManager, &ActionManager::cutTriggered, editorController, &EditorController::cut);
    connect(actionManager, &ActionManager::copyTriggered, editorController, &EditorController::copy);
    connect(actionManager, &ActionManager::pasteTriggered, editorController, &EditorController::paste);
    connect(actionManager, &ActionManager::selectAllTriggered, editorController, &EditorController::selectAll);
    connect(actionManager, &ActionManager::deleteTriggered, editorController, &EditorController::deleteText);
    connect(actionManager, &ActionManager::findTriggered, this, &MainWindow::find);
    connect(actionManager, &ActionManager::replaceTriggered, this, &MainWindow::replace);
    connect(actionManager, &ActionManager::findInFilesTriggered, this, &MainWindow::findInFiles);
    connect(actionManager, &ActionManager::goToLineTriggered, this, &MainWindow::goToLine);
    connect(actionManager, &ActionManager::findNextTriggered, this, &MainWindow::findNext);
    connect(actionManager, &ActionManager::findPreviousTriggered, this, &MainWindow::findPrevious);

    connect(actionManager, &ActionManager::zoomInTriggered, editorController, &EditorController::zoomIn);
    connect(actionManager, &ActionManager::zoomOutTriggered, editorController, &EditorController::zoomOut);
    connect(actionManager, &ActionManager::zoomResetTriggered, editorController, &EditorController::zoomReset);
    connect(actionManager, &ActionManager::toggleFullScreenTriggered, this, &MainWindow::toggleFullScreen);
    connect(actionManager, &ActionManager::toggleProjectPanelTriggered, this, &MainWindow::toggleProjectPanel);
    connect(actionManager, &ActionManager::toggleTerminalPanelTriggered, this, &MainWindow::toggleTerminalPanel);
    connect(actionManager, &ActionManager::toggleReadOnlyTriggered, editorController, &EditorController::toggleReadOnly);
    connect(actionManager, &ActionManager::toggleBookmarkTriggered, editorController, &EditorController::toggleBookmark);
    connect(actionManager, &ActionManager::nextBookmarkTriggered, editorController, &EditorController::nextBookmark);
    connect(actionManager, &ActionManager::prevBookmarkTriggered, editorController, &EditorController::prevBookmark);
    connect(actionManager, &ActionManager::clearBookmarksTriggered, editorController, &EditorController::clearBookmarks);
    connect(actionManager, &ActionManager::toggleToolBarTriggered, this,
            [this]()
            {
                bool visible = actionManager->toolBar()->isVisible();
                actionManager->toolBar()->setVisible(!visible);
                actionManager->toolBarAct()->setChecked(!visible);
                SettingsManager::instance().setShowToolbar(!visible);
            });
    connect(actionManager, &ActionManager::toggleStatusBarTriggered, this,
            [this]()
            {
                bool visible = statusBar()->isVisible();
                statusBar()->setVisible(!visible);
                actionManager->statusBarAct()->setChecked(!visible);
                SettingsManager::instance().setShowStatusbar(!visible);
            });
    connect(actionManager, &ActionManager::toggleWordWrapTriggered, editorController, &EditorController::toggleWordWrap);
    connect(actionManager, &ActionManager::printFileTriggered, this, &MainWindow::printFile);
    connect(actionManager, &ActionManager::printPreviewTriggered, this, &MainWindow::printPreview);
    connect(actionManager, &ActionManager::optionsTriggered, this, &MainWindow::showOptions);
    connect(actionManager, &ActionManager::aboutTriggered, this, &MainWindow::showAbout);

    connect(actionManager, &ActionManager::openRecentFileTriggered, this,
            [this](const QString& filePath)
            {
                if (!filePath.isEmpty() && QFile::exists(filePath))
                {
                    loadFile(filePath);
                }
            });
    connect(actionManager, &ActionManager::clearRecentFilesTriggered, this,
            [this]()
            {
                SettingsManager::instance().clearRecentFiles();
                updateRecentFileActions();
            });

    connect(encodingManager, &EncodingManager::encodingSelected, this,
            [this](const QString& encoding, bool reopen)
            {
                if (reopen)
                {
                    reloadCurrentFileWithEncoding(encoding);
                }
                else
                {
                    saveCurrentFileWithEncoding(encoding);
                }
            });

    connect(editorController, &EditorController::fileSaved, this, [this](const QString&) { updateRecentFileActions(); });

    encodingManager->populateEncodingMenu(actionManager->reopenEncodingMenu(), true);
    encodingManager->populateEncodingMenu(actionManager->saveEncodingMenu(), false);
}

void MainWindow::connectPanelSignals()
{
    connect(tabManager, &TabManager::editorCreated, this, &MainWindow::onEditorCreated);
    connect(tabManager, &TabManager::editorClosed, this, [this](CodeEditor*) { updateSplitViewVisibility(); });
    connect(tabManager, &TabManager::tabCloseRequested, this,
            [this](int index)
            {
                if (editorController->onTabCloseRequested(index))
                {
                    updateSplitViewVisibility();
                    updateStatusBarLabelsVisibility();
                    editorController->updateReadOnlyActionState();
                }
            });
    connect(projectPanel, &ProjectPanel::fileDoubleClicked, this, &MainWindow::openFileFromProject);
    connect(projectPanel, &ProjectPanel::folderSelected, this, &MainWindow::openFolderFromPath);

    connect(projectPanel, &QDockWidget::visibilityChanged, actionManager->projectPanelAct(), &QAction::setChecked);
    connect(projectPanel, &QDockWidget::visibilityChanged, actionManager->projectPanelButton(), &QToolButton::setChecked);
    connect(terminalPanel, &QDockWidget::visibilityChanged, this,
            [this](bool visible) {
                if (SettingsManager::instance().terminalPanelPosition() != TerminalPanelPosition::Tab) {
                    actionManager->terminalPanelAct()->setChecked(visible);
                    actionManager->terminalPanelButton()->setChecked(visible);
                    SettingsManager::instance().setShowTerminalPanel(visible);
                }
            });
}

void MainWindow::applyCloseButtonPosition()
{
    tabManager->updateAllCloseButtons(SettingsManager::instance().closeButtonMode());
    for (QTabWidget* pane : splitView->findChildren<QTabWidget*>())
    {
        QTabBar* tabBar = pane->tabBar();
        if (tabBar)
        {
            tabBar->setTabsClosable(false);
        }
        pane->setStyleSheet("");
    }
}

void MainWindow::applyTabBarPosition()
{
    TabBarPosition position = SettingsManager::instance().tabBarPosition();
    QTabWidget::TabPosition tp = position == TabBarPosition::Top ? QTabWidget::North : QTabWidget::South;
    for (QTabWidget* pane : tabManager->panes())
    {
        pane->setTabPosition(tp);
    }
}

void MainWindow::applyTerminalPanelPosition()
{
    terminalPanel->applyPosition(SettingsManager::instance().terminalPanelPosition(), tabWidget, this);
    bool visible = SettingsManager::instance().showTerminalPanel();
    actionManager->terminalPanelAct()->setChecked(visible);
    actionManager->terminalPanelButton()->setChecked(visible);
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(), Strings::fileFilter());
    if (!fileName.isEmpty())
    {
        loadFile(fileName);
    }
}

void MainWindow::openRemoteFileFromData(const QString& urlStr, const QString& fileName, const QByteArray& data)
{
    QString detectedEnc = FileManager::detectEncoding(data);
    auto encOpt = QStringConverter::encodingForName(detectedEnc.toUtf8());
    QStringDecoder decoder(encOpt.value_or(QStringConverter::Utf8));
    QString content = decoder(data);
    statusBar()->clearMessage();

    QString displayName = fileName.isEmpty() ? QUrl(urlStr).fileName() : fileName;
    if (displayName.isEmpty())
    {
        displayName = tr("Remote");
    }

    CodeEditor* editor = tabManager->createEditor();
    editor->setFileName(urlStr);
    editor->setText(content);
    editor->setModified(false);

    editorController->connectEditorSignals(editor);

    SettingsManager::instance().applyToEditor(editor);

    QString syntax = getSyntaxForFile(displayName);
    if (!syntax.isEmpty())
    {
        editor->setSyntax(syntax);
    }

    tabManager->addEditor(editor, displayName);
    editorController->updateUndoRedoState();
    updateFileTypeLabel();

    Logger::instance().info(QString("Opened remote file: %1").arg(urlStr));
}

void MainWindow::openRemote()
{
    bool ok;
    QString urlStr =
        QInputDialog::getText(this, tr("Open Remote"), tr("Enter remote file URL\n(HTTP/HTTPS or SSH):"), QLineEdit::Normal, "https://", &ok);
    if (!ok || urlStr.trimmed().isEmpty())
        return;

    urlStr = urlStr.trimmed();

    bool isSsh = urlStr.startsWith("ssh://", Qt::CaseInsensitive);

    if (!isSsh && !urlStr.startsWith("http://", Qt::CaseInsensitive) && !urlStr.startsWith("https://", Qt::CaseInsensitive))
    {
        urlStr = "https://" + urlStr;
    }

    QUrl url(urlStr);
    if (!url.isValid())
    {
        QMessageBox::warning(this, tr("Invalid URL"), tr("The URL \"%1\" is not valid.").arg(urlStr));
        return;
    }

    m_remoteFileService->openRemote(urlStr);
}

void MainWindow::openFileFromPath(const QString& filePath)
{
    loadFile(filePath);
}

void MainWindow::openFolder()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, tr("Open Folder"));
    if (dirPath.isEmpty())
        return;

    projectPanel->setRootPath(dirPath);
    projectPanel->show();
    actionManager->projectPanelAct()->setChecked(true);
    terminalPanel->setWorkingDirectory(dirPath);
    Logger::instance().info(QString("Opened folder: %1").arg(dirPath));
}

void MainWindow::openFileFromProject(const QString& filePath)
{
    loadFile(filePath);
}

void MainWindow::openFolderFromPath(const QString& folderPath)
{
    projectPanel->setRootPath(folderPath);
    projectPanel->show();
    actionManager->projectPanelAct()->setChecked(true);
    terminalPanel->setWorkingDirectory(folderPath);
    Logger::instance().info(QString("Opened folder: %1").arg(folderPath));
}

void MainWindow::loadFile(const QString& fileName, const QString& encoding)
{
    CodeEditor* existingEditor = tabManager->findEditorByFileName(fileName);
    if (existingEditor)
    {
        for (QTabWidget* pane : tabManager->panes())
        {
            int index = pane->indexOf(existingEditor);
            if (index >= 0)
            {
                pane->setCurrentIndex(index);
                existingEditor->setFocus();
                tabManager->setActivePane(pane);
                return;
            }
        }
        return;
    }

    CodeEditor* editor = tabManager->createEditor();

    if (!fileManager->loadFile(fileName, editor, encoding))
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file: ") + fileName);
        editor->setParent(nullptr);
        editor->deleteLater();
        return;
    }

    SettingsManager::instance().applyToEditor(editor);

    QString syntax = getSyntaxForFile(fileName);
    if (!syntax.isEmpty())
    {
        editor->setSyntax(syntax);
    }

    editorController->connectEditorSignals(editor);

    QFileInfo fileInfo(fileName);
    if (!fileInfo.isWritable())
    {
        editor->setReadOnlyMode(true);
    }

    tabManager->addEditor(editor, QFileInfo(fileName).fileName());
    fileWatcherManager->watchFile(fileName);
    SettingsManager::instance().addRecentFile(fileName);
    updateRecentFileActions();
    updateEncodingSelector();
    editorController->updateUndoRedoState();
    updateFileTypeLabel();
    Logger::instance().info(QString("Opened file: %1").arg(fileName));
    editorController->promptBackupRestore(fileName);
}

void MainWindow::newWindow()
{
    QString appPath = QApplication::applicationFilePath();
    if (!QProcess::startDetached(appPath, QStringList()))
    {
        Logger::instance().error("Failed to start detached process for new window");
    }
    Logger::instance().info("Opened new window");
}

void MainWindow::find()
{
    if (auto* editor = tabManager->currentEditor())
    {
        searchManager->setEditor(editor);
    }
    searchManager->showFindDialog();
}

void MainWindow::replace()
{
    if (auto* editor = tabManager->currentEditor())
    {
        searchManager->setEditor(editor);
    }
    searchManager->showReplaceDialog();
}

void MainWindow::goToLine()
{
    CodeEditor* editor = tabManager->currentEditor();
    if (!editor)
        return;
    int lineCount = editor->lines();
    if (lineCount < 1)
        lineCount = 1;
    bool ok;
    int line = QInputDialog::getInt(this, tr("Go To Line"), tr("Line number:"), 1, 1, lineCount, 1, &ok);
    if (ok)
    {
        editor->setCursorPosition(line - 1, 0);
    }
}

void MainWindow::findNext()
{
    if (auto* editor = tabManager->currentEditor())
    {
        searchManager->setEditor(editor);
    }
    searchManager->findNext();
}

void MainWindow::findPrevious()
{
    if (auto* editor = tabManager->currentEditor())
    {
        searchManager->setEditor(editor);
    }
    searchManager->findPrevious();
}

void MainWindow::findInFiles()
{
    if (!findInFilesDialog)
    {
        QString defaultDir;
        if (projectPanel->isVisible())
            defaultDir = projectPanel->rootPath();
        else
            defaultDir = QDir::homePath();

        findInFilesDialog = new FindInFilesDialog(defaultDir, this);
        connect(findInFilesDialog, &FindInFilesDialog::navigateToResult, this,
                [this](const QString& filePath, int lineNumber)
                {
                    CodeEditor* existing = tabManager->findEditorByFileName(filePath);
                    if (existing)
                    {
                        for (QTabWidget* pane : tabManager->panes())
                        {
                            int idx = pane->indexOf(existing);
                            if (idx >= 0)
                            {
                                pane->setCurrentIndex(idx);
                                tabManager->setActivePane(pane);
                                break;
                            }
                        }
                        existing->setCursorPosition(lineNumber - 1, 0);
                        existing->setFocus();
                        return;
                    }

                    loadFile(filePath);
                    CodeEditor* editor = tabManager->currentEditor();
                    if (editor)
                    {
                        editor->setCursorPosition(lineNumber - 1, 0);
                    }
                });
        connect(findInFilesDialog, &QDialog::finished, this,
                [this]()
                {
                    findInFilesDialog->deleteLater();
                    findInFilesDialog = nullptr;
                });
    }

    if (auto* editor = tabManager->currentEditor())
    {
        QString selected = editor->selectedText();
        if (!selected.isEmpty())
            findInFilesDialog->startSearch(selected, projectPanel->isVisible() ? projectPanel->rootPath() : QDir::homePath());
    }

    findInFilesDialog->show();
    findInFilesDialog->raise();
    findInFilesDialog->activateWindow();
}

void MainWindow::showOptions()
{
    OptionsDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted)
    {
        applySettings();
        applyCloseButtonPosition();
        applyTabBarPosition();
        applyTerminalPanelPosition();
        tabManager->updateTabBarVisibility();
        updateRecentFileActions();
        applyAutoSaveSettings();
    }
}

void MainWindow::applySettings()
{
    themeApplier->applyTheme(this);
    themeApplier->applySettingsToAllEditors(tabManager);
}

void MainWindow::applyStartupMode()
{
    if (QApplication::arguments().size() > 1)
        return;

    StartupMode mode = SettingsManager::instance().startupMode();
    if (mode == StartupMode::NewFile)
    {
        editorController->newFile();
    }
    else if (mode == StartupMode::OpenLastFile)
    {
        QStringList recentFiles = SettingsManager::instance().recentFiles();
        if (!recentFiles.isEmpty())
        {
            QString lastFile = recentFiles.first();
            if (QFile::exists(lastFile))
            {
                loadFile(lastFile);
            }
        }
    }
    else if (mode == StartupMode::RestoreSession)
    {
        sessionManager->restoreSession([this](const QString& filePath) { loadFile(filePath); }, splitView, projectPanel);
        QString projectPath = projectPanel->rootPath();
        if (!projectPath.isEmpty())
        {
            terminalPanel->setWorkingDirectory(projectPath);
        }
    }
}

void MainWindow::updateStatusBar()
{
    CodeEditor* editor = tabManager->currentEditor();
    if (!editor)
        return;

    int line, index;
    editor->getCursorPosition(&line, &index);
    actionManager->lineColLabel()->setText(tr("Line: %1, Col: %2").arg(line + 1).arg(index + 1));
}

void MainWindow::updateFileTypeLabel()
{
    CodeEditor* editor = tabManager->currentEditor();
    if (!editor)
        return;

    QString syntax = editor->syntax();
    if (syntax.isEmpty())
    {
        syntax = tr("Plain Text");
    }
    else
    {
        static const QHash<QString, QString> displayNames = {
            {"cpp", tr("C++")},   {"c", tr("C")},           {"csharp", tr("C#")},
            {"java", tr("Java")}, {"python", tr("Python")}, {"javascript", tr("JavaScript")},
            {"html", tr("HTML")}, {"css", tr("CSS")},       {"xml", tr("XML")},
            {"sql", tr("SQL")},   {"bash", tr("Shell")},    {"cmake", tr("CMake")},
        };
        syntax = displayNames.value(syntax, syntax);
    }

    if (editor->isReadOnlyMode())
    {
        syntax += tr(" | Read Only");
    }

    actionManager->fileTypeLabel()->setText(syntax);
}

void MainWindow::onTabChanged(int index)
{
    Q_UNUSED(index)
    updateSplitViewVisibility();
    updateStatusBar();
    editorController->updateUndoRedoState();
    updateFileTypeLabel();
    updateStatusBarLabelsVisibility();
    updateEncodingSelector();
    editorController->updateReadOnlyActionState();
}

void MainWindow::onEditorCreated(CodeEditor* editor)
{
    Q_UNUSED(editor)
    updateSplitViewVisibility();
    editorController->updateUndoRedoState();
    updateStatusBarLabelsVisibility();
    editorController->updateReadOnlyActionState();
}

void MainWindow::updateRecentFileActions()
{
    QStringList recentFiles = SettingsManager::instance().recentFiles();
    int numRecentFiles = qMin(recentFiles.size(), (qsizetype)10);

    for (int i = 0; i < numRecentFiles; ++i)
    {
        QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(recentFiles[i]).fileName());
        actionManager->recentFileAct(i)->setText(text);
        actionManager->recentFileAct(i)->setData(recentFiles[i]);
        actionManager->recentFileAct(i)->setIcon(ProjectPanel::iconForFile(recentFiles[i]));
        actionManager->recentFileAct(i)->setVisible(true);
    }
    for (int i = numRecentFiles; i < 10; ++i)
    {
        actionManager->recentFileAct(i)->setVisible(false);
    }
    actionManager->recentFilesMenu()->setEnabled(numRecentFiles > 0);
}

void MainWindow::revertFile()
{
    CodeEditor* editor = tabManager->currentEditor();
    if (!editor)
        return;

    QString fileName = editor->fileName();
    if (fileName.isEmpty() || fileName == Strings::untitled())
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot revert an unsaved file."));
        return;
    }

    if (editor->isModified())
    {
        QMessageBox::StandardButton result =
            QMessageBox::question(this, tr("Revert"), tr("The file has unsaved changes.\n\nDo you want to discard them and reload from disk?"),
                                  QMessageBox::Yes | QMessageBox::No);
        if (result != QMessageBox::Yes)
            return;
    }

    int idx = tabManager->indexOf(editor);
    QString encoding = editor->encoding();
    QString syntax = editor->syntax();
    QList<int> bookmarks = editor->bookmarkLines();

    tabManager->closeEditor(idx);
    fileWatcherManager->unwatchFile(fileName);
    loadFile(fileName, encoding);
    if (auto* newEditor = tabManager->currentEditor())
    {
        newEditor->setBookmarks(bookmarks);
    }
}

void MainWindow::printFile()
{
    CodeEditor* editor = tabManager->currentEditor();
    if (!editor)
        return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        printManager->printEditorWithHighlighting(editor, &printer);
    }
}

void MainWindow::printPreview()
{
    CodeEditor* editor = tabManager->currentEditor();
    if (!editor)
        return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    QPointer<CodeEditor> editorPtr = editor;
    QPointer<MainWindow> thisPtr = this;
    connect(&preview, &QPrintPreviewDialog::paintRequested, this,
            [editorPtr, thisPtr](QPrinter* p)
            {
                if (editorPtr && thisPtr)
                {
                    thisPtr->printManager->printEditorWithHighlighting(editorPtr, p);
                }
            });
    preview.exec();
}

void MainWindow::toggleFullScreen()
{
    bool fullScreen = !isFullScreen();
    if (fullScreen)
        showFullScreen();
    else
        showNormal();
    actionManager->fullScreenAct()->setChecked(fullScreen);
}

void MainWindow::toggleProjectPanel()
{
    if (projectPanel->isVisible())
    {
        projectPanel->hide();
        actionManager->projectPanelAct()->setChecked(false);
        actionManager->projectPanelButton()->setChecked(false);
    }
    else
    {
        projectPanel->show();
        actionManager->projectPanelAct()->setChecked(true);
        actionManager->projectPanelButton()->setChecked(true);
    }
}

void MainWindow::toggleTerminalPanel()
{
    terminalPanel->toggle(tabWidget, this);
    bool visible = SettingsManager::instance().showTerminalPanel();
    actionManager->terminalPanelAct()->setChecked(visible);
    actionManager->terminalPanelButton()->setChecked(visible);
}

void MainWindow::updateStatusBarLabelsVisibility()
{
    bool hasEditor = tabManager->count() > 0;
    QTabWidget* activePane = tabManager->activePane();
    bool isTerminalTab = (activePane != nullptr) &&
                         (SettingsManager::instance().terminalPanelPosition() == TerminalPanelPosition::Tab) &&
                         (activePane->currentWidget() == terminalPanel);
    bool shouldShow = hasEditor && !isTerminalTab;
    actionManager->lineColLabel()->setVisible(shouldShow);
    actionManager->encodingComboBox()->setVisible(shouldShow);
    actionManager->fileTypeLabel()->setVisible(shouldShow);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    for (int i = tabManager->count() - 1; i >= 0; --i)
    {
        CodeEditor* editor = tabManager->editorAt(i);
        if (!editorController->maybeSave(editor))
        {
            event->ignore();
            return;
        }
    }
    SettingsManager::instance().setShowTerminalPanel(actionManager->terminalPanelAct()->isChecked());
    sessionManager->saveSession(tabManager, projectPanel);
    event->accept();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        actionManager->fullScreenAct()->setChecked(isFullScreen());
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    if (mimeData->hasUrls())
    {
        QList<QUrl> urlList = mimeData->urls();
        for (const QUrl& url : urlList)
        {
            QString fileName = url.toLocalFile();
            if (!fileName.isEmpty() && QFileInfo(fileName).isFile())
            {
                loadFile(fileName);
            }
        }
    }
}

void MainWindow::showAbout()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::updateEncodingSelector()
{
    encodingManager->updateEncodingSelector(actionManager->encodingComboBox(), tabManager->currentEditor());
}

void MainWindow::reloadCurrentFileWithEncoding(const QString& encoding)
{
    CodeEditor* editor = tabManager->currentEditor();
    if (!editor)
        return;

    QString fileName = editor->fileName();
    if (fileName.isEmpty() || fileName == Strings::untitled())
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot reopen an unsaved file."));
        return;
    }

    int idx = tabManager->indexOf(editor);
    QList<int> bookmarks = editor->bookmarkLines();
    if (!editorController->maybeSave(editor))
        return;

    if (idx >= 0)
        tabManager->closeEditor(idx);
    loadFile(fileName, encoding);
    if (auto* newEditor = tabManager->currentEditor())
    {
        newEditor->setBookmarks(bookmarks);
    }
}

void MainWindow::saveCurrentFileWithEncoding(const QString& encoding)
{
    CodeEditor* editor = tabManager->currentEditor();
    if (!editor)
        return;

    QString fileName = editor->fileName();
    if (fileName.isEmpty() || fileName == Strings::untitled())
    {
        bool saveAsResult = editorController->saveFileAs();
        if (saveAsResult)
        {
            editor = tabManager->currentEditor();
            if (editor)
            {
                editor->setEncoding(encoding);
                if (!fileManager->saveFile(editor->fileName(), editor))
                {
                    QMessageBox::warning(this, tr("Error"), tr("Cannot save file: ") + editor->fileName());
                }
                updateEncodingSelector();
            }
        }
        return;
    }

    editor->setEncoding(encoding);
    if (!fileManager->saveFile(fileName, editor))
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot save file: ") + fileName);
    }
    updateEncodingSelector();
    Logger::instance().info(QString("Saved file with encoding: %1 -> %2").arg(fileName, encoding));
}

void MainWindow::applyAutoSaveSettings()
{
    if (SettingsManager::instance().autoSaveEnabled())
    {
        autoSaveTimer->start(SettingsManager::instance().autoSaveInterval() * 1000);
    }
    else
    {
        autoSaveTimer->stop();
    }
}

QString MainWindow::getSyntaxForFile(const QString& displayName) const
{
    return SettingsManager::instance().syntaxForFile(displayName);
}

void MainWindow::onFileModifiedExternally(const QString& filePath)
{
    CodeEditor* editor = tabManager->findEditorByFileName(filePath);
    if (!editor)
        return;

    QString message;
    if (editor->isModified())
    {
        message = tr("The file \"%1\" has been modified outside of Devpad.\n\n"
                     "Your version has unsaved changes.\n\n"
                     "Do you want to reload the file? (Unsaved changes will be lost)");
    }
    else
    {
        message = tr("The file \"%1\" has been modified outside of Devpad.\n\n"
                     "Do you want to reload the file?");
    }

    QMessageBox::StandardButton result =
        QMessageBox::question(this, tr("File Modified"), message.arg(QFileInfo(filePath).fileName()), QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes)
    {
        QList<int> bookmarks = editor->bookmarkLines();
        QString encoding = editor->encoding();

        int idx = tabManager->indexOf(editor);
        if (idx >= 0)
        {
            tabManager->closeEditor(idx);
        }
        fileWatcherManager->unwatchFile(filePath);
        loadFile(filePath, encoding);
        if (auto* newEditor = tabManager->currentEditor())
        {
            newEditor->setBookmarks(bookmarks);
        }
    }
}

void MainWindow::updateSplitViewVisibility()
{
    QTabWidget* primary = splitView->primaryTabWidget();
    if (primary->count() == 0)
    {
        for (int i = 0; i < splitView->paneCount(); ++i)
        {
            QTabWidget* pane = splitView->paneAt(i);
            if (pane && pane != primary && pane->count() > 0)
            {
                for (int j = pane->count() - 1; j >= 0; --j)
                {
                    splitView->moveTabToPane(j, pane, primary);
                }
                break;
            }
        }
    }
    for (int i = splitView->paneCount() - 1; i >= 0; --i)
    {
        QTabWidget* pane = splitView->paneAt(i);
        if (pane && pane->count() == 0 && pane != primary)
        {
            splitView->removePane(pane);
        }
    }
    tabManager->updateTabBarVisibility();
}

void MainWindow::openTransferFile(const QString& filePath)
{
    loadFile(filePath);
    CodeEditor* editor = tabManager->currentEditor();
    if (editor && editor->fileName() == filePath)
    {
        fileWatcherManager->unwatchFile(filePath);
        editor->setFileName(Strings::untitled());
        editor->forceModified();
        tabManager->updateTabTitle(editor);
    }
    QFile::remove(filePath);
}
