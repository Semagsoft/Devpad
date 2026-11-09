#ifndef DAPCLIENT_H
#define DAPCLIENT_H

#include "daptypes.h"

#include "../lsp/lspjsonrpc.h"

#include <QObject>
#include <QProcess>
#include <QTimer>

namespace dap
{

// Debug Adapter Protocol client. Speaks DAP over stdio to a debug adapter
// (e.g. lldb-vscode, codelldb, python's debugpy, vscode-chrome-debug-core).
// Framing is the same Content-Length JSON-RPC used by LSP, so LspJsonRpc is
// reused directly.
//
// Lifecycle:
//   initialize -> (capabilities) -> launch/attach -> configurationDone
//             -> [threads/stackTrace/scopes/variables while stopped]
//             -> continue/step*/pause -> ... -> terminate/disconnect
//
// Breakpoints are managed by setBreakpoints per source file.
class DapClient : public QObject
{
    Q_OBJECT

public:
    explicit DapClient(QObject* parent = nullptr);
    ~DapClient() override;

    void startServer(const QString& command, const QStringList& args);
    void stopServer();
    bool isRunning() const;

    // ── Handshake ───────────────────────────────────────────────
    void sendInitialize(const QString& clientName = QStringLiteral("Devpad"),
                        const QString& adapterID = QString(),
                        const QJsonObject& pathFormat = QJsonObject());

    // ── Session control ─────────────────────────────────────────
    // Programmatic launch (no debug UI yet). Arguments are passed through
    // verbatim; subclasses/UI fill in program/args/cwd/env.
    void launch(const QJsonObject& arguments);
    void attach(const QJsonObject& arguments);
    void disconnectDebug(bool terminateDebuggee = true);
    void terminate();

    // ── Breakpoints ─────────────────────────────────────────────
    // sourcePath: file path on disk. lines: 0-based breakpoint lines.
    void setSourceBreakpoints(const QString& sourcePath, const QList<int>& lines,
                              const QJsonObject& sourceParams = QJsonObject());
    void setExceptionBreakpoints(const QStringList& filters,
                                 const QJsonObject& filterOptions = QJsonObject());

    // ── Execution control ───────────────────────────────────────
    void configurationDone();
    void threads();
    void stackTrace(int threadId, int startFrame = 0, int levels = 0);
    void scopes(int frameId);
    void variables(int variablesReference, int start = 0, int count = 0);
    void continueExecution(int threadId);
    void stepOver(int threadId);
    void pause(int threadId);
    void stepInto(int threadId, int targetId = 0);
    void stepOut(int threadId, int targetId = 0);
    void stepBack(int threadId);
    void reverseContinue(int threadId);

signals:
    // Lifecycle
    void serverStarted();
    void serverStopped();
    void serverError(const QString& error);

    // initialize handshake
    void initializeDone(const Capabilities& capabilities);

    // launch / attach
    void launchDone();
    void attachDone();

    // Breakpoints
    void breakpointsSet(const QString& sourcePath, const QList<Breakpoint>& breakpoints);
    void exceptionBreakpointsSet();

    // Events from the adapter
    void stopped(const StoppedEvent& ev);
    void terminated(const TerminatedEvent& ev);
    void threadEvent(const ThreadEvent& ev);
    void breakpointEvent(const BreakpointEvent& ev);
    void output(const OutputEvent& ev);
    void capabilitiesEvent(const QJsonObject& capabilities);

    // Data results
    void threadsReady(const QList<Thread>& threads);
    void stackTraceReady(const QList<StackFrame>& frames, int totalFrames);
    void scopesReady(const QList<Scope>& scopes);
    void variablesReady(const QList<Variable>& variables);
    void continueDone(int threadId, bool allThreadsContinued);

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    void sendInitializeHandshake();
    void sendTerminateIfStopping();
    int sendRequest(const QString& method, const QJsonObject& params,
                    lsp::LspJsonRpc::ResponseCallback callback);

    void handleNotification(const QString& method, const QJsonObject& params);
    void handleResponse(const QVariant& id, const QJsonValue& result, const QJsonObject& error);
    void handleServerRequest(const QVariant& id, const QString& method, const QJsonObject& params);

    QProcess* m_process = nullptr;
    lsp::LspJsonRpc* m_jsonRpc = nullptr;
    QString m_command;
    QStringList m_args;
    Capabilities m_capabilities;
    bool m_initialized = false;
    bool m_starting = false;
    bool m_stopping = false;
    int m_retryCount = 0;
    static constexpr int START_TIMEOUT_MS = 5000;
    static constexpr int SHUTDOWN_TIMEOUT_MS = 5000;
    static constexpr int KILL_TIMEOUT_MS = 2000;
};

} // namespace dap

#endif // DAPCLIENT_H
