#ifndef DEBUGPANEL_H
#define DEBUGPANEL_H

#include "dap/daptypes.h"

#include <QDockWidget>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QToolBar>
#include <QTreeView>

namespace dap
{
class DapServerManager;
}

// Debug tool window: call stack + variables for the active debug session.
// Connects to DapServerManager to drive execution and display state.
class DebugPanel : public QDockWidget
{
    Q_OBJECT

public:
    explicit DebugPanel(QWidget* parent = nullptr);
    ~DebugPanel() override;

    void setServerManager(dap::DapServerManager* manager);
    void setSessionKey(const QString& sessionKey);

    // Refresh the call stack and variables for the current stopped state.
    void refreshStack();

public slots:
    void continueDebug();
    void stepOver();
    void stepInto();
    void stepOut();
    void stopDebug();

signals:
    void navigateToLocation(const QString& filePath, int line, int column);

private slots:
    void onStackTraceReady(const QList<dap::StackFrame>& frames, int totalFrames);
    void onScopesReady(const QList<dap::Scope>& scopes);
    void onVariablesReady(const QList<dap::Variable>& variables);
    void onStopped(const dap::StoppedEvent& ev);
    void onTerminated();
    void onContinueDone(int threadId, bool allThreadsContinued);
    void onSessionStarted();
    void onSessionStopped();
    void onFrameSelected(const QModelIndex& current, const QModelIndex& previous);
    void onContinueClicked();
    void onStepOverClicked();
    void onStepIntoClicked();
    void onStepOutClicked();
    void onStopClicked();

public:
    void updateState(bool debugging, const QString& status);

private:
    void setupUi();
    void populateStack(const QList<dap::StackFrame>& frames);
    void populateVariables(const QList<dap::Variable>& variables);
    void loadScopesForFrame(int frameId);
    void loadVariablesForScope(int variablesReference);

    dap::DapServerManager* m_dapManager = nullptr;
    QString m_sessionKey;

    QToolBar* m_toolBar = nullptr;
    QLabel* m_stateLabel = nullptr;
    QAction* m_btnContinue = nullptr;
    QAction* m_btnStepOver = nullptr;
    QAction* m_btnStepInto = nullptr;
    QAction* m_btnStepOut = nullptr;
    QAction* m_btnStop = nullptr;

    QTreeView* m_stackView = nullptr;
    QStandardItemModel* m_stackModel = nullptr;

    QTreeView* m_varsView = nullptr;
    QStandardItemModel* m_varsModel = nullptr;

    int m_selectedFrameId = 0;
    int m_selectedScopeRef = 0;
    int m_stoppedThreadId = 0;
};

#endif // DEBUGPANEL_H
