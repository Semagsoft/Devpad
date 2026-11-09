#include "dapeditorintegration.h"

#include "../codeeditor.h"
#include "dapclient.h"
#include "dapservermanager.h"
#include "daptypes.h"
#include "lsp/lsptypes.h"
#include "settingsmanager.h"

#include <QInputDialog>

namespace dap
{

DapEditorIntegration::DapEditorIntegration(CodeEditor* editor, QObject* parent)
    : QObject(parent), m_editor(editor)
{
}

DapEditorIntegration::~DapEditorIntegration() = default;

void DapEditorIntegration::setServerManager(DapServerManager* manager)
{
    m_dapManager = manager;
}

void DapEditorIntegration::setSessionKey(const QString& sessionKey)
{
    m_sessionKey = sessionKey;
    m_dapActive = !sessionKey.isEmpty();
}

DapClient* DapEditorIntegration::clientForSession() const
{
    if (!m_dapManager || m_sessionKey.isEmpty())
        return nullptr;
    return m_dapManager->clientForSession(m_sessionKey);
}

QString DapEditorIntegration::filePath() const
{
    return m_editor ? m_editor->fileName() : QString();
}

QString DapEditorIntegration::currentUri() const
{
    if (filePath().isEmpty())
        return QString();
    return lsp::uriFromPath(filePath());
}

void DapEditorIntegration::syncBreakpoints()
{
    DapClient* client = clientForSession();
    if (!client || filePath().isEmpty())
        return;

    QList<int> lines = m_editor->breakpointLines();
    client->setSourceBreakpoints(filePath(), lines);
}

void DapEditorIntegration::setBreakpointsFromDap(const QList<int>& lines)
{
    if (m_editor)
        m_editor->setBreakpoints(lines);
}

void DapEditorIntegration::continueExecution()
{
    DapClient* client = clientForSession();
    if (!client)
        return;
    // Resume all threads on the current session.
    client->threads(); // refresh threads first is optional; continue needs a threadId.
    // DAP `continue` requires a threadId; default to the first known thread.
    // We track the last stopped thread via the session; here we ask the client
    // to continue thread 0 as a reasonable default adapters accept.
    client->continueExecution(0);
}

void DapEditorIntegration::stepOver()
{
    DapClient* client = clientForSession();
    if (client)
        client->stepOver(0);
}

void DapEditorIntegration::stepInto()
{
    DapClient* client = clientForSession();
    if (client)
        client->stepInto(0);
}

void DapEditorIntegration::stepOut()
{
    DapClient* client = clientForSession();
    if (client)
        client->stepOut(0);
}

void DapEditorIntegration::pause()
{
    DapClient* client = clientForSession();
    if (client)
        client->pause(0);
}

void DapEditorIntegration::terminate()
{
    DapClient* client = clientForSession();
    if (client)
        client->terminate();
}

void DapEditorIntegration::onStopped(const StoppedEvent& /*ev*/)
{
    emit debuggingStateChanged(true);
    // The editor's current-line highlight is driven by the stack-trace arrival
    // (routed via MainWindow from stackTraceReady).
}

void DapEditorIntegration::onTerminated()
{
    if (m_editor)
        m_editor->clearCurrentExecutionLine();
    m_dapActive = false;
    emit debuggingStateChanged(false);
}

void DapEditorIntegration::onThreadEvent(const ThreadEvent& ev)
{
    Q_UNUSED(ev);
    // Thread started/exited; optionally refresh the call stack view.
}

} // namespace dap
