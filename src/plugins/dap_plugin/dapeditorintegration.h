#ifndef DAPEDITORINTEGRATION_H
#define DAPEDITORINTEGRATION_H

#include "daptypes.h"

#include <QList>
#include <QObject>
#include <QString>

class CodeEditor;

namespace dap
{

class DapServerManager;
class DapClient;

// Per-editor DAP integration: syncs breakpoint margins with the debug adapter
// and issues execution-control requests (continue, step, pause, terminate)
// for the editor's file.
class DapEditorIntegration : public QObject
{
    Q_OBJECT

public:
    explicit DapEditorIntegration(CodeEditor* editor, QObject* parent = nullptr);
    ~DapEditorIntegration() override;

    void setServerManager(DapServerManager* manager);
    DapServerManager* serverManager() const
    {
        return m_dapManager;
    }

    void setSessionKey(const QString& sessionKey);
    const QString& sessionKey() const
    {
        return m_sessionKey;
    }
    bool isActive() const
    {
        return m_dapActive;
    }

public slots:
    // Sync the editor's breakpoint margin with the DAP client for this file.
    void syncBreakpoints();
    void setBreakpointsFromDap(const QList<int>& lines);

    // Execution control on the active session.
    void continueExecution();
    void stepOver();
    void stepInto();
    void stepOut();
    void pause();
    void terminate();

    // Called when the adapter reports a stop: highlights the stopped line.
    void onStopped(const StoppedEvent& ev);
    void onTerminated();
    void onThreadEvent(const ThreadEvent& ev);

    CodeEditor* editor() const
    {
        return m_editor;
    }

signals:
    void navigateToLocation(const QString& filePath, int line, int column);
    void debuggingStateChanged(bool debugging);

private:
    DapClient* clientForSession() const;
    QString filePath() const;
    QString currentUri() const;
    void connectClientSignals();

    CodeEditor* m_editor;
    DapServerManager* m_dapManager = nullptr;
    QString m_sessionKey;
    bool m_dapActive = false;
};

} // namespace dap

#endif // DAPEDITORINTEGRATION_H
