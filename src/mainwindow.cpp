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
#include "errorlistpanel.h"
#include "findsymbolsdialog.h"
#include "editorcontroller.h"
#include "encodingmanager.h"
#include "encodingutils.h"
#include "externaltoolmanager.h"
#include "filemanager.h"
#include "filewatchermanager.h"
#include "findinfilesdialog.h"
#include "logger.h"
#include "externaltoolsdialog.h"
#include "lsp/lsptypes.h"
#include "lsp/lspservermanager.h"
#include "lsp/lspclient.h"
#include "optionsdialog.h"
#include "printmanager.h"
#include "projectpanel.h"
#include "remotefileservice.h"
#include "searchmanager.h"
#include "sessionmanager.h"
#include "settingsmanager.h"
#include "snippetmanager.h"
#include "splitview.h"
#include "tabmanager.h"
#include "terminalpanel.h"
#include "theme.h"
#include "themeapplier.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QFileDialog>
#include <QFileInfo>
#include <QDesktopServices>
#include <QLocalSocket>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QPointer>
#include <QPageSetupDialog>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QProcess>
#include <QRegularExpression>
#include <QShortcut>
#include <QStringConverter>
#include <QTabBar>
#include <QTextDocument>
#include <QToolButton>
#ifndef Q_OS_WIN
#include <qtermwidget.h>
#endif

MainWindow::~MainWindow()
{
    // Natural QObject child destruction order (reverse creation) is correct:
    //   m_editorController (created last)  → destroyed first  → m_tabManager alive ✓
    //   m_tabManager       (created second) → destroyed second → m_splitView alive ✓
    //   m_splitView        (created first)  → destroyed last
    // No manual delete ordering needed.
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle(Strings::AppName());
    setMinimumSize(800, 600);
    QIcon windowIcon(":/icons/devpad.svg");
    if (!windowIcon.isNull())
    {
        setWindowIcon(windowIcon);
    }
    setAcceptDrops(true);

    m_splitView = new SplitView(this);
    m_tabWidget = m_splitView->primaryTabWidget();
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->tabBar()->setExpanding(false);

    m_splitView->setPaneCallbacks([this](QTabWidget* pane) { m_tabManager->addPane(pane); }, [this](QTabWidget* pane) { m_tabManager->removePane(pane); },
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

    connect(m_splitView, &SplitView::externalTabDropped, this,
            [this](const QString &filePath)
            {
                loadFile(filePath);
            });

    m_projectPanel = new ProjectPanel(this);
    {
        auto pos = SettingsManager::instance().projectPanelPosition();
        Qt::DockWidgetArea area = (pos == ProjectPanelPosition::Right)
            ? Qt::RightDockWidgetArea : Qt::LeftDockWidgetArea;
        addDockWidget(area, m_projectPanel);
    }
    m_projectPanel->hide();

    m_terminalPanel = new TerminalPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_terminalPanel);
    m_terminalPanel->hide();

    // Install SplitView's event filter on MainWindow to catch drag events over
    // dock widgets and other areas where no child widget accepts drops
    installEventFilter(m_splitView);

    // QTermWidget's internal drag handling intercepts events before our
    // filters; disable acceptDrops on all terminal child widgets so that drag
    // events fall through to MainWindow where the SplitView's filter handles
    // them (cross-process acknowledgments, etc.)
    connect(m_terminalPanel, &TerminalPanel::terminalStarted, this, [this]() {
        auto disableDrops = [](auto &&self, QObject *obj) -> void {
            if (auto *w = qobject_cast<QWidget*>(obj))
                w->setAcceptDrops(false);
            for (QObject *child : obj->children())
                self(self, child);
        };
        disableDrops(disableDrops, m_terminalPanel);
    });

    m_tabManager = new TabManager(m_tabWidget, this);
    m_searchManager = new SearchManager(this, m_tabManager);

    m_actionManager = new ActionManager(this);
    m_sessionManager = new SessionManager(this);
    m_encodingManager = new EncodingManager(this);
    m_themeApplier = new ThemeApplier(this);
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
    m_snippetManager = new SnippetManager(this);

    m_lspServerManager = new lsp::LspServerManager(this);
    m_errorListPanel = new ErrorListPanel(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_errorListPanel);
    bool showErrorList = SettingsManager::instance().lspShowErrorList();
    m_errorListPanel->setVisible(showErrorList);

    connect(m_fileWatcherManager, &FileWatcherManager::fileModifiedExternally, this, &MainWindow::onFileModifiedExternally);

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
                    int percent = static_cast<int>(received * 100 / total);
                    statusBar()->showMessage(tr("Downloading... %1%").arg(qBound(0, percent, 100)));
                }
            });
    connect(m_remoteFileService, &RemoteFileService::statusMessage, this, [this](const QString& message) { statusBar()->showMessage(message); });

    setupUI();
    wireActions();
    connectPanelSignals();

    setCentralWidget(m_splitView);

    m_actionManager->menuBarAct()->setChecked(SettingsManager::instance().showMenuBar());
    m_actionManager->toolBarAct()->setChecked(SettingsManager::instance().showToolbar());
    m_actionManager->statusBarAct()->setChecked(SettingsManager::instance().showStatusbar());
    addToolBar(m_actionManager->toolBar());
    menuBar()->setVisible(SettingsManager::instance().showMenuBar());
    m_actionManager->toolBar()->setVisible(SettingsManager::instance().showToolbar());
    statusBar()->setVisible(SettingsManager::instance().showStatusbar());

    m_actionManager->terminalPanelAct()->setChecked(SettingsManager::instance().showTerminalPanel());
    m_actionManager->terminalPanelButton()->setChecked(SettingsManager::instance().showTerminalPanel());
    m_actionManager->errorListPanelAct()->setChecked(showErrorList);
    m_actionManager->errorListPanelButton()->setChecked(showErrorList);

    applyStartupMode();
    updateRecentFileActions();

    if (SettingsManager::instance().showTerminalPanel())
    {
        m_terminalPanel->toggle(m_tabWidget, this);
        m_actionManager->terminalPanelAct()->setChecked(true);
        m_actionManager->terminalPanelButton()->setChecked(true);
    }
    else
    {
        m_terminalPanel->applyPosition(SettingsManager::instance().terminalPanelPosition(), m_tabWidget, this);
    }

    applySettings();

    setupEditorConnections();
    setupLspConnections();

    applyAutoSaveSettings();
    updateSplitViewVisibility();
}

void MainWindow::setupUI()
{
    m_actionManager->createActions();
    m_actionManager->buildMenus(menuBar());
    m_actionManager->buildToolBar();
    m_actionManager->buildStatusBar(statusBar(), m_projectPanel, m_terminalPanel);

    m_actionManager->menuBarAct()->setChecked(SettingsManager::instance().showMenuBar());
    m_actionManager->toolBarAct()->setChecked(SettingsManager::instance().showToolbar());
    m_actionManager->statusBarAct()->setChecked(SettingsManager::instance().showStatusbar());

    auto *menuBarShortcut = new QShortcut(QKeySequence("Ctrl+Alt+M"), this);
    connect(menuBarShortcut, &QShortcut::activated, this, [this]() {
        bool visible = !menuBar()->isVisible();
        menuBar()->setVisible(visible);
        m_actionManager->menuBarAct()->setChecked(visible);
        SettingsManager::instance().setShowMenuBar(visible);
    });

    auto *toolBarShortcut = new QShortcut(QKeySequence("Ctrl+Alt+T"), this);
    connect(toolBarShortcut, &QShortcut::activated, m_actionManager->toolBarAct(), &QAction::trigger);

    auto *statusBarShortcut = new QShortcut(QKeySequence("Ctrl+Shift+Alt+S"), this);
    connect(statusBarShortcut, &QShortcut::activated, m_actionManager->statusBarAct(), &QAction::trigger);

    const auto shortcutActions = m_actionManager->actionsWithShortcuts();
    for (QAction *act : shortcutActions)
        addAction(act);
}

void MainWindow::wireActions()
{
    ActionTargets targets;
    targets.editorController = m_editorController;
    targets.terminalPanel = m_terminalPanel;
    targets.splitView = m_splitView;
    targets.encodingManager = m_encodingManager;
    targets.mainWindow = this;
    targets.openRecentFile = [this](const QString &filePath) { loadFile(filePath); };
    targets.clearRecentFiles = [this]() { updateRecentFileActions(); };
    targets.updateRecentFileActions = [this]() { updateRecentFileActions(); };
    targets.encodingSelected = [this](const QString &encoding, bool reopen) {
        if (reopen)
            reloadCurrentFileWithEncoding(encoding);
        else
            saveCurrentFileWithEncoding(encoding);
    };
    targets.tabPinToggled = [this](int localIdx, QTabWidget *pane) {
        auto *w = pane->widget(localIdx);
        auto *editor = w ? w->findChild<CodeEditor*>() : nullptr;
        if (editor)
            m_tabManager->setTabPinned(editor, !m_tabManager->isTabPinned(editor));
    };
    m_actionManager->wireConnections(targets);
}

void MainWindow::connectPanelSignals()
{
    connect(m_tabManager, &TabManager::editorCreated, this, [this](CodeEditor* editor)
            {
                onEditorCreated(editor);
                connect(editor, &CodeEditor::findRequested, this, &MainWindow::find);
                connect(editor, &CodeEditor::replaceRequested, this, &MainWindow::replace);
                connect(editor, &CodeEditor::goToLineRequested, this, &MainWindow::goToLine);
            });
    connect(m_tabManager, &TabManager::editorClosed, this,
            [this](CodeEditor* editor)
            {
                QString filePath = editor->fileName();
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
    connect(m_projectPanel, &ProjectPanel::fileDoubleClicked, this, &MainWindow::openFileFromProject);
    connect(m_projectPanel, &ProjectPanel::folderSelected, this, &MainWindow::openFolderFromPath);

    connect(m_projectPanel, &QDockWidget::visibilityChanged, m_actionManager->projectPanelAct(), &QAction::setChecked);
    connect(m_projectPanel, &QDockWidget::visibilityChanged, m_actionManager->projectPanelButton(), &QToolButton::setChecked);
    connect(m_terminalPanel, &QDockWidget::visibilityChanged, this,
            [this](bool visible) {
                if (SettingsManager::instance().terminalPanelPosition() != TerminalPanelPosition::Tab) {
                    m_actionManager->terminalPanelAct()->setChecked(visible);
                    m_actionManager->terminalPanelButton()->setChecked(visible);
                    SettingsManager::instance().setShowTerminalPanel(visible);
                }
            });
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
                connect(editor, &CodeEditor::fileDropped, this, [this](const QString &path) {
                    loadFile(path);
                });
                connect(editor, &CodeEditor::navigateToLocation, this, &MainWindow::onNavigateToLocation);
                connect(editor, &CodeEditor::diagnosticsChanged, this, [this](const QString& uri, const QList<lsp::Diagnostic>& diags) {
                    m_errorListPanel->updateDiagnostics(uri, diags);
                });

                QString filePath = editor->fileName();
                if (!filePath.isEmpty() && filePath != Strings::untitled()) {
                    QString syntax = SettingsManager::instance().syntaxForFile(filePath);
                    if (!syntax.isEmpty()) {
                        editor->setLspLanguage(syntax);
                        editor->setLspServerManager(m_lspServerManager);
                        QPointer<CodeEditor> guard(editor);
                        QTimer::singleShot(200, this, [guard]() {
                            if (guard) guard->sendDidOpen();
                        });
                    }
                }
            });

    connect(m_editorController, &EditorController::fileSaved, this,
            [this](const QString& fileName) {
        CodeEditor* editor = m_editorController->findEditorByFileName(fileName);
        if (!editor) return;

        if (!editor->lspActive()) {
            QString syntax = SettingsManager::instance().syntaxForFile(fileName);
            if (!syntax.isEmpty()) {
                editor->setLspLanguage(syntax);
                editor->setLspServerManager(m_lspServerManager);
                QPointer<CodeEditor> guard(editor);
                QTimer::singleShot(200, this, [guard]() {
                    if (guard) guard->sendDidOpen();
                });
            }
        } else {
            QString uri = lsp::uriFromPath(fileName);
            m_lspServerManager->saveDocument(uri);
        }
    });

    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout, m_editorController, &EditorController::autoSave);
}

void MainWindow::setupLspConnections()
{
    connect(m_lspServerManager, &lsp::LspServerManager::definitionReady, this,
            [this](const QString&, const lsp::Location& location)
            {
                QString filePath = lsp::pathFromUri(location.uri);
                onNavigateToLocation(filePath, location.range.start.line, location.range.start.character);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::completionReady, this,
            [this](const QString& uri, const lsp::CompletionList& completions)
            {
                Q_UNUSED(uri)
                CodeEditor* editor = m_tabManager->currentEditor();
                if (editor)
                    editor->showCompletion(completions);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::typeDefinitionReady, this,
            [this](const QString&, const lsp::Location& location)
            {
                QString filePath = lsp::pathFromUri(location.uri);
                onNavigateToLocation(filePath, location.range.start.line, location.range.start.character);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::declarationReady, this,
            [this](const QString&, const lsp::Location& location)
            {
                QString filePath = lsp::pathFromUri(location.uri);
                onNavigateToLocation(filePath, location.range.start.line, location.range.start.character);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::codeActionReady, this,
            [this](const QString& uri, const QList<QJsonObject>& actions)
            {
                if (actions.isEmpty()) return;
                CodeEditor* editor = m_tabManager->findEditorByFileName(lsp::pathFromUri(uri));
                if (!editor) return;

                QMenu menu(editor);
                for (const auto& a : actions) {
                    QString title = a["title"].toString();
                    QAction* act = menu.addAction(title);
                    QJsonObject cmd = a["command"].toObject();
                    QString command = cmd["command"].toString();
                    QJsonArray args = cmd["arguments"].toArray();
                    act->setData(command);
                }
                QAction* chosen = menu.exec(QCursor::pos());
                if (chosen && !chosen->data().toString().isEmpty()) {
#ifdef QT_DEBUG
                    qDebug() << "Code action:" << chosen->data().toString();
#endif
                }
            });

    connect(m_lspServerManager, &lsp::LspServerManager::referencesReady, this,
            [this](const QString&, const QList<lsp::Location>& locations)
            {
                if (locations.isEmpty()) return;
                const auto& loc = locations.first();
                QString filePath = lsp::pathFromUri(loc.uri);
                onNavigateToLocation(filePath, loc.range.start.line, loc.range.start.character);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::hoverReady, this,
            [this](const QString& uri, const QString& contents)
            {
                Q_UNUSED(uri)
                CodeEditor* editor = m_tabManager->currentEditor();
                if (editor && !contents.isEmpty()) {
                    int pos = editor->cursorPosition();
                    editor->showToolTip(pos, contents);
                }
            });

    connect(m_lspServerManager, &lsp::LspServerManager::selectionRangesReady, this,
            [this](const QString& uri, const QJsonArray& ranges)
            {
                Q_UNUSED(uri)
                CodeEditor* editor = m_tabManager->currentEditor();
                if (editor)
                    editor->setSelectionRanges(ranges);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::semanticTokensFullReady, this,
            [this](const QString& uri, const QJsonArray& tokens)
            {
                CodeEditor* editor = m_tabManager->findEditorByFileName(lsp::pathFromUri(uri));
                if (editor)
                    editor->applySemanticTokens(uri, tokens);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::linkedEditingRangeReady, this,
            [this](const QString& uri, const QJsonObject& result)
            {
                CodeEditor* editor = m_tabManager->findEditorByFileName(lsp::pathFromUri(uri));
                if (editor)
                    editor->setLinkedEditingRanges(result);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::formattingReady, this,
            [this](const QString& uri, const QList<QJsonObject>& edits)
            {
                CodeEditor* editor = m_tabManager->findEditorByFileName(lsp::pathFromUri(uri));
                if (editor)
                    editor->applyFormattingEdits(edits);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::rangeFormattingReady, this,
            [this](const QString& uri, const QList<QJsonObject>& edits)
            {
                CodeEditor* editor = m_tabManager->findEditorByFileName(lsp::pathFromUri(uri));
                if (editor)
                    editor->applyFormattingEdits(edits);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::signatureHelpReady, this,
            [this](const QString& uri, const QJsonObject& info)
            {
                Q_UNUSED(uri)
                CodeEditor* editor = m_tabManager->currentEditor();
                if (editor)
                    editor->showSignatureHelp(info);
            });

    connect(m_lspServerManager, &lsp::LspServerManager::documentHighlightReady, this,
            [this](const QString& uri, const QJsonArray& highlights)
            {
                CodeEditor* editor = m_tabManager->findEditorByFileName(lsp::pathFromUri(uri));
                if (editor) {
                    editor->applyHighlights(highlights);
                }
            });

    connect(m_lspServerManager, &lsp::LspServerManager::diagnosticsReady, this,
            [this](const QString& uri, const QList<lsp::Diagnostic>& diagnostics)
            {
                CodeEditor* editor = m_tabManager->findEditorByFileName(lsp::pathFromUri(uri));
                if (editor) {
                    editor->applyDiagnostics(uri, diagnostics);
                }
                m_errorListPanel->updateDiagnostics(uri, diagnostics);
            });

    connect(m_errorListPanel, &ErrorListPanel::navigateToLocation, this, &MainWindow::onNavigateToLocation);

    if (m_actionManager->errorListPanelAct())
        connect(m_errorListPanel, &QDockWidget::visibilityChanged, m_actionManager->errorListPanelAct(), &QAction::setChecked);
    connect(m_errorListPanel, &QDockWidget::visibilityChanged, m_actionManager->errorListPanelButton(), &QToolButton::setChecked);

    connect(&SettingsManager::instance(), &SettingsManager::settingsChanged, this,
            [this]() {
        for (const QString& lang : m_lspServerManager->languages()) {
            auto* client = m_lspServerManager->clientForLanguage(lang);
            if (client)
                client->sendDidChangeConfiguration(QJsonObject());
        }
    });
}

void MainWindow::applyCloseButtonPosition()
{
    m_tabManager->updateAllCloseButtons(SettingsManager::instance().closeButtonMode());
    for (QTabWidget* pane : m_splitView->findChildren<QTabWidget*>())
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
    for (QTabWidget* pane : m_tabManager->panes())
    {
        pane->setTabPosition(tp);
    }
}

void MainWindow::applyTerminalPanelPosition()
{
    m_terminalPanel->applyPosition(SettingsManager::instance().terminalPanelPosition(), m_tabWidget, this);
    bool visible = SettingsManager::instance().showTerminalPanel();
    m_actionManager->terminalPanelAct()->setChecked(visible);
    m_actionManager->terminalPanelButton()->setChecked(visible);
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

    CodeEditor* editor = m_tabManager->createEditor();
    editor->setFileName(urlStr);
    editor->setText(content);
    editor->setModified(false);

    m_editorController->connectEditorSignals(editor);

    SettingsManager::instance().applyToEditor(editor);

    QString syntax = getSyntaxForFile(displayName);
    if (!syntax.isEmpty())
    {
        editor->setSyntax(syntax);
    }

    m_tabManager->addEditor(editor, displayName);
    m_editorController->updateUndoRedoState();
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

    m_projectPanel->setRootPath(dirPath);
    m_projectPanel->show();
    m_actionManager->projectPanelAct()->setChecked(true);
    m_terminalPanel->setWorkingDirectory(dirPath);
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

void MainWindow::openFolderFromPath(const QString& folderPath)
{
    m_projectPanel->setRootPath(folderPath);
    m_projectPanel->show();
    m_actionManager->projectPanelAct()->setChecked(true);
    m_terminalPanel->setWorkingDirectory(folderPath);
    Logger::instance().info(QString("Opened folder: %1").arg(folderPath));
}

void MainWindow::loadFile(const QString& fileName, const QString& encoding)
{
    CodeEditor* existingEditor = m_tabManager->findEditorByFileName(fileName);
    if (existingEditor)
    {
        for (QTabWidget* pane : m_tabManager->panes())
        {
            int index = pane->indexOf(existingEditor);
            if (index >= 0)
            {
                pane->setCurrentIndex(index);
                existingEditor->setFocus();
                m_tabManager->setActivePane(pane);
                return;
            }
        }
        return;
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
    QString appPath = QApplication::applicationFilePath();
    if (!QProcess::startDetached(appPath, QStringList()))
    {
        Logger::instance().error("Failed to start detached process for new window");
    }
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
    if (!m_findInFilesDialog)
    {
        QString defaultDir;
        if (m_projectPanel->isVisible())
            defaultDir = m_projectPanel->rootPath();
        else
            defaultDir = QDir::homePath();

        m_findInFilesDialog = new FindInFilesDialog(defaultDir, this);
        connect(m_findInFilesDialog, &FindInFilesDialog::navigateToResult, this,
                [this](const QString& filePath, int lineNumber)
                {
                    CodeEditor* existing = m_tabManager->findEditorByFileName(filePath);
                    if (existing)
                    {
                        for (QTabWidget* pane : m_tabManager->panes())
                        {
                            int idx = pane->indexOf(existing);
                            if (idx >= 0)
                            {
                                pane->setCurrentIndex(idx);
                                m_tabManager->setActivePane(pane);
                                break;
                            }
                        }
                        existing->setCursorPosition(lineNumber - 1, 0);
                        existing->setFocus();
                        return;
                    }

                    loadFile(filePath);
                    CodeEditor* editor = m_tabManager->currentEditor();
                    if (editor)
                    {
                        editor->setCursorPosition(lineNumber - 1, 0);
                    }
                });
        connect(m_findInFilesDialog, &QDialog::finished, this,
                [this]()
                {
                    m_findInFilesDialog->deleteLater();
                    m_findInFilesDialog = nullptr;
                });
    }

    if (auto* editor = m_tabManager->currentEditor())
    {
        QString selected = editor->selectedText();
        if (!selected.isEmpty())
            m_findInFilesDialog->startSearch(selected, m_projectPanel->isVisible() ? m_projectPanel->rootPath() : QDir::homePath());
    }

    m_findInFilesDialog->show();
    m_findInFilesDialog->raise();
    m_findInFilesDialog->activateWindow();
}

void MainWindow::showOptions()
{
    OptionsDialog dlg(this);
    ThemeId originalTheme = SettingsManager::instance().theme();
    QColor originalAccent = SettingsManager::instance().accentColor();
    QString originalTermFontFamily = SettingsManager::instance().terminalFontFamily();
    int originalTermFontSize = SettingsManager::instance().terminalFontSize();

    connect(&dlg, &OptionsDialog::themeChanged, this, [this]() {
        applySettings();
        m_terminalPanel->refreshTheme();
    });

    if (dlg.exec() == QDialog::Accepted)
    {
        applySettings();
        applyCloseButtonPosition();
        applyTabBarPosition();
        applyTerminalPanelPosition();
        m_terminalPanel->refreshTheme();
    m_errorListPanel->setVisible(SettingsManager::instance().lspShowErrorList());
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
        m_terminalPanel->refreshTheme();
    }
}

void MainWindow::runExternalTool(int index)
{
    CodeEditor* editor = m_tabManager->currentEditor();
    QString filePath;
    int lineNumber = 1;
    QString selectedText;
    if (editor) {
        filePath = editor->fileName();
        int dummyIndex;
        editor->getCursorPosition(&lineNumber, &dummyIndex);
        lineNumber++;
        selectedText = editor->selectedText();
    }

    QString projectDir;
    if (m_projectPanel->isVisible() && !m_projectPanel->rootPath().isEmpty())
        projectDir = m_projectPanel->rootPath();

    m_externalToolManager->runTool(index, filePath, projectDir, selectedText, lineNumber,
        [this](const QString& cmd) {
            if (!m_terminalPanel->isVisible()) {
                m_terminalPanel->toggle(m_tabWidget, this);
                m_terminalPanel->show();
            }
            m_terminalPanel->sendCommand(cmd);
        }, this);
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
            QString lastFile = recentFiles.first();
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
        if (!terminalDir.isEmpty())
            m_terminalPanel->setWorkingDirectory(terminalDir);
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
    int numRecentFiles = qMin(recentFiles.size(), (qsizetype)10);

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
    loadFile(fileName, encoding);
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
    m_terminalPanel->toggle(m_tabWidget, this);
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

void MainWindow::findSymbols()
{
    if (!m_findSymbolsDialog) {
        m_findSymbolsDialog = new FindSymbolsDialog(m_lspServerManager, this);
        connect(m_findSymbolsDialog, &FindSymbolsDialog::navigateToSymbol, this, &MainWindow::onNavigateToLocation);
    }
    m_findSymbolsDialog->show();
    m_findSymbolsDialog->raise();
    m_findSymbolsDialog->activateWindow();
    m_findSymbolsDialog->setFocus();
}

void MainWindow::expandSelection()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
        editor->expandSelection();
}

void MainWindow::shrinkSelection()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
        editor->shrinkSelection();
}

void MainWindow::updateStatusBarLabelsVisibility()
{
    bool hasEditor = m_tabManager->count() > 0;
    QTabWidget* activePane = m_tabManager->activePane();
    bool isTerminalTab = (activePane != nullptr) &&
                         (SettingsManager::instance().terminalPanelPosition() == TerminalPanelPosition::Tab) &&
                         (activePane->currentWidget() == m_terminalPanel);
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

    SettingsManager::instance().setShowTerminalPanel(m_actionManager->terminalPanelAct()->isChecked());
    m_sessionManager->saveSession(m_tabManager, m_projectPanel, m_terminalPanel->workingDirectory());
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
    for (const QString& line : QString::fromUtf8(pgrep.readAllStandardOutput())
             .split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        qint64 pid = line.trimmed().toLongLong();
        if (pid > 0 && pid != QCoreApplication::applicationPid())
            pids.append(pid);
    }

    for (qint64 pid : pids) {
        QLocalSocket socket;
        socket.connectToServer(QStringLiteral("devpad-%1").arg(pid));
        if (!socket.waitForConnected(1000))
            continue;

        socket.write("save_and_quit\n");
        socket.flush();

        if (socket.waitForReadyRead(1000)) {
            while (socket.canReadLine()) {
                QString line = QString::fromUtf8(socket.readLine()).trimmed();
                if (line == QStringLiteral("abort")) {
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
    for (int i = m_tabManager->count() - 1; i >= 0; --i) {
        CodeEditor* editor = m_tabManager->editorAt(i);
        if (!m_editorController->maybeSave(editor)) {
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
    connect(client, &QLocalSocket::readyRead, this, [this, client]() {
        if (!client->canReadLine())
            return;

        QString command = QString::fromUtf8(client->readLine()).trimmed();
        if (command != QStringLiteral("save_and_quit"))
            return;

        if (m_quitRequested) {
            client->disconnectFromServer();
            return;
        }
        m_quitRequested = true;

        if (!saveAllModifiedTabs()) {
            client->write("abort\n");
            client->flush();
            client->disconnectFromServer();
            return;
        }

        SettingsManager::instance().setShowTerminalPanel(m_actionManager->terminalPanelAct()->isChecked());
        m_sessionManager->saveSession(m_tabManager, m_projectPanel, m_terminalPanel->workingDirectory());
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
    SettingsManager::instance().setShowTerminalPanel(m_actionManager->terminalPanelAct()->isChecked());
    m_sessionManager->saveSession(m_tabManager, m_projectPanel, m_terminalPanel->workingDirectory());
    event->accept();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        m_actionManager->fullScreenAct()->setChecked(isFullScreen());
    }
    else if (event->type() == QEvent::ThemeChange)
    {
        if (prefersNativeStyling(SettingsManager::instance().theme()))
        {
            window()->setStyleSheet(QString());

            for (int i = 0; i < m_tabManager->count(); ++i)
            {
                CodeEditor* editor = m_tabManager->editorAt(i);
                if (editor) {
                    editor->applyTheme(ThemeId::System);
                }
            }

            m_terminalPanel->refreshTheme();
        }
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

void MainWindow::goToDefinition()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
        editor->goToDefinition();
}

void MainWindow::formatSelection()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
        editor->formatSelection();
}

void MainWindow::goToTypeDefinition()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
        editor->goToTypeDefinition();
}

void MainWindow::goToDeclaration()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
        editor->goToDeclaration();
}

void MainWindow::renameSymbol()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
        editor->requestRename();
}

void MainWindow::findReferences()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
        editor->findReferences();
}

void MainWindow::triggerCompletion()
{
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor)
        editor->triggerCompletion();
}

void MainWindow::onNavigateToLocation(const QString& filePath, int line, int column)
{
    if (filePath.isEmpty())
        return;

    CodeEditor* existing = m_tabManager->findEditorByFileName(filePath);
    if (existing) {
        for (QTabWidget* pane : m_tabManager->panes()) {
            int idx = pane->indexOf(existing);
            if (idx >= 0) {
                pane->setCurrentIndex(idx);
                m_tabManager->setActivePane(pane);
                break;
            }
        }
        existing->setCursorPosition(line, column);
        existing->setFocus();
        return;
    }

    loadFile(filePath);
    CodeEditor* editor = m_tabManager->currentEditor();
    if (editor) {
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
    QPointer<CodeEditor> editor = m_tabManager->findEditorByFileName(filePath);
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
        if (!editor)
            return;

        QList<int> bookmarks = editor->bookmarkLines();
        QString encoding = editor->encoding();

        int idx = m_tabManager->indexOf(editor);
        if (idx >= 0)
        {
            m_tabManager->closeEditor(idx);
        }
        m_fileWatcherManager->unwatchFile(filePath);
        loadFile(filePath, encoding);
        if (auto* newEditor = m_tabManager->currentEditor())
        {
            newEditor->setBookmarks(bookmarks);
        }
    }
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
