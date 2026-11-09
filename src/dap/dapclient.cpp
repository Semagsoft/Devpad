#include "dapclient.h"

#include "logger.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QProcess>

namespace dap
{

DapClient::DapClient(QObject* parent) : QObject(parent)
{
    m_jsonRpc = new lsp::LspJsonRpc(this);

    connect(m_jsonRpc, &lsp::LspJsonRpc::notificationReceived, this, &DapClient::handleNotification);
    connect(m_jsonRpc, &lsp::LspJsonRpc::responseReceived, this, &DapClient::handleResponse);
    connect(m_jsonRpc, &lsp::LspJsonRpc::serverRequestReceived, this, &DapClient::handleServerRequest);
}

DapClient::~DapClient()
{
    // Best-effort graceful shutdown, then guarantee no orphaned adapter
    // process outlives this object.
    stopServer();

    if (m_process)
    {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning)
        {
            m_process->terminate();
            if (!m_process->waitForFinished(KILL_TIMEOUT_MS))
                m_process->kill();
            m_process->waitForFinished(KILL_TIMEOUT_MS);
        }
        delete m_process;
        m_process = nullptr;
    }
}

void DapClient::startServer(const QString& command, const QStringList& args)
{
    if (m_process && m_process->state() != QProcess::NotRunning)
        stopServer();

    m_command = command;
    m_args = args;
    m_initialized = false;
    m_starting = true;

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput, this, &DapClient::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError, this, &DapClient::onReadyReadStderr);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &DapClient::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &DapClient::onProcessError);
    connect(m_process, &QProcess::started, this, &DapClient::onProcessStarted);

    m_process->start(command, args);

    QPointer<QProcess> guard(m_process);
    QTimer::singleShot(START_TIMEOUT_MS, this,
                       [this, guard]()
                       {
                           if (!m_starting || !guard || guard != m_process)
                               return;
                           m_starting = false;
                           emit serverError(tr("Debug adapter timed out starting: %1").arg(m_command));
                           stopServer();
                       });
}

void DapClient::onProcessStarted()
{
    if (!m_process)
        return;
    m_starting = false;
    sendInitializeHandshake();
    emit serverStarted();
}

void DapClient::stopServer()
{
    if (!m_process)
        return;

    m_starting = false;
    m_stopping = true;

    if (m_process->state() == QProcess::NotRunning)
    {
        if (m_process)
        {
            m_process->disconnect();
            m_process->deleteLater();
            m_process = nullptr;
        }
        return;
    }

    // DAP lifecycle: send "disconnect" (with terminateDebuggee) to shut down
    // the debug session cleanly. If the adapter does not respond, escalate to
    // terminate/terminate/kill.
    int id = m_jsonRpc->nextRequestId();
    m_process->write(m_jsonRpc->createRequest(id, "disconnect", QJsonObject{{"terminateDebuggee", true}}));
    m_jsonRpc->registerPendingRequest(
        id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) { sendTerminateIfStopping(); }, SHUTDOWN_TIMEOUT_MS);

    QPointer<QProcess> guard(m_process);
    QTimer::singleShot(SHUTDOWN_TIMEOUT_MS, this,
                       [this, guard]()
                       {
                           if (!guard || guard != m_process || !m_stopping)
                               return;
                           if (m_process->state() != QProcess::NotRunning)
                           {
                               sendTerminateIfStopping();
                               m_process->terminate();
                               QTimer::singleShot(KILL_TIMEOUT_MS, this,
                                                  [this, guard]()
                                                  {
                                                      if (!guard || guard != m_process || !m_stopping)
                                                          return;
                                                      if (m_process->state() != QProcess::NotRunning)
                                                          m_process->kill();
                                                  });
                           }
                       });
}

void DapClient::sendTerminateIfStopping()
{
    if (!m_stopping || !m_process)
        return;
    m_process->write(m_jsonRpc->createRequest(0, "terminate", QJsonObject{{"terminateDebuggee", true}}));
}

bool DapClient::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

void DapClient::sendInitialize(const QString& clientName, const QString& adapterID, const QJsonObject& pathFormat)
{
    if (!isRunning())
        return;

    QJsonObject params;
    params["clientName"] = clientName;
    if (!adapterID.isEmpty())
        params["adapterID"] = adapterID;
    params["pathFormat"] = pathFormat.isEmpty() ? QJsonValue(QStringLiteral("path")) : pathFormat;
    params["linesStartAt1"] = true;
    params["columnsStartAt1"] = true;
    params["supportsVariableType"] = true;
    params["supportsVariablePaging"] = true;
    params["supportsRunInTerminalRequest"] = true;
    params["locale"] = QStringLiteral("en-us");

    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(
        id,
        [this](const QJsonValue& result, const QJsonObject& error)
        {
            if (!error.isEmpty())
            {
                emit serverError(QStringLiteral("[%1] %2").arg(QString::number(error["code"].toInt(0)), error["message"].toString()));
                m_starting = false;
                stopServer();
                return;
            }
            m_capabilities = Capabilities::fromJson(result.toObject()["capabilities"].toObject());
            m_initialized = true;
            emit initializeDone(m_capabilities);
        });
    m_process->write(m_jsonRpc->createRequest(id, "initialize", params));
}

void DapClient::sendInitializeHandshake()
{
    sendInitialize();
}

// ── Session control ─────────────────────────────────────────────

void DapClient::launch(const QJsonObject& arguments)
{
    if (!isRunning() || !m_initialized)
        return;
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(
        id,
        [this](const QJsonValue& /*result*/, const QJsonObject& error)
        {
            if (!error.isEmpty())
            {
                emit serverError(QStringLiteral("[%1] %2").arg(QString::number(error["code"].toInt(0)), error["message"].toString()));
                return;
            }
            emit launchDone();
        });
    m_process->write(m_jsonRpc->createRequest(id, "launch", arguments));
}

void DapClient::attach(const QJsonObject& arguments)
{
    if (!isRunning() || !m_initialized)
        return;
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(
        id,
        [this](const QJsonValue& /*result*/, const QJsonObject& error)
        {
            if (!error.isEmpty())
            {
                emit serverError(QStringLiteral("[%1] %2").arg(QString::number(error["code"].toInt(0)), error["message"].toString()));
                return;
            }
            emit attachDone();
        });
    m_process->write(m_jsonRpc->createRequest(id, "attach", arguments));
}

void DapClient::disconnectDebug(bool terminateDebuggee)
{
    if (!isRunning())
        return;
    QJsonObject params{{"terminateDebuggee", terminateDebuggee}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 2000);
    m_process->write(m_jsonRpc->createRequest(id, "disconnect", params));
}

void DapClient::terminate()
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonObject params{{"terminateDebuggee", true}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 2000);
    m_process->write(m_jsonRpc->createRequest(id, "terminate", params));
}

// ── Breakpoints ─────────────────────────────────────────────────

void DapClient::setSourceBreakpoints(const QString& sourcePath, const QList<int>& lines, const QJsonObject& sourceParams)
{
    if (!isRunning() || !m_initialized)
        return;

    QJsonObject source = sourceParams;
    if (source.isEmpty())
    {
        source = QJsonObject{{"path", sourcePath}};
    }
    else if (!source.contains("path"))
    {
        source["path"] = sourcePath;
    }

    QJsonArray linesArr;
    for (int line : lines)
    {
        QJsonObject bp;
        bp["line"] = line;
        linesArr.append(bp);
    }

    QJsonObject params{{"source", source}, {"lines", linesArr}, {"sourceModified", false}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, sourcePath, lines](const QJsonValue& result, const QJsonObject& /*error*/)
                                      {
                                          QList<Breakpoint> bps;
                                          if (result.isArray())
                                              for (const auto& v : result.toArray())
                                                  bps.append(Breakpoint::fromJson(v.toObject()));
                                          emit breakpointsSet(sourcePath, bps);
                                      });
    m_process->write(m_jsonRpc->createRequest(id, "setBreakpoints", params));
}

void DapClient::setExceptionBreakpoints(const QStringList& filters, const QJsonObject& filterOptions)
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonArray filtersArr;
    for (const QString& f : filters)
        filtersArr.append(f);
    QJsonObject params{{"filters", filtersArr}, {"filterOptions", filterOptions}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) { emit exceptionBreakpointsSet(); });
    m_process->write(m_jsonRpc->createRequest(id, "setExceptionBreakpoints", params));
}

// ── Execution control ───────────────────────────────────────────

void DapClient::configurationDone()
{
    if (!isRunning() || !m_initialized)
        return;
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "configurationDone", QJsonObject()));
}

void DapClient::threads()
{
    if (!isRunning() || !m_initialized)
        return;
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this](const QJsonValue& result, const QJsonObject& /*error*/)
                                      {
                                          QList<Thread> threads;
                                          if (result.isObject())
                                              for (const auto& v : result.toObject()["threads"].toArray())
                                                  threads.append(Thread::fromJson(v.toObject()));
                                          emit threadsReady(threads);
                                      });
    m_process->write(m_jsonRpc->createRequest(id, "threads", QJsonObject()));
}

void DapClient::stackTrace(int threadId, int startFrame, int levels)
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonObject params{{"threadId", threadId}, {"startFrame", startFrame}, {"levels", levels}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this](const QJsonValue& result, const QJsonObject& /*error*/)
                                      {
                                          QList<StackFrame> frames;
                                          int total = 0;
                                          if (result.isObject())
                                          {
                                              QJsonObject obj = result.toObject();
                                              for (const auto& v : obj["stackFrames"].toArray())
                                                  frames.append(StackFrame::fromJson(v.toObject()));
                                              total = obj["totalFrames"].toInt(0);
                                          }
                                          emit stackTraceReady(frames, total);
                                      });
    m_process->write(m_jsonRpc->createRequest(id, "stackTrace", params));
}

void DapClient::scopes(int frameId)
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonObject params{{"frameId", frameId}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this](const QJsonValue& result, const QJsonObject& /*error*/)
                                      {
                                          QList<Scope> scopes;
                                          if (result.isObject())
                                              for (const auto& v : result.toObject()["scopes"].toArray())
                                                  scopes.append(Scope::fromJson(v.toObject()));
                                          emit scopesReady(scopes);
                                      });
    m_process->write(m_jsonRpc->createRequest(id, "scopes", params));
}

void DapClient::variables(int variablesReference, int start, int count)
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonObject params{{"variablesReference", variablesReference}};
    if (start >= 0)
        params["start"] = start;
    if (count > 0)
        params["count"] = count;
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this](const QJsonValue& result, const QJsonObject& /*error*/)
                                      {
                                          QList<Variable> vars;
                                          if (result.isObject())
                                              for (const auto& v : result.toObject()["variables"].toArray())
                                                  vars.append(Variable::fromJson(v.toObject()));
                                          emit variablesReady(vars);
                                      });
    m_process->write(m_jsonRpc->createRequest(id, "variables", params));
}

void DapClient::continueExecution(int threadId)
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonObject params{{"threadId", threadId}, {"singleThread", false}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, threadId](const QJsonValue& result, const QJsonObject& /*error*/)
                                      {
                                          bool allThreads = true;
                                          if (result.isObject())
                                              allThreads = result.toObject()["allThreadsContinued"].toBool(true);
                                          emit continueDone(threadId, allThreads);
                                      });
    m_process->write(m_jsonRpc->createRequest(id, "continue", params));
}

void DapClient::pause(int threadId)
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonObject params{{"threadId", threadId}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "pause", params));
}

void DapClient::stepOver(int threadId)
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonObject params{{"threadId", threadId}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "next", params));
}

void DapClient::stepInto(int threadId, int targetId)
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonObject params{{"threadId", threadId}};
    if (targetId > 0)
        params["targetId"] = targetId;
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "stepIn", params));
}

void DapClient::stepOut(int threadId, int targetId)
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonObject params{{"threadId", threadId}};
    if (targetId > 0)
        params["targetId"] = targetId;
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "stepOut", params));
}

void DapClient::stepBack(int threadId)
{
    if (!isRunning() || !m_initialized || !m_capabilities.supportsStepBack)
        return;
    QJsonObject params{{"threadId", threadId}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "stepBack", params));
}

void DapClient::reverseContinue(int threadId)
{
    if (!isRunning() || !m_initialized || !m_capabilities.supportsStepBack)
        return;
    QJsonObject params{{"threadId", threadId}};
    int id = m_jsonRpc->nextRequestId();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "reverseContinue", params));
}

int DapClient::sendRequest(const QString& /*method*/, const QJsonObject& /*params*/, lsp::LspJsonRpc::ResponseCallback /*callback*/)
{
    // Kept for API symmetry with LspClient; DapClient writes framed messages
    // directly in each request method above.
    return -1;
}

// ── Internal slots ──────────────────────────────────────────────

void DapClient::onReadyReadStdout()
{
    if (!m_process)
        return;
    QByteArray data = m_process->readAllStandardOutput();
    m_jsonRpc->handleData(data);
}

void DapClient::onReadyReadStderr()
{
    if (!m_process)
        return;
    QByteArray data = m_process->readAllStandardError();
    if (!data.isEmpty())
    {
        QString msg = QString::fromUtf8(data).trimmed();
        if (!msg.isEmpty())
#ifdef QT_DEBUG
            qDebug().noquote() << QString("[DAP %1] %2").arg(m_command, msg);
#endif
    }
}

void DapClient::onProcessFinished(int /*exitCode*/, QProcess::ExitStatus status)
{
    m_initialized = false;
    m_jsonRpc->failPendingRequests(-32001, QStringLiteral("Debug adapter stopped"));

    if (m_stopping)
    {
        m_stopping = false;
        if (m_process)
        {
            m_process->disconnect();
            m_process->deleteLater();
            m_process = nullptr;
        }
        return;
    }

    if (!m_process)
        return;

    emit serverStopped();

    if (status == QProcess::CrashExit && m_retryCount < 3)
    {
        m_retryCount++;
#ifdef QT_DEBUG
        qDebug().noquote() << QString("Debug adapter crashed, restarting (attempt %1/3)...").arg(m_retryCount);
#endif
        QTimer::singleShot(m_retryCount * 1000, this,
                           [this]()
                           {
                               if (!m_command.isEmpty())
                                   startServer(m_command, m_args);
                           });
    }
}

void DapClient::onProcessError(QProcess::ProcessError error)
{
    m_starting = false;
    switch (error)
    {
    case QProcess::FailedToStart:
        emit serverError(tr("Debug adapter failed to start: %1").arg(m_command));
        if (m_process && m_process->state() == QProcess::NotRunning)
        {
            m_process->disconnect();
            m_process->deleteLater();
            m_process = nullptr;
        }
        break;
    case QProcess::Timedout:
        emit serverError(tr("Debug adapter timed out: %1").arg(m_command));
        break;
    default:
        emit serverError(tr("Debug adapter process error: %1").arg(m_command));
        break;
    }
}

// ── Notification / response handling ────────────────────────────

void DapClient::handleNotification(const QString& method, const QJsonObject& params)
{
    if (method == "initialized")
    {
        // Handshake complete; adapter is ready for launch/attach.
        return;
    }
    if (method == "stopped")
    {
        emit stopped(StoppedEvent::fromJson(params));
        return;
    }
    if (method == "terminated")
    {
        emit terminated(TerminatedEvent::fromJson(params));
        return;
    }
    if (method == "thread")
    {
        emit threadEvent(ThreadEvent::fromJson(params));
        return;
    }
    if (method == "breakpoint")
    {
        emit breakpointEvent(BreakpointEvent::fromJson(params));
        return;
    }
    if (method == "output")
    {
        emit output(OutputEvent::fromJson(params));
        return;
    }
    if (method == "capabilities")
    {
        emit capabilitiesEvent(params);
        return;
    }
#ifdef QT_DEBUG
    qDebug().noquote() << QString("[DAP] unhandled notification: %1").arg(method);
#endif
}

void DapClient::handleResponse(const QVariant& /*id*/, const QJsonValue& /*result*/, const QJsonObject& error)
{
    if (!error.isEmpty())
    {
        QString msg = error["message"].toString();
        QString code = QString::number(error["code"].toInt());
        emit serverError(QStringLiteral("[%1] %2").arg(code, msg));
    }
}

void DapClient::handleServerRequest(const QVariant& /*id*/, const QString& /*method*/, const QJsonObject& /*params*/)
{
#ifdef QT_DEBUG
    qDebug().noquote() << QString("[DAP] server request not supported: %1").arg(/*method*/ QString());
#endif
    // Answer server->client requests with "method not found" so adapters do
    // not block waiting for a response.
    // Note: DAP rarely sends server->client requests; keep a stub for safety.
}

} // namespace dap
