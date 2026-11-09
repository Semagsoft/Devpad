#include "debugpanel.h"

#include "dap/dapclient.h"
#include "dap/dapservermanager.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

DebugPanel::DebugPanel(QWidget* parent) : QDockWidget(tr("Debug"), parent)
{
    setObjectName("DebugPanel");
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
    setupUi();
    hide();
}

DebugPanel::~DebugPanel() = default;

void DebugPanel::setupUi()
{
    setTitleBarWidget(new QWidget(this));
    QWidget* content = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // ── Toolbar ─────────────────────────────────────────────────
    m_toolBar = new QToolBar(this);
    m_toolBar->setMovable(false);
    m_toolBar->setIconSize(QSize(16, 16));

    m_btnContinue =
        m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("media-playback-start")), tr("Continue"), this, &DebugPanel::onContinueClicked);
    m_btnStepOver =
        m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("media-skip-forward")), tr("Step Over"), this, &DebugPanel::onStepOverClicked);
    m_btnStepInto =
        m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("media-seek-forward")), tr("Step Into"), this, &DebugPanel::onStepIntoClicked);
    m_btnStepOut = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("media-skip-backward")), tr("Step Out"), this, &DebugPanel::onStepOutClicked);
    m_btnStop = m_toolBar->addAction(QIcon::fromTheme(QStringLiteral("media-playback-stop")), tr("Stop"), this, &DebugPanel::onStopClicked);

    m_stateLabel = new QLabel(tr("Not debugging"), this);
    m_stateLabel->setStyleSheet(QStringLiteral("QLabel { color: palette(text); }"));
    m_toolBar->addWidget(m_stateLabel);
    m_toolBar->addSeparator();

    layout->addWidget(m_toolBar);

    // ── Splitter: call stack above, variables below ─────────────
    QSplitter* splitter = new QSplitter(Qt::Vertical, content);

    m_stackModel = new QStandardItemModel(0, 2, splitter);
    m_stackModel->setHorizontalHeaderLabels({tr("Frame"), tr("Location")});

    m_stackView = new QTreeView(splitter);
    m_stackView->setModel(m_stackModel);
    m_stackView->setRootIsDecorated(false);
    m_stackView->setAlternatingRowColors(true);
    m_stackView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_stackView->header()->setStretchLastSection(true);
    m_stackView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_stackView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    splitter->addWidget(m_stackView);

    m_varsModel = new QStandardItemModel(0, 3, splitter);
    m_varsModel->setHorizontalHeaderLabels({tr("Name"), tr("Value"), tr("Type")});

    m_varsView = new QTreeView(splitter);
    m_varsView->setModel(m_varsModel);
    m_varsView->setRootIsDecorated(false);
    m_varsView->setAlternatingRowColors(true);
    m_varsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_varsView->header()->setStretchLastSection(true);
    m_varsView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_varsView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_varsView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    splitter->addWidget(m_varsView);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter);

    setWidget(content);

    // Connections
    connect(m_stackView->selectionModel(), &QItemSelectionModel::currentChanged, this, &DebugPanel::onFrameSelected);
}

void DebugPanel::setServerManager(dap::DapServerManager* manager)
{
    m_dapManager = manager;
    if (!m_dapManager)
        return;

    connect(m_dapManager, &dap::DapServerManager::stackTraceReady, this,
            [this](const QString& sessionKey, const QList<dap::StackFrame>& frames, int totalFrames)
            {
                if (sessionKey == m_sessionKey)
                    onStackTraceReady(frames, totalFrames);
            });
    connect(m_dapManager, &dap::DapServerManager::scopesReady, this,
            [this](const QString& sessionKey, const QList<dap::Scope>& scopes)
            {
                if (sessionKey == m_sessionKey)
                    onScopesReady(scopes);
            });
    connect(m_dapManager, &dap::DapServerManager::variablesReady, this,
            [this](const QString& sessionKey, const QList<dap::Variable>& variables)
            {
                if (sessionKey == m_sessionKey)
                    onVariablesReady(variables);
            });
    connect(m_dapManager, &dap::DapServerManager::stopped, this,
            [this](const QString& sessionKey, const dap::StoppedEvent& ev)
            {
                if (sessionKey == m_sessionKey)
                    onStopped(ev);
            });
    connect(m_dapManager, &dap::DapServerManager::terminated, this,
            [this](const QString& sessionKey, const dap::TerminatedEvent& ev)
            {
                Q_UNUSED(ev);
                if (sessionKey == m_sessionKey)
                    onTerminated();
            });
    connect(m_dapManager, &dap::DapServerManager::continueDone, this,
            [this](const QString& sessionKey, int threadId, bool allThreadsContinued)
            {
                Q_UNUSED(threadId);
                Q_UNUSED(allThreadsContinued);
                if (sessionKey == m_sessionKey)
                    onContinueDone(threadId, allThreadsContinued);
            });
    connect(m_dapManager, &dap::DapServerManager::clientStarted, this,
            [this](const QString& sessionKey)
            {
                if (sessionKey == m_sessionKey)
                    onSessionStarted();
            });
    connect(m_dapManager, &dap::DapServerManager::clientStopped, this,
            [this](const QString& sessionKey)
            {
                if (sessionKey == m_sessionKey)
                    onSessionStopped();
            });
}

void DebugPanel::setSessionKey(const QString& sessionKey)
{
    m_sessionKey = sessionKey;
    m_stackModel->removeRows(0, m_stackModel->rowCount());
    m_varsModel->removeRows(0, m_varsModel->rowCount());
    m_selectedFrameId = 0;
    m_selectedScopeRef = 0;
    updateState(!sessionKey.isEmpty() && m_dapManager && m_dapManager->isDebugging(sessionKey),
                sessionKey.isEmpty() ? tr("Not debugging") : tr("Debugging: %1").arg(sessionKey));
    setVisible(!sessionKey.isEmpty());
    if (!sessionKey.isEmpty())
        refreshStack();
}

void DebugPanel::updateState(bool debugging, const QString& status)
{
    m_stateLabel->setText(status);
    bool enabled = debugging;
    m_btnContinue->setEnabled(enabled);
    m_btnStepOver->setEnabled(enabled);
    m_btnStepInto->setEnabled(enabled);
    m_btnStepOut->setEnabled(enabled);
    m_btnStop->setEnabled(enabled);
}

void DebugPanel::refreshStack()
{
    if (!m_dapManager || m_sessionKey.isEmpty())
        return;
    dap::DapClient* client = m_dapManager->clientForSession(m_sessionKey);
    if (!client)
        return;
    client->threads();
}

void DebugPanel::populateStack(const QList<dap::StackFrame>& frames)
{
    m_stackModel->removeRows(0, m_stackModel->rowCount());
    for (const auto& f : frames)
    {
        QList<QStandardItem*> row;
        auto* nameItem = new QStandardItem(f.name);
        nameItem->setEditable(false);
        nameItem->setData(f.id, Qt::UserRole);
        QString loc;
        if (!f.sourcePath.isEmpty())
            loc = QStringLiteral("%1:%2").arg(QFileInfo(f.sourcePath).fileName()).arg(f.line + 1);
        else
            loc = QStringLiteral("%1:%2").arg(f.sourceName).arg(f.line + 1);
        auto* locItem = new QStandardItem(loc);
        locItem->setEditable(false);
        row << nameItem << locItem;
        m_stackModel->appendRow(row);
    }
    if (!frames.isEmpty())
        m_stackView->setCurrentIndex(m_stackModel->index(0, 0));
}

void DebugPanel::populateVariables(const QList<dap::Variable>& variables)
{
    m_varsModel->removeRows(0, m_varsModel->rowCount());
    for (const auto& v : variables)
    {
        QList<QStandardItem*> row;
        row << new QStandardItem(v.name) << new QStandardItem(v.value) << new QStandardItem(v.type);
        for (auto* item : row)
            item->setEditable(false);
        m_varsModel->appendRow(row);
    }
}

void DebugPanel::loadScopesForFrame(int frameId)
{
    if (!m_dapManager || m_sessionKey.isEmpty())
        return;
    dap::DapClient* client = m_dapManager->clientForSession(m_sessionKey);
    if (client)
        client->scopes(frameId);
}

void DebugPanel::loadVariablesForScope(int variablesReference)
{
    if (!m_dapManager || m_sessionKey.isEmpty())
        return;
    dap::DapClient* client = m_dapManager->clientForSession(m_sessionKey);
    if (client)
        client->variables(variablesReference);
}

// ── Slots ───────────────────────────────────────────────────────

void DebugPanel::onStackTraceReady(const QList<dap::StackFrame>& frames, int /*totalFrames*/)
{
    populateStack(frames);
    if (!frames.isEmpty())
    {
        m_selectedFrameId = frames.first().id;
        const auto& top = frames.first();
        if (!top.sourcePath.isEmpty())
            emit navigateToLocation(top.sourcePath, top.line - 1, top.column - 1);
        loadScopesForFrame(m_selectedFrameId);
    }
}

void DebugPanel::onScopesReady(const QList<dap::Scope>& scopes)
{
    if (!scopes.isEmpty())
    {
        m_selectedScopeRef = scopes.first().variablesReference;
        loadVariablesForScope(m_selectedScopeRef);
    }
}

void DebugPanel::onVariablesReady(const QList<dap::Variable>& variables)
{
    populateVariables(variables);
}

void DebugPanel::onStopped(const dap::StoppedEvent& ev)
{
    m_stoppedThreadId = ev.threadId;
    updateState(true, tr("Paused: %1").arg(ev.text));
    dap::DapClient* client = m_dapManager ? m_dapManager->clientForSession(m_sessionKey) : nullptr;
    if (client)
        client->stackTrace(ev.threadId);
}

void DebugPanel::onTerminated()
{
    updateState(false, tr("Terminated"));
    m_stackModel->removeRows(0, m_stackModel->rowCount());
    m_varsModel->removeRows(0, m_varsModel->rowCount());
}

void DebugPanel::onContinueDone(int /*threadId*/, bool /*allThreadsContinued*/)
{
    updateState(true, tr("Running"));
}

void DebugPanel::onSessionStarted()
{
    updateState(true, tr("Starting..."));
}

void DebugPanel::onSessionStopped()
{
    updateState(false, tr("Not debugging"));
    m_stackModel->removeRows(0, m_stackModel->rowCount());
    m_varsModel->removeRows(0, m_varsModel->rowCount());
}

void DebugPanel::onFrameSelected(const QModelIndex& current, const QModelIndex& /*previous*/)
{
    Q_UNUSED(current);
    if (!current.isValid())
        return;
    QStandardItem* item = m_stackModel->item(current.row(), 0);
    if (!item)
        return;
    m_selectedFrameId = item->data(Qt::UserRole).toInt();
    loadScopesForFrame(m_selectedFrameId);
}

void DebugPanel::onContinueClicked()
{
    continueDebug();
}

void DebugPanel::onStepOverClicked()
{
    stepOver();
}

void DebugPanel::onStepIntoClicked()
{
    stepInto();
}

void DebugPanel::onStepOutClicked()
{
    stepOut();
}

void DebugPanel::onStopClicked()
{
    stopDebug();
}

// ── Public execution-control slots (forward to DAP client) ──────

void DebugPanel::continueDebug()
{
    if (!m_dapManager || m_sessionKey.isEmpty())
        return;
    dap::DapClient* client = m_dapManager->clientForSession(m_sessionKey);
    if (client)
        client->continueExecution(0);
}

void DebugPanel::stepOver()
{
    if (!m_dapManager || m_sessionKey.isEmpty())
        return;
    dap::DapClient* client = m_dapManager->clientForSession(m_sessionKey);
    if (client)
        client->stepOver(0);
}

void DebugPanel::stepInto()
{
    if (!m_dapManager || m_sessionKey.isEmpty())
        return;
    dap::DapClient* client = m_dapManager->clientForSession(m_sessionKey);
    if (client)
        client->stepInto(0);
}

void DebugPanel::stepOut()
{
    if (!m_dapManager || m_sessionKey.isEmpty())
        return;
    dap::DapClient* client = m_dapManager->clientForSession(m_sessionKey);
    if (client)
        client->stepOut(0);
}

void DebugPanel::stopDebug()
{
    if (!m_dapManager || m_sessionKey.isEmpty())
        return;
    dap::DapClient* client = m_dapManager->clientForSession(m_sessionKey);
    if (client)
        client->terminate();
}
