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
#include "dialogs/dialogcoordinator.h"
#include "editorcontroller.h"
#include "encodingmanager.h"
#include "encodingutils.h"
#include "errorlistpanel.h"
#include "externaltoolmanager.h"
#include "externaltoolsdialog.h"
#include "filemanager.h"
#include "filewatchermanager.h"
#include "logger.h"
#include "lsp/lspclient.h"
#include "lsp/lspconnectionhelper.h"
#include "lsp/lspservermanager.h"
#include "lsp/lsptypes.h"
#include "managers/externalchangecoordinator.h"
#include "managers/remoteopencoordinator.h"
#include "optionsdialog.h"
#include "plugins/terminalplugin/terminalpanel.h"
#include "printmanager.h"
#include "projectpanel.h"
#include "referencespanel.h"
#include "remotefileservice.h"
#include "remotefsentry.h"
#include "searchmanager.h"
#include "sessionmanager.h"
#include "settingsmanager.h"
#include "snippetmanager.h"
#include "splitview.h"
#include "tabmanager.h"
#include "theme.h"
#include "themeapplier.h"
#include "palettechangefilter.h"
#include "theme.h"
#include "widgets/titlebar.h"
#ifdef Q_OS_WIN
#include "widgets/windowsframebridge.h"
#endif
#include "updatechecker.h"
#include "updatedialog.h"
#ifdef BUILD_PLUGINS
#include "plugins/pluginhost.h"
#include "plugins/pluginmanager.h"
#endif

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#ifdef Q_OS_MACOS
#include <QFileOpenEvent>
#endif
#include <QIcon>
#include <QInputDialog>
#include <QLocalSocket>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaMethod>
#include <QMetaObject>
#include <QMimeData>
#include <QPageSetupDialog>
#include <QPointer>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QProcess>
#include <QRegularExpression>
#include <QShortcut>
#include <QStatusBar>
#include <QTabBar>
#include <QTextDocument>
#include <QToolBar>
#include <QToolButton>
#ifndef Q_OS_WIN
// #include <QTermWidget>  // Removed - QTermWidget not available in Qt6
#endif

#ifdef Q_OS_MACOS
namespace
{
QString appBundlePath()
{
    const QString bundle = QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../.."));
    return QFileInfo::exists(bundle + QStringLiteral("/Contents/Info.plist")) ? bundle : QString();
}

void launchAppBundle(const QStringList& args)
{
    const QString bundle = appBundlePath();
    if (bundle.isEmpty())
    {
        if (!QProcess::startDetached(QCoreApplication::applicationFilePath(), args))
            Logger::instance().error(QString("Failed to start detached process: %1").arg(args.join(QLatin1Char(' '))));
        return;
    }
    QStringList openArgs;
    openArgs << QStringLiteral("-n") << bundle;
    if (!args.isEmpty())
        openArgs << QStringLiteral("--args") << args;
    if (!QProcess::startDetached(QStringLiteral("open"), openArgs))
        Logger::instance().error(QString("Failed to launch app bundle with: %1").arg(openArgs.join(QLatin1Char(' '))));
}
} // namespace
#endif

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    qApp->installEventFilter(this);

    setupWindow();

    m_splitView = new SplitView(this);
    m_tabWidget = m_splitView->primaryTabWidget();
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->tabBar()->setExpanding(false);

    m_splitView->setPaneCallbacks([this](QTabWidget* pane) { m_tabManager->addPane(pane); },
                                  [this](QTabWidget* pane) { m_tabManager->removePane(pane); },
                                  [this](QTabWidget* pane, int index)
                                  {
                                      m_tabManager->updateCloseButton(index, pane, SettingsManager::instance().closeButtonMode());
                                      m_tabManager->updateTabBarVisibility();
                                  });

    connect(m_splitView, &SplitView::activeTabWidgetChanged, this,
            [this](QTabWidget* tw)
            {
                m_tabManager->setActivePane(tw);
                updateStatusBar();
                m_editorController->updateUndoRedoState();
                updateFileTypeLabel();
                updateStatusBarLabelsVisibility();
                updateEncodingSelector();
                m_editorController->updateReadOnlyActionState();
            });

    connect(m_splitView, &SplitView::tabDetachedToWindow, this,
            [](const QString& filePath)
            {
#ifdef Q_OS_MACOS
                QStringList args;
                if (!filePath.isEmpty() && filePath[0] == QChar(1))
                    args << QStringLiteral("--transfer") << filePath.mid(1);
                else if (!filePath.isEmpty())
                    args << filePath;
                launchAppBundle(args);
#else
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
#endif
            });

    connect(m_splitView, &SplitView::externalTabDropped, this, [this](const QString& filePath) { loadFile(filePath); });

    setupDockWidgets();
    createManagers();
    connectRemoteService();
    connectUpdateService();

    setupUI();
    wireActions();
    connectPanelSignals();

    setCentralWidget(m_splitView);

    applyInitialSettings();

    setupEditorConnections();
    setupLspConnections();

#ifdef BUILD_PLUGINS
    m_pluginManager->loadAll();

    // The Terminal plugin creates TerminalPanel and registers it with the host.
    // Locate it here so MainWindow can connect signals and apply settings.
    setupTerminalPlugin();

    // Debugger UI (Debug menu, View->Debug Panel) is only relevant while the
    // DAP plugin is enabled. The plugin owns the debug panel, so without it
    // the actions would silently no-op.
    const bool dapEnabled = m_pluginManager->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap"));
    m_actionManager->setDebugUiVisible(dapEnabled);
    connect(m_pluginManager, &PluginManager::pluginEnabledChanged, this,
            [this](const QString& pluginId, bool enabled)
            {
                if (pluginId == QStringLiteral("org.semagsoft.devpad.dap"))
                {
                    m_actionManager->setDebugUiVisible(enabled);
                    if (!enabled)
                        m_debugSessionKey.clear();
                }
                else if (pluginId == QStringLiteral("org.semagsoft.devpad.terminal"))
                {
                    if (enabled)
                    {
                        setupTerminalPlugin();
                    }
                    else
                    {
                        // The plugin owns and deletes the panel on unload.
                        m_terminalPanelDock = nullptr;
                        m_actionManager->terminalPanelAct()->setVisible(false);
                        m_actionManager->terminalPanelButton()->setVisible(false);
                    }
                }
            });
#endif

    applyAutoSaveSettings();
    updateSplitViewVisibility();
}

void MainWindow::setupWindow()
{
    setWindowTitle(Strings::AppName());
    setMinimumSize(800, 600);
    QIcon windowIcon(":/icons/devpad.svg");
    if (!windowIcon.isNull())
        setWindowIcon(windowIcon);
    setAcceptDrops(true);
}

void MainWindow::setupDockWidgets()
{
    m_projectPanel = new ProjectPanel(this);
    {
        auto pos = SettingsManager::instance().projectPanelPosition();
        Qt::DockWidgetArea area = (pos == ProjectPanelPosition::Right) ? Qt::RightDockWidgetArea : Qt::LeftDockWidgetArea;
        addDockWidget(area, m_projectPanel);
    }
    m_projectPanel->hide();

    installEventFilter(m_splitView);
}

void MainWindow::createManagers()
{
    m_tabManager = new TabManager(m_tabWidget, this);
    m_searchManager = new SearchManager(this, m_tabManager);

    m_actionManager = new ActionManager(this);
    m_sessionManager = new SessionManager(this);
    m_encodingManager = new EncodingManager(this);
    m_themeApplier = new ThemeApplier(this);
    if (auto* filter = systemThemeChangeFilter())
    {
        connect(filter, &PaletteChangeFilter::systemThemeChanged, this, [this]()
                {
                    applySettings();
                });
    }
    m_printManager = new PrintManager(this);
    m_fileManager = new FileManager(this);
    m_fileWatcherManager = new FileWatcherManager(this);

    m_editorController = new EditorController(m_tabManager, m_fileManager, m_fileWatcherManager, m_actionManager, m_tabWidget, this);

    m_localServer = new QLocalServer(this);
    QString serverName = QStringLiteral("devpad-%1").arg(QCoreApplication::applicationPid());
    QLocalServer::removeServer(serverName);
    m_localServer->listen(serverName);
    connect(m_localServer, &QLocalServer::newConnection, this, &MainWindow::handleQuitRequest);

    m_externalToolManager = new ExternalToolManager(this);
    m_remoteFileService = new RemoteFileService(this);
    m_fileManager->setRemoteFileService(m_remoteFileService);
    m_updateChecker = new UpdateChecker(this);
    m_snippetManager = new SnippetManager(this);

    m_lspServerManager = new lsp::LspServerManager(this);
#ifdef BUILD_PLUGINS
    m_pluginManager = new PluginManager(this);
#endif
    m_errorListPanel = new ErrorListPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_errorListPanel);
    bool showErrorList = SettingsManager::instance().lspShowErrorList();
    m_errorListPanel->setVisible(showErrorList);

    m_referencesPanel = new ReferencesPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_referencesPanel);
    m_referencesPanel->hide();
    connect(m_referencesPanel, &ReferencesPanel::navigateToLocation, this, &MainWindow::onNavigateToLocation);

    m_dialogCoordinator = new DialogCoordinator(m_tabManager, m_projectPanel, this);
    connect(m_dialogCoordinator, &DialogCoordinator::navigateToResult, this, &MainWindow::onNavigateToLocation);

    m_externalChangeCoordinator = new ExternalChangeCoordinator(m_tabManager, m_fileWatcherManager, this);
    connect(m_externalChangeCoordinator, &ExternalChangeCoordinator::reloadRequested, this,
            [this](const QString& filePath, const QString& encoding, const QList<int>& bookmarks)
            {
                loadFile(filePath, encoding, true);
                if (auto* newEditor = m_tabManager->currentEditor())
                {
                    newEditor->setBookmarks(bookmarks);
                }
            });

    m_remoteOpenCoordinator = new RemoteOpenCoordinator(m_remoteFileService, m_projectPanel, m_tabManager, m_editorController, this);
    connect(m_remoteOpenCoordinator, &RemoteOpenCoordinator::fileTypeLabelDirty, this, &MainWindow::updateFileTypeLabel);

    connect(m_fileWatcherManager, &FileWatcherManager::fileModifiedExternally, this, &MainWindow::onFileModifiedExternally);
}

void MainWindow::connectRemoteService()
{
    connect(m_remoteFileService, &RemoteFileService::fileDownloaded, this,
            [this](const QString& url, const QString& fileName, const QByteArray& data)
            {
                statusBar()->clearMessage();
                openRemoteFileFromData(url, fileName, data);
            });
    connect(m_remoteFileService, &RemoteFileService::remoteDirListed, this,
            [this](const QString& url, const QString& parentPath, const QList<RemoteFsEntry>& entries)
            {
                statusBar()->clearMessage();
                m_projectPanel->applyRemoteListing(url, parentPath, entries);
                if (!m_projectPanel->isVisible())
                {
                    m_projectPanel->show();
                    m_actionManager->projectPanelAct()->setChecked(true);
                }
                Logger::instance().info(QString("Remote folder listing: %1 (%2, %3 entries)").arg(url, parentPath).arg(entries.size()));
            });
    connect(m_remoteFileService, &RemoteFileService::remoteListingFailed, this,
            [this](const QString&, const QString& remotePath, const QString&)
            {
                // Let the panel retry the directory (removes the pending state).
                m_projectPanel->onRemoteListingFailed(remotePath);
            });
    connect(m_remoteFileService, &RemoteFileService::downloadFailed, this,
            [this](const QString& url, const QString& error)
            {
                statusBar()->clearMessage();
                QMessageBox::warning(this, tr("Download Failed"), tr("Failed to download \"%1\":\n%2").arg(url, error));
            });
    connect(m_remoteFileService, &RemoteFileService::fileUploaded, this,
            [this](const QString& url)
            {
                statusBar()->clearMessage();
                Logger::instance().info(QString("Uploaded remote file: %1").arg(url));
            });
    connect(m_remoteFileService, &RemoteFileService::downloadProgress, this,
            [this](const QString&, qint64 received, qint64 total)
            {
                if (total > 0)
                {
                    int percent = static_cast<int>(received * 100 / total);
                    statusBar()->showMessage(tr("Downloading... %1%").arg(qBound(0, percent, 100)));
                }
            });
    connect(m_remoteFileService, &RemoteFileService::statusMessage, this, [this](const QString& message) { statusBar()->showMessage(message); });
}

void MainWindow::connectUpdateService()
{
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this,
            [this](const QString& latestVersion, const QString& releaseUrl)
            {
                statusBar()->clearMessage();
                UpdateDialog::showUpdateAvailable(this, latestVersion, releaseUrl);
            });
    connect(m_updateChecker, &UpdateChecker::upToDate, this,
            [this](const QString& latestVersion)
            {
                statusBar()->clearMessage();
                UpdateDialog::showUpToDate(this, latestVersion);
            });
    connect(m_updateChecker, &UpdateChecker::checkFailed, this,
            [this](const QString& friendlyMessage, const QString& details)
            {
                statusBar()->clearMessage();
                if (UpdateDialog::showCheckFailed(this, friendlyMessage, details))
                    checkForUpdates();
            });
}

void MainWindow::applyInitialSettings()
{
    bool showErrorList = SettingsManager::instance().lspShowErrorList();

    m_actionManager->menuBarAct()->setChecked(SettingsManager::instance().showMenuBar());
    m_actionManager->toolBarAct()->setChecked(SettingsManager::instance().showToolbar());
    m_actionManager->statusBarAct()->setChecked(SettingsManager::instance().showStatusbar());
    addToolBar(m_actionManager->toolBar());
    menuBarWidget()->setVisible(SettingsManager::instance().showMenuBar());
    m_actionManager->toolBar()->setVisible(SettingsManager::instance().showToolbar());
    statusBar()->setVisible(SettingsManager::instance().showStatusbar());

    m_actionManager->errorListPanelAct()->setChecked(showErrorList);
    m_actionManager->errorListPanelButton()->setChecked(showErrorList);

    // Terminal panel settings are applied after the Terminal plugin loads
    // (see applyTerminalPanelSettings()).
    if (m_actionManager->terminalPanelAct()->isVisible())
    {
        m_actionManager->terminalPanelAct()->setChecked(SettingsManager::instance().showTerminalPanel());
        m_actionManager->terminalPanelButton()->setChecked(SettingsManager::instance().showTerminalPanel());
    }

    applyStartupMode();
    updateRecentFileActions();
    // Keep recent menus in sync across multiple windows.
    connect(&SettingsManager::instance(), &SettingsManager::recentFilesChanged, this, &MainWindow::updateRecentFileActions);
    applySettings();

    // Auto-check for updates on startup (respects the 1-hour interval so we
    // never hit the server more than once per hour).
    if (SettingsManager::instance().checkForUpdatesEnabled())
        m_updateChecker->checkForUpdates();
}

void MainWindow::setupUI()
{
    m_actionManager->createActions();
    m_actionManager->buildMenus(menuBar());
    m_actionManager->buildToolBar();
    m_actionManager->buildStatusBar(statusBar(), m_projectPanel, nullptr);

    m_actionManager->menuBarAct()->setChecked(SettingsManager::instance().showMenuBar());
    m_actionManager->toolBarAct()->setChecked(SettingsManager::instance().showToolbar());
    m_actionManager->statusBarAct()->setChecked(SettingsManager::instance().showStatusbar());

    auto* menuBarShortcut = new QShortcut(QKeySequence("Ctrl+Alt+M"), this);
    connect(menuBarShortcut, &QShortcut::activated, this,
            [this]()
            {
                bool visible = !menuBarWidget()->isVisible();
                menuBarWidget()->setVisible(visible);
                m_actionManager->menuBarAct()->setChecked(visible);
                SettingsManager::instance().setShowMenuBar(visible);
            });

    auto* toolBarShortcut = new QShortcut(QKeySequence("Ctrl+Alt+T"), this);
    connect(toolBarShortcut, &QShortcut::activated, m_actionManager->toolBarAct(), &QAction::trigger);

    auto* statusBarShortcut = new QShortcut(QKeySequence("Ctrl+Shift+Alt+S"), this);
    connect(statusBarShortcut, &QShortcut::activated, m_actionManager->statusBarAct(), &QAction::trigger);

    auto* winCopyShortcut = new QShortcut(QKeySequence("Meta+C"), this);
    connect(winCopyShortcut, &QShortcut::activated, this, &MainWindow::copy);

    auto* winPasteShortcut = new QShortcut(QKeySequence("Meta+V"), this);
    connect(winPasteShortcut, &QShortcut::activated, this, &MainWindow::paste);

    const auto shortcutActions = m_actionManager->actionsWithShortcuts();
    for (QAction* act : shortcutActions)
        addAction(act);

    applyTitleBarMode();
}

void MainWindow::applyTitleBarMode()
{
#ifdef Q_OS_WIN
    const bool useTitlebar = SettingsManager::instance().showMenuInTitlebar();

    if (useTitlebar && !m_titleBar)
    {
        m_titleBar = new TitleBar(this);
        setMenuWidget(m_titleBar);

        QMenuBar* mb = new QMenuBar(m_titleBar);
        m_titleBar->setMenuBar(mb);
        m_actionManager->buildMenus(mb);
        m_titleBar->setMaximized(isMaximized());

        connect(m_titleBar, &TitleBar::minimizeRequested, this, &MainWindow::showMinimized);
        connect(m_titleBar, &TitleBar::maximizeRequested, this,
                [this]()
                {
                    if (isMaximized())
                        showNormal();
                    else
                        showMaximized();
                });
        connect(m_titleBar, &TitleBar::closeRequested, this, &MainWindow::close);

        m_frameBridge = new WindowsFrameBridge(this, m_titleBar);

        if (m_actionManager->menuTitlebarAct())
            m_actionManager->menuTitlebarAct()->setChecked(true);
    }
    else if (!useTitlebar && m_titleBar)
    {
        if (m_frameBridge)
        {
            qApp->removeNativeEventFilter(m_frameBridge);
            delete m_frameBridge;
            m_frameBridge = nullptr;
        }

        if (QMenuBar* mb = m_titleBar->menuBarWidget())
        {
            mb->setParent(this);
            mb->show();
            setMenuBar(mb);
        }
        delete m_titleBar;
        m_titleBar = nullptr;

        if (m_actionManager->menuTitlebarAct())
            m_actionManager->menuTitlebarAct()->setChecked(false);
    }
#endif
}

QMenuBar* MainWindow::menuBarWidget()
{
    return m_titleBar ? m_titleBar->menuBarWidget() : menuBar();
}

void MainWindow::wireActions()
{
    ActionTargets targets;
    targets.editorController = m_editorController;
    targets.terminalPanel = nullptr;
    targets.splitView = m_splitView;
    targets.encodingManager = m_encodingManager;
    targets.mainWindow = this;
    targets.openRecentFile = [this](const QString& filePath) { loadFile(filePath); };
    targets.clearRecentFiles = [this]() { updateRecentFileActions(); };
    targets.updateRecentFileActions = [this]() { updateRecentFileActions(); };
    targets.encodingSelected = [this](const QString& encoding, bool reopen)
    {
        if (reopen)
            reloadCurrentFileWithEncoding(encoding);
        else
            saveCurrentFileWithEncoding(encoding);
    };
    targets.tabPinToggled = [this](int localIdx, QTabWidget* pane)
    {
        auto* w = pane->widget(localIdx);
        auto* editor = w ? w->findChild<CodeEditor*>() : nullptr;
        if (editor)
            m_tabManager->setTabPinned(editor, !m_tabManager->isTabPinned(editor));
    };
    m_actionManager->wireConnections(targets);
}

void MainWindow::connectPanelSignals()
{
    connect(m_tabManager, &TabManager::editorCreated, this, [this](CodeEditor* editor) { onEditorCreated(editor); });
    connect(m_tabManager, &TabManager::editorClosed, this,
            [this](CodeEditor* editor)
            {
                const QString& filePath = editor->fileName();
                if (!filePath.isEmpty() && filePath != Strings::untitled())
                    m_lspServerManager->closeDocument(lsp::uriFromPath(filePath));
                updateSplitViewVisibility();
            });
    connect(m_tabManager, &TabManager::tabCloseRequested, this,
            [this](int index)
            {
                if (m_editorController->onTabCloseRequested(index))
                {
                    updateSplitViewVisibility();
                    updateStatusBarLabelsVisibility();
                    m_editorController->updateReadOnlyActionState();
                }
            });
    connect(m_projectPanel, &ProjectPanel::fileActivated, this, &MainWindow::openFileFromProject);
    connect(m_projectPanel, &ProjectPanel::fileRenamed, m_editorController, &EditorController::retargetEditorFile);
    connect(m_projectPanel, &ProjectPanel::fileDeleted, m_editorController, &EditorController::handleFileDeleted);
    connect(m_projectPanel, &ProjectPanel::folderSelected, this, &MainWindow::openFolderFromPath);
    connect(m_projectPanel, &ProjectPanel::remoteDirExpandRequested, this,
            [this](const QString& url, const QString& path) { m_remoteFileService->listRemoteDir(url, path); });
    connect(m_projectPanel, &ProjectPanel::remoteFileActivated, this, &MainWindow::openRemoteFileActivated);
    connect(m_projectPanel, &ProjectPanel::remoteFolderSelected, this,
            [this](const QString& url)
            {
                Logger::instance().info(QString("Opening recent remote folder: %1").arg(url));
                m_remoteFileService->openRemoteFolder(url);
            });

    connect(m_projectPanel, &QDockWidget::visibilityChanged, m_actionManager->projectPanelAct(), &QAction::setChecked);
    connect(m_projectPanel, &QDockWidget::visibilityChanged, m_actionManager->projectPanelButton(), &QToolButton::setChecked);
}

void MainWindow::setupEditorConnections()
{
    connect(m_tabManager, &TabManager::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_editorController, &EditorController::editorConnected, this,
            [this](CodeEditor* editor)
            {
                connect(editor, &QsciScintilla::textChanged, this, &MainWindow::updateStatusBar);
                connect(editor, &QsciScintilla::cursorPositionChanged, this, [this]() { updateStatusBar(); });
                connect(editor, &CodeEditor::findRequested, this, &MainWindow::find);
                connect(editor, &CodeEditor::replaceRequested, this, &MainWindow::replace);
                connect(editor, &CodeEditor::goToLineRequested, this, &MainWindow::goToLine);
                connect(editor, &CodeEditor::insertSnippetRequested, m_editorController, &EditorController::insertSnippet);
                connect(editor, &CodeEditor::fileDropped, this, [this](const QString& path) { loadFile(path); });
                connect(editor, &CodeEditor::navigateToLocation, this, &MainWindow::onNavigateToLocation);
                connect(editor, &CodeEditor::diagnosticsChanged, this,
                        [this](const QString& uri, const QList<lsp::Diagnostic>& diags) { m_errorListPanel->updateDiagnostics(uri, diags); });

                QString filePath = editor->fileName();
                if (!filePath.isEmpty() && filePath != Strings::untitled())
                {
                    QString syntax = SettingsManager::instance().syntaxForFile(filePath);
                    if (!syntax.isEmpty())
                    {
                        editor->setLspLanguage(syntax);
                        editor->setLspServerManager(m_lspServerManager);
                        QPointer<CodeEditor> guard(editor);
                        QTimer::singleShot(200, this,
                                           [guard]()
                                           {
                                               if (guard)
                                                   guard->sendDidOpen();
                                           });
                    }
                }
            });

    connect(m_editorController, &EditorController::fileSaved, this,
            [this](const QString& fileName)
            {
                CodeEditor* editor = m_editorController->findEditorByFileName(fileName);
                if (!editor)
                    return;

                if (!editor->lspActive())
                {
                    QString syntax = SettingsManager::instance().syntaxForFile(fileName);
                    if (!syntax.isEmpty())
                    {
                        editor->setLspLanguage(syntax);
                        editor->setLspServerManager(m_lspServerManager);
                        QPointer<CodeEditor> guard(editor);
                        QTimer::singleShot(200, this,
                                           [guard]()
                                           {
                                               if (guard)
                                                   guard->sendDidOpen();
                                           });
                    }
                }
                else
                {
                    QString uri = lsp::uriFromPath(fileName);
                    m_lspServerManager->saveDocument(uri);
                }
            });

    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout, m_editorController, &EditorController::autoSave);
}

void MainWindow::setupLspConnections()
{
    lsp::setupLspConnections(m_lspServerManager, m_tabManager, m_errorListPanel, m_referencesPanel, m_actionManager,
                             [this](const QString& filePath, int line, int column) { onNavigateToLocation(filePath, line, column); });
}

void MainWindow::applyCloseButtonPosition()
{
    m_tabManager->applyCloseButtonPosition(SettingsManager::instance().closeButtonMode());
}

void MainWindow::applyTabBarPosition()
{
    m_tabManager->applyTabBarPosition(SettingsManager::instance().tabBarPosition());
}

void MainWindow::applyTerminalPanelPosition()
{
    if (!m_terminalPanelDock)
        return;
    bool visible = SettingsManager::instance().showTerminalPanel();
    m_actionManager->terminalPanelAct()->setChecked(visible);
    m_actionManager->terminalPanelButton()->setChecked(visible);

    int pos = static_cast<int>(SettingsManager::instance().terminalPanelPosition());
    QMetaObject::invokeMethod(m_terminalPanelDock, "applyPosition", Q_ARG(int, pos), Q_ARG(QTabWidget*, m_tabWidget), Q_ARG(QMainWindow*, this));
}

void MainWindow::applyTerminalPanelSettings()
{
    if (!m_terminalPanelDock)
        return;

    if (SettingsManager::instance().showTerminalPanel())
    {
        QMetaObject::invokeMethod(m_terminalPanelDock, "toggle", Q_ARG(QTabWidget*, m_tabWidget), Q_ARG(QMainWindow*, this));
        m_actionManager->terminalPanelAct()->setChecked(true);
        m_actionManager->terminalPanelButton()->setChecked(true);
    }
    else
    {
        int pos = static_cast<int>(SettingsManager::instance().terminalPanelPosition());
        QMetaObject::invokeMethod(m_terminalPanelDock, "applyPosition", Q_ARG(int, pos), Q_ARG(QTabWidget*, m_tabWidget), Q_ARG(QMainWindow*, this));
    }
}

void MainWindow::setupTerminalPlugin()
{
    if (!m_pluginManager)
        return;

    auto* panel = m_pluginManager->host()->panel(QStringLiteral("terminal"));
    if (panel && panel->inherits("TerminalPanel"))
    {
        m_terminalPanelDock = qobject_cast<QDockWidget*>(panel);
        // The Terminal panel exists — show the View menu action and status bar button.
        m_actionManager->terminalPanelAct()->setVisible(true);
        m_actionManager->terminalPanelButton()->setVisible(true);
    }
    else
    {
        m_terminalPanelDock = nullptr;
        m_actionManager->terminalPanelAct()->setVisible(false);
        m_actionManager->terminalPanelButton()->setVisible(false);
        return;
    }

    // Connect terminal lifecycle signals (now that the panel exists).
    auto* meta = m_terminalPanelDock->metaObject();
    int startedIdx = meta->indexOfSignal("terminalStarted()");
    if (startedIdx >= 0)
    {
        QMetaMethod signalMethod = meta->method(startedIdx);
        auto* myMeta = this->metaObject();
        int slotIdx = myMeta->indexOfSlot("onTerminalStarted()");
        if (slotIdx >= 0)
            QObject::connect(m_terminalPanelDock, signalMethod, this, myMeta->method(slotIdx));
    }

    connect(m_terminalPanelDock, &QDockWidget::visibilityChanged, this,
            [this](bool visible)
            {
                if (SettingsManager::instance().terminalPanelPosition() != TerminalPanelPosition::Tab)
                {
                    m_actionManager->terminalPanelAct()->setChecked(visible);
                    m_actionManager->terminalPanelButton()->setChecked(visible);
                    SettingsManager::instance().setShowTerminalPanel(visible);
                }
            });

    // Apply terminal settings now that the panel is available.
    applyTerminalPanelSettings();
}

void MainWindow::onTerminalStarted()
{
    if (!m_terminalPanelDock)
        return;
    auto disableDrops = [](auto&& self, QObject* obj) -> void
    {
        if (auto* w = qobject_cast<QWidget*>(obj))
            w->setAcceptDrops(false);
        for (QObject* child : obj->children())
            self(self, child);
    };
    disableDrops(disableDrops, m_terminalPanelDock);
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
    m_remoteOpenCoordinator->openRemoteFileFromData(urlStr, fileName, data);
}

void MainWindow::openRemote()
{
    m_remoteOpenCoordinator->openRemote();
}

void MainWindow::openRemoteFolder()
{
    m_remoteOpenCoordinator->openRemoteFolder();
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

    m_projectPanel->setRootPath(dirPath);
    m_projectPanel->show();
    m_actionManager->projectPanelAct()->setChecked(true);
    m_actionManager->projectPanelButton()->setChecked(true);
    if (m_terminalPanelDock)
        QMetaObject::invokeMethod(m_terminalPanelDock, "setWorkingDirectory", Q_ARG(QString, dirPath));
    Logger::instance().info(QString("Opened folder: %1").arg(dirPath));
}

void MainWindow::closeProject()
{
    m_projectPanel->setRootPath(QString());
    m_projectPanel->hide();
    m_actionManager->projectPanelAct()->setChecked(false);
    m_actionManager->projectPanelButton()->setChecked(false);
    Logger::instance().info("Closed project");
}

void MainWindow::openFileFromProject(const QString& filePath)
{
    loadFile(filePath);
}

void MainWindow::openRemoteFileActivated(const QString& urlStr)
{
    m_remoteOpenCoordinator->openRemoteFileActivated(urlStr);
}

void MainWindow::openFolderFromPath(const QString& folderPath)
{
    m_projectPanel->setRootPath(folderPath);
    m_projectPanel->show();
    m_actionManager->projectPanelAct()->setChecked(true);
    if (m_terminalPanelDock)
        QMetaObject::invokeMethod(m_terminalPanelDock, "setWorkingDirectory", Q_ARG(QString, folderPath));
    Logger::instance().info(QString("Opened folder: %1").arg(folderPath));
}

void MainWindow::loadFile(const QString& fileName, const QString& encoding, bool reload)
{
    if (!reload)
    {
        CodeEditor* existingEditor = m_tabManager->findEditorByFileName(fileName);
        if (existingEditor)
        {
            if (m_tabManager->containerFor(existingEditor))
            {
                m_tabManager->focusEditorPane(existingEditor);
                existingEditor->setFocus();
                return;
            }
            return;
        }
    }

    // When forcing a reload (revert / external-change reload), close any remaining
    // editors for this file across all panes so we don't end up with duplicate tabs
    // (one stale, one freshly reloaded). closeEditor() returns false for pinned tabs,
    // in which case we stop to avoid an infinite loop.
    if (reload)
    {
        while (CodeEditor* existingEditor = m_tabManager->findEditorByFileName(fileName))
        {
            int idx = m_tabManager->indexOf(existingEditor);
            if (idx < 0)
                break;
            if (!m_tabManager->closeEditor(idx))
                break;
        }
    }

    CodeEditor* editor = m_editorController->openFile(fileName, encoding);
    if (!editor)
    {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open file: ") + fileName);
        return;
    }

    SettingsManager::instance().addRecentFile(fileName);
    updateRecentFileActions();
    updateEncodingSelector();
    m_editorController->updateUndoRedoState();
    updateFileTypeLabel();
    Logger::instance().info(QString("Opened file: %1").arg(fileName));
    m_editorController->promptBackupRestore(fileName);
}

void MainWindow::newWindow()
{
#ifdef Q_OS_MACOS
    launchAppBundle(QStringList());
#else
    QString appPath = QApplication::applicationFilePath();
    if (!QProcess::startDetached(appPath, QStringList()))
    {
        Logger::instance().error("Failed to start detached process for new window");
    }
#endif
    Logger::instance().info("Opened new window");
}

void MainWindow::find()
{
    m_searchManager->showFindDialog();
}

void MainWindow::replace()
{
    m_searchManager->showReplaceDialog();
}

void MainWindow::goToLine()
{
    CodeEditor* editor = m_tabManager->currentEditor();
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
    m_searchManager->findNext();
}

void MainWindow::findPrevious()
{
    m_searchManager->findPrevious();
}

void MainWindow::findInFiles()
{
    m_dialogCoordinator->findInFiles();
}

void MainWindow::showOptions()
{
    OptionsDialog dlg(this);
    ThemeId originalTheme = SettingsManager::instance().theme();
    QColor originalAccent = SettingsManager::instance().accentColor();
    QString originalTermFontFamily = SettingsManager::instance().terminalFontFamily();
    int originalTermFontSize = SettingsManager::instance().terminalFontSize();

    connect(&dlg, &OptionsDialog::themeChanged, this,
            [this]()
            {
                applySettings();
                if (m_terminalPanelDock)
                    QMetaObject::invokeMethod(m_terminalPanelDock, "refreshTheme");
            });

    if (dlg.exec() == QDialog::Accepted)
    {
        applySettings();
        applyCloseButtonPosition();
        applyTabBarPosition();
        applyTerminalPanelPosition();
        if (m_terminalPanelDock)
            QMetaObject::invokeMethod(m_terminalPanelDock, "refreshTheme");
        m_tabManager->updateTabBarVisibility();
        updateRecentFileActions();
        applyAutoSaveSettings();
    }
    else
    {
        SettingsManager::instance().setTheme(originalTheme);
        SettingsManager::instance().setAccentColor(originalAccent);
        SettingsManager::instance().setTerminalFontFamily(originalTermFontFamily);
        SettingsManager::instance().setTerminalFontSize(originalTermFontSize);
        applySettings();
        if (m_terminalPanelDock)
            QMetaObject::invokeMethod(m_terminalPanelDock, "refreshTheme");
    }
}

void MainWindow::runExternalTool(int index)
{
    CodeEditor* editor = m_tabManager->currentEditor();
    QString filePath;
    int lineNumber = 1;
    QString selectedText;
    if (editor)
    {
        filePath = editor->fileName();
        int dummyIndex;
        editor->getCursorPosition(&lineNumber, &dummyIndex);
        lineNumber++;
        selectedText = editor->selectedText();
    }

    QString projectDir;
    if (m_projectPanel->isVisible() && !m_projectPanel->rootPath().isEmpty())
        projectDir = m_projectPanel->rootPath();

    m_externalToolManager->runTool(
        index, filePath, projectDir, selectedText, lineNumber,
        [this](const QString& cmd)
        {
            if (!m_terminalPanelDock)
                return;
            if (!m_terminalPanelDock->isVisible())
            {
                QMetaObject::invokeMethod(m_terminalPanelDock, "toggle", Q_ARG(QTabWidget*, m_tabWidget), Q_ARG(QMainWindow*, this));
                m_terminalPanelDock->show();
            }
            QMetaObject::invokeMethod(m_terminalPanelDock, "sendCommand", Q_ARG(QString, cmd));
        },
        this);
}

void MainWindow::applySettings()
{
    m_themeApplier->applyTheme(this);
    m_themeApplier->applySettingsToAllEditors(m_tabManager);
}

void MainWindow::applyStartupMode()
{
    if (QApplication::arguments().size() > 1)
        return;

    StartupMode mode = SettingsManager::instance().startupMode();
    if (mode == StartupMode::NewFile)
    {
        m_editorController->newFile();
    }
    else if (mode == StartupMode::OpenLastFile)
    {
        QStringList recentFiles = SettingsManager::instance().recentFiles();
        if (!recentFiles.isEmpty())
        {
            const QString& lastFile = recentFiles.first();
            if (QFile::exists(lastFile))
            {
                loadFile(lastFile);
            }
        }
    }
    else if (mode == StartupMode::RestoreSession)
    {
        auto sessionData = m_sessionManager->restoreSession([this](const QString& filePath) { loadFile(filePath); }, m_splitView, m_projectPanel);
        m_tabManager->setPinnedFiles(m_sessionManager->loadSessionPinnedFiles());

        QString terminalDir = sessionData.terminalWorkingDir;
        if (terminalDir.isEmpty())
            terminalDir = m_projectPanel->rootPath();
        if (terminalDir.isEmpty() && !sessionData.files.isEmpty())
            terminalDir = QFileInfo(sessionData.files.first()).absolutePath();
        if (!terminalDir.isEmpty() && m_terminalPanelDock)
            QMetaObject::invokeMethod(m_terminalPanelDock, "setWorkingDirectory", Q_ARG(QString, terminalDir));
    }
}

void MainWindow::updateStatusBar()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (!editor)
        return;

    int line, index;
    editor->getCursorPosition(&line, &index);
    m_actionManager->lineColLabel()->setText(tr("Line: %1, Col: %2").arg(line + 1).arg(index + 1));
}

void MainWindow::updateFileTypeLabel()
{
    CodeEditor* editor = m_tabManager->currentEditor();
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
            {"qml", tr("QML")},   {"lua", tr("Lua")},
        };
        syntax = displayNames.value(syntax, syntax);
    }

    if (editor->isReadOnlyMode())
    {
        syntax += tr(" | Read Only");
    }

    m_actionManager->fileTypeLabel()->setText(syntax);
}

void MainWindow::onTabChanged(int index)
{
    Q_UNUSED(index)
    updateSplitViewVisibility();
    updateStatusBar();
    m_editorController->updateUndoRedoState();
    updateFileTypeLabel();
    updateStatusBarLabelsVisibility();
    updateEncodingSelector();
    m_editorController->updateReadOnlyActionState();
}

void MainWindow::onEditorCreated(CodeEditor* editor)
{
    Q_UNUSED(editor)
    updateSplitViewVisibility();
    m_editorController->updateUndoRedoState();
    updateStatusBarLabelsVisibility();
    m_editorController->updateReadOnlyActionState();
}

void MainWindow::updateRecentFileActions()
{
    QStringList recentFiles = SettingsManager::instance().recentFiles();
    int numRecentFiles = qMin(recentFiles.size(), static_cast<qsizetype>(10));

    for (int i = 0; i < numRecentFiles; ++i)
    {
        QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(recentFiles[i]).fileName());
        m_actionManager->recentFileAct(i)->setText(text);
        m_actionManager->recentFileAct(i)->setData(recentFiles[i]);
        m_actionManager->recentFileAct(i)->setIcon(ProjectPanel::iconForFile(recentFiles[i]));
        m_actionManager->recentFileAct(i)->setVisible(true);
    }
    for (int i = numRecentFiles; i < 10; ++i)
    {
        m_actionManager->recentFileAct(i)->setVisible(false);
    }
    m_actionManager->recentFilesMenu()->setEnabled(numRecentFiles > 0);
}

void MainWindow::revertFile()
{
    CodeEditor* editor = m_tabManager->currentEditor();
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

    int idx = m_tabManager->indexOf(editor);
    QString encoding = editor->encoding();
    QString syntax = editor->syntax();
    QList<int> bookmarks = editor->bookmarkLines();

    m_tabManager->closeEditor(idx);
    m_fileWatcherManager->unwatchFile(fileName);
    loadFile(fileName, encoding, true);
    if (auto* newEditor = m_tabManager->currentEditor())
    {
        newEditor->setBookmarks(bookmarks);
    }
}

void MainWindow::printFile()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (!editor)
        return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        m_printManager->printEditorWithHighlighting(editor, &printer);
    }
}

void MainWindow::printPreview()
{
    CodeEditor* editor = m_tabManager->currentEditor();
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
                    thisPtr->m_printManager->printEditorWithHighlighting(editorPtr, p);
                }
            });
    preview.exec();
}

void MainWindow::pageSetup()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (!editor)
        return;

    QPrinter printer(QPrinter::HighResolution);
    QPageSetupDialog dlg(&printer, this);
    dlg.exec();
}

void MainWindow::paste()
{
    if (m_terminalPanelDock)
    {
        bool hasFocus = false;
        QMetaObject::invokeMethod(m_terminalPanelDock, "terminalHasFocus", Qt::DirectConnection, Q_RETURN_ARG(bool, hasFocus));
        if (hasFocus)
            QMetaObject::invokeMethod(m_terminalPanelDock, "pasteToTerminal");
        else if (m_editorController)
            m_editorController->paste();
    }
    else if (m_editorController)
        m_editorController->paste();
}

void MainWindow::copy()
{
    if (m_terminalPanelDock)
    {
        bool hasFocus = false;
        QMetaObject::invokeMethod(m_terminalPanelDock, "terminalHasFocus", Qt::DirectConnection, Q_RETURN_ARG(bool, hasFocus));
        if (hasFocus)
            QMetaObject::invokeMethod(m_terminalPanelDock, "copyFromTerminal");
        else if (m_editorController)
            m_editorController->copy();
    }
    else if (m_editorController)
        m_editorController->copy();
}

void MainWindow::toggleFullScreen()
{
    bool fullScreen = !isFullScreen();
    if (fullScreen)
        showFullScreen();
    else
        showNormal();
    m_actionManager->fullScreenAct()->setChecked(fullScreen);
}

void MainWindow::toggleProjectPanel()
{
    if (m_projectPanel->isVisible())
    {
        m_projectPanel->hide();
        m_actionManager->projectPanelAct()->setChecked(false);
        m_actionManager->projectPanelButton()->setChecked(false);
    }
    else
    {
        m_projectPanel->show();
        m_actionManager->projectPanelAct()->setChecked(true);
        m_actionManager->projectPanelButton()->setChecked(true);
    }
}

void MainWindow::toggleTerminalPanel()
{
    if (!m_terminalPanelDock || !m_actionManager->terminalPanelAct()->isVisible())
        return;
    QMetaObject::invokeMethod(m_terminalPanelDock, "toggle", Q_ARG(QTabWidget*, m_tabWidget), Q_ARG(QMainWindow*, this));
    bool visible = SettingsManager::instance().showTerminalPanel();
    m_actionManager->terminalPanelAct()->setChecked(visible);
    m_actionManager->terminalPanelButton()->setChecked(visible);
}

void MainWindow::toggleErrorListPanel()
{
    bool visible = !m_errorListPanel->isVisible();
    m_errorListPanel->setVisible(visible);
    m_actionManager->errorListPanelAct()->setChecked(visible);
    m_actionManager->errorListPanelButton()->setChecked(visible);
    SettingsManager::instance().setLspShowErrorList(visible);
}

void MainWindow::toggleReferencesPanel()
{
    bool visible = !m_referencesPanel->isVisible();
    m_referencesPanel->setVisible(visible);
    m_actionManager->referencesPanelAct()->setChecked(visible);
}

void MainWindow::findSymbols()
{
    m_dialogCoordinator->findSymbols(m_lspServerManager);
}

void MainWindow::expandSelection()
{
    invokeOnEditor(&CodeEditor::expandSelection);
}

void MainWindow::shrinkSelection()
{
    invokeOnEditor(&CodeEditor::shrinkSelection);
}

void MainWindow::updateStatusBarLabelsVisibility()
{
    bool hasEditor = m_tabManager->count() > 0;
    QTabWidget* activePane = m_tabManager->activePane();
    bool isTerminalTab = (activePane != nullptr) && (SettingsManager::instance().terminalPanelPosition() == TerminalPanelPosition::Tab) &&
                         (activePane->currentWidget() == m_terminalPanelDock);
    bool shouldShow = hasEditor && !isTerminalTab;
    m_actionManager->lineColLabel()->setVisible(shouldShow);
    m_actionManager->encodingComboBox()->setVisible(shouldShow);
    m_actionManager->fileTypeLabel()->setVisible(shouldShow);
}

void MainWindow::quitDevpad()
{
    m_quitRequested = true;

    if (!signalOtherInstancesToQuit())
        return;

    if (!saveAllModifiedTabs())
        return;

    if (m_actionManager->terminalPanelAct()->isVisible())
        SettingsManager::instance().setShowTerminalPanel(m_actionManager->terminalPanelAct()->isChecked());
    QString termDir;
    if (m_terminalPanelDock)
        QMetaObject::invokeMethod(m_terminalPanelDock, "workingDirectory", Qt::DirectConnection, Q_RETURN_ARG(QString, termDir));
    m_sessionManager->saveSession(m_tabManager, m_projectPanel, termDir);
    QApplication::quit();
}

bool MainWindow::signalOtherInstancesToQuit()
{
#ifdef Q_OS_LINUX
    QProcess pgrep;
    pgrep.start(QStringLiteral("pgrep"), QStringList() << QStringLiteral("-ix") << QStringLiteral("Devpad"));
    if (!pgrep.waitForFinished(2000))
        return true;

    QList<qint64> pids;
    for (const QString& line : QString::fromUtf8(pgrep.readAllStandardOutput()).split(QLatin1Char('\n'), Qt::SkipEmptyParts))
    {
        qint64 pid = line.trimmed().toLongLong();
        if (pid > 0 && pid != QCoreApplication::applicationPid())
            pids.append(pid);
    }

    for (qint64 pid : pids)
    {
        QLocalSocket socket;
        socket.connectToServer(QStringLiteral("devpad-%1").arg(pid));
        if (!socket.waitForConnected(1000))
            continue;

        socket.write("save_and_quit\n");
        socket.flush();

        if (socket.waitForReadyRead(1000))
        {
            while (socket.canReadLine())
            {
                QString line = QString::fromUtf8(socket.readLine()).trimmed();
                if (line == QStringLiteral("abort"))
                {
                    m_quitRequested = false;
                    return false;
                }
            }
        }
    }
#else
    Q_UNUSED(QCoreApplication::applicationPid())
#endif
    return true;
}

bool MainWindow::saveAllModifiedTabs()
{
    for (int i = m_tabManager->count() - 1; i >= 0; --i)
    {
        CodeEditor* editor = m_tabManager->editorAt(i);
        if (!m_editorController->maybeSave(editor))
        {
            m_quitRequested = false;
            return false;
        }
    }
    return true;
}

void MainWindow::handleQuitRequest()
{
    QLocalSocket* client = m_localServer->nextPendingConnection();
    if (!client)
        return;
    connect(client, &QLocalSocket::readyRead, this,
            [this, client]()
            {
                if (!client->canReadLine())
                    return;

                QString command = QString::fromUtf8(client->readLine()).trimmed();
                if (command == QStringLiteral("open_files"))
                {
                    QStringList paths;
                    while (client->canReadLine())
                    {
                        const QString line = QString::fromUtf8(client->readLine()).trimmed();
                        if (line == QStringLiteral("end"))
                            break;
                        if (!line.isEmpty())
                            paths.append(line);
                    }
                    for (const QString& path : paths)
                    {
                        const QFileInfo info(path);
                        if (info.isDir())
                            openFolderFromPath(path);
                        else
                            openFileFromPath(path);
                    }
                    raise();
                    activateWindow();
                    client->write("ok\n");
                    client->flush();
                    client->disconnectFromServer();
                    return;
                }
                if (command != QStringLiteral("save_and_quit"))
                    return;

                if (m_quitRequested)
                {
                    client->disconnectFromServer();
                    return;
                }
                m_quitRequested = true;

                if (!saveAllModifiedTabs())
                {
                    client->write("abort\n");
                    client->flush();
                    client->disconnectFromServer();
                    return;
                }

                if (m_actionManager->terminalPanelAct()->isVisible())
                    SettingsManager::instance().setShowTerminalPanel(m_actionManager->terminalPanelAct()->isChecked());
                QString termDir;
                if (m_terminalPanelDock)
                    QMetaObject::invokeMethod(m_terminalPanelDock, "workingDirectory", Qt::DirectConnection, Q_RETURN_ARG(QString, termDir));
                m_sessionManager->saveSession(m_tabManager, m_projectPanel, termDir);
                client->disconnectFromServer();
                QApplication::quit();
            });
    connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_quitRequested)
    {
        event->accept();
        return;
    }
    for (int i = m_tabManager->count() - 1; i >= 0; --i)
    {
        CodeEditor* editor = m_tabManager->editorAt(i);
        if (!m_editorController->maybeSave(editor))
        {
            event->ignore();
            return;
        }
    }
    if (m_actionManager->terminalPanelAct()->isVisible())
        SettingsManager::instance().setShowTerminalPanel(m_actionManager->terminalPanelAct()->isChecked());
    QString termDir;
    if (m_terminalPanelDock)
        QMetaObject::invokeMethod(m_terminalPanelDock, "workingDirectory", Qt::DirectConnection, Q_RETURN_ARG(QString, termDir));
    m_sessionManager->saveSession(m_tabManager, m_projectPanel, termDir);
    event->accept();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        m_actionManager->fullScreenAct()->setChecked(isFullScreen());
        if (m_titleBar)
        {
            m_titleBar->setMaximized(isMaximized());
            m_titleBar->setVisible(!isFullScreen());
        }
    }
    else if (event->type() == QEvent::ThemeChange)
    {
        if (prefersNativeStyling(SettingsManager::instance().theme()))
        {
            window()->setStyleSheet(QString());

            for (int i = 0; i < m_tabManager->count(); ++i)
            {
                CodeEditor* editor = m_tabManager->editorAt(i);
                if (editor)
                {
                    editor->applyTheme(ThemeId::System);
                }
            }

            if (m_terminalPanelDock)
                QMetaObject::invokeMethod(m_terminalPanelDock, "refreshTheme");
        }
    }
    QMainWindow::changeEvent(event);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
#ifdef Q_OS_MACOS
    if (event->type() == QEvent::FileOpen)
    {
        const auto* fileOpenEvent = static_cast<const QFileOpenEvent*>(event);
        const QString path = fileOpenEvent->file();
        if (!path.isEmpty())
        {
            const QFileInfo info(path);
            if (info.isDir())
                openFolderFromPath(path);
            else
                openFileFromPath(path);
            raise();
            activateWindow();
        }
        return true;
    }
#endif
    return QMainWindow::eventFilter(obj, event);
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

void MainWindow::checkForUpdates()
{
    if (!SettingsManager::instance().checkForUpdatesEnabled())
    {
        statusBar()->showMessage(tr("Update checks are disabled in settings."), 3000);
        return;
    }

    statusBar()->showMessage(tr("Checking for updates..."));
    if (!m_updateChecker->checkForUpdates())
    {
        // A check was already performed recently; don't hammer the server.
        statusBar()->showMessage(tr("Already checked for updates recently. Try again later."), 3000);
    }
}

void MainWindow::updateEncodingSelector()
{
    m_encodingManager->updateEncodingSelector(m_actionManager->encodingComboBox(), m_tabManager->currentEditor());
}

void MainWindow::reloadCurrentFileWithEncoding(const QString& encoding)
{
    m_editorController->reloadWithEncoding(encoding);
    updateEncodingSelector();
}

void MainWindow::saveCurrentFileWithEncoding(const QString& encoding)
{
    m_editorController->saveWithEncoding(encoding);
    updateEncodingSelector();
}

void MainWindow::invokeOnEditor(void (CodeEditor::*method)())
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
        (editor->*method)();
}

void MainWindow::goToDefinition()
{
    invokeOnEditor(&CodeEditor::goToDefinition);
}

void MainWindow::formatSelection()
{
    invokeOnEditor(&CodeEditor::formatSelection);
}

void MainWindow::goToTypeDefinition()
{
    invokeOnEditor(&CodeEditor::goToTypeDefinition);
}

void MainWindow::goToDeclaration()
{
    invokeOnEditor(&CodeEditor::goToDeclaration);
}

void MainWindow::renameSymbol()
{
    invokeOnEditor(&CodeEditor::requestRename);
}

void MainWindow::findReferences()
{
    m_referencesPanel->clear();
    m_referencesPanel->show();
    m_referencesPanel->raise();
    m_referencesPanel->setFocus();
    invokeOnEditor(&CodeEditor::findReferences);
}

void MainWindow::requestCodeActions()
{
    invokeOnEditor(&CodeEditor::requestCodeActions);
}

void MainWindow::triggerCompletion()
{
    invokeOnEditor(&CodeEditor::triggerCompletion);
}

void MainWindow::startDebugging()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (!editor)
        return;
    QString filePath = editor->fileName();
    if (filePath.isEmpty())
        return;

    QString language = editor->syntax();
    QString command = SettingsManager::instance().dapAdapterCommand(language);
    QStringList args = SettingsManager::instance().dapAdapterArgs(language);

    auto* plugin = m_pluginManager->plugin("org.semagsoft.devpad.dap");
    if (plugin)
        QMetaObject::invokeMethod(dynamic_cast<QObject*>(plugin), "startDebugging", Qt::DirectConnection, Q_ARG(QString, lsp::uriFromPath(filePath)),
                                  Q_ARG(QString, language), Q_ARG(QString, command), Q_ARG(QStringList, args));
}

void MainWindow::stopDebugging()
{
    auto* plugin = m_pluginManager->plugin("org.semagsoft.devpad.dap");
    if (plugin)
        QMetaObject::invokeMethod(dynamic_cast<QObject*>(plugin), "stopDebugging", Qt::DirectConnection, Q_ARG(QString, m_debugSessionKey));
    m_debugSessionKey.clear();
    m_actionManager->startDebuggingAct()->setEnabled(true);
    m_actionManager->stopDebuggingAct()->setEnabled(false);
    m_actionManager->debugPanelAct()->setChecked(false);
}

void MainWindow::debugContinue()
{
    auto* plugin = m_pluginManager->plugin("org.semagsoft.devpad.dap");
    if (plugin)
        QMetaObject::invokeMethod(dynamic_cast<QObject*>(plugin), "debugContinue", Qt::DirectConnection, Q_ARG(QString, m_debugSessionKey));
}

void MainWindow::debugStepOver()
{
    auto* plugin = m_pluginManager->plugin("org.semagsoft.devpad.dap");
    if (plugin)
        QMetaObject::invokeMethod(dynamic_cast<QObject*>(plugin), "debugStepOver", Qt::DirectConnection, Q_ARG(QString, m_debugSessionKey));
}

void MainWindow::debugStepInto()
{
    auto* plugin = m_pluginManager->plugin("org.semagsoft.devpad.dap");
    if (plugin)
        QMetaObject::invokeMethod(dynamic_cast<QObject*>(plugin), "debugStepInto", Qt::DirectConnection, Q_ARG(QString, m_debugSessionKey));
}

void MainWindow::debugStepOut()
{
    auto* plugin = m_pluginManager->plugin("org.semagsoft.devpad.dap");
    if (plugin)
        QMetaObject::invokeMethod(dynamic_cast<QObject*>(plugin), "debugStepOut", Qt::DirectConnection, Q_ARG(QString, m_debugSessionKey));
}

void MainWindow::debugRestart()
{
    auto* plugin = m_pluginManager->plugin("org.semagsoft.devpad.dap");
    if (plugin)
        QMetaObject::invokeMethod(dynamic_cast<QObject*>(plugin), "debugRestart", Qt::DirectConnection, Q_ARG(QString, m_debugSessionKey));
    m_debugSessionKey.clear();
    startDebugging();
}

void MainWindow::toggleDebugPanel()
{
    auto* dock = qobject_cast<QDockWidget*>(m_pluginManager->host()->panel("debug"));
    if (!dock)
        return;
    bool visible = !dock->isVisible();
    dock->setVisible(visible);
    m_actionManager->debugPanelAct()->setChecked(visible);
}

void MainWindow::onNavigateToLocation(const QString& filePath, int line, int column)
{
    if (filePath.isEmpty())
        return;

    CodeEditor* existing = m_tabManager->findEditorByFileName(filePath);
    if (existing)
    {
        m_tabManager->focusEditorPane(existing);
        existing->setCursorPosition(line, column);
        existing->setFocus();
        return;
    }

    loadFile(filePath);
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
    {
        editor->setCursorPosition(line, column);
    }
}

void MainWindow::applyAutoSaveSettings()
{
    if (SettingsManager::instance().autoSaveEnabled())
    {
        m_autoSaveTimer->start(SettingsManager::instance().autoSaveInterval() * 1000);
    }
    else
    {
        m_autoSaveTimer->stop();
    }
}

QString MainWindow::getSyntaxForFile(const QString& displayName) const
{
    return SettingsManager::instance().syntaxForFile(displayName);
}

void MainWindow::onFileModifiedExternally(const QString& filePath)
{
    m_externalChangeCoordinator->handleFileModifiedExternally(filePath);
}

void MainWindow::updateSplitViewVisibility()
{
    QTabWidget* primary = m_splitView->primaryTabWidget();
    if (primary->count() == 0)
    {
        for (int i = 0; i < m_splitView->paneCount(); ++i)
        {
            QTabWidget* pane = m_splitView->paneAt(i);
            if (pane && pane != primary && pane->count() > 0)
            {
                for (int j = pane->count() - 1; j >= 0; --j)
                {
                    m_splitView->moveTabToPane(j, pane, primary);
                }
                break;
            }
        }
    }
    QList<QTabWidget*> emptyPanes;
    for (int i = m_splitView->paneCount() - 1; i >= 0; --i)
    {
        QTabWidget* pane = m_splitView->paneAt(i);
        if (pane && pane->count() == 0 && pane != primary)
            emptyPanes.append(pane);
    }
    for (QTabWidget* pane : emptyPanes)
        m_splitView->removePane(pane);
    m_tabManager->updateTabBarVisibility();
}

void MainWindow::openTransferFile(const QString& filePath)
{
    loadFile(filePath);
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor && editor->fileName() == filePath)
    {
        m_fileWatcherManager->unwatchFile(filePath);
        editor->setFileName(Strings::untitled());
        editor->forceModified();
        m_tabManager->updateTabTitle(editor);
        QFile::remove(filePath);
    }
}
