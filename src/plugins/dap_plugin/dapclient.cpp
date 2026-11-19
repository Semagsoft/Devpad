#include "dapclient.h"

#include "../../logger.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QProcess>
#include <QTimer>

namespace dap
{

DapClient::DapClient(QObject* parent) : QObject(parent)
{
    m_jsonRpc = new DapJsonRpc(this);

    connect(m_jsonRpc, &DapJsonRpc::eventReceived, this, &DapClient::handleEvent);
    connect(m_jsonRpc, &DapJsonRpc::responseReceived, this, &DapClient::handleResponse);
    connect(m_jsonRpc, &DapJsonRpc::reverseRequestReceived, this, &DapClient::handleReverseRequest);
}

DapClient::~DapClient()
{
    // Best-effort graceful shutdown, guaranteed without ever blocking the
    // GUI thread. stopServer() is a no-op when a disconnect was already sent
    // (stopSession()); otherwise this sends it here.
    stopServer();

    QProcess* p = m_process;
    m_process = nullptr;
    if (p)
    {
        p->disconnect();
        // The process is a child of DapClient. Detach it before ~QObject() runs,
        // otherwise it is deleted immediately and the kill-escalation timer
        // below would dereference freed memory two seconds later.
        p->setParent(nullptr);
        QPointer<QProcess> guard(p);
        if (p->state() != QProcess::NotRunning)
        {
            p->terminate();
            // Escalate to kill on the event loop, then release the process —
            // never waitForFinished() on the GUI thread. The QPointer keeps the
            // callback safe even if the process disappears for another reason.
            QTimer::singleShot(KILL_TIMEOUT_MS, QCoreApplication::instance(),
                               [guard]()
                               {
                                   if (!guard)
                                       return;
                                   if (guard->state() != QProcess::NotRunning)
                                       guard->kill();
                                   guard->deleteLater();
                               });
        }
        else
        {
            p->deleteLater();
        }
    }
}

void DapClient::startServer(const QString& command, const QStringList& args)
{
    if (m_process && m_process->state() != QProcess::NotRunning)
        stopServer();

    m_command = command;
    m_args = args;
    m_initialized = false;
    m_configurationReady = false;
    m_starting = true;
    m_stopping = false;
    m_disconnectSent = false;
    m_stoppedThreadId = 0;

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
    if (!m_process || m_disconnectSent)
        return;

    m_starting = false;
    m_stopping = true;
    m_disconnectSent = true;

    if (m_process->state() == QProcess::NotRunning)
    {
        m_stopping = false;
        QProcess* dead = m_process;
        m_process = nullptr;
        dead->disconnect(this);
        dead->deleteLater();
        emit serverStopped();
        return;
    }

    // DAP lifecycle: send "disconnect" (with terminateDebuggee) to shut down
    // the debug session cleanly. If the adapter does not respond, escalate to
    // terminate/terminate/kill.
    int id = m_jsonRpc->nextRequestSeq();
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
    m_process->write(m_jsonRpc->createRequest(m_jsonRpc->nextRequestSeq(), "terminate", QJsonObject{{"terminateDebuggee", true}}));
}

bool DapClient::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

void DapClient::checkConfigurationReady()
{
    if (m_configurationReady)
        return;
    m_configurationReady = true;
    emit configurationReady();
}

int DapClient::resolveThreadId(int threadId) const
{
    return threadId > 0 ? threadId : m_stoppedThreadId;
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
    params["locale"] = QStringLiteral("en-us");

    int id = m_jsonRpc->nextRequestSeq();
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
            // DAP: the initialize response body IS the capabilities object.
            // Tolerate adapters that nest it one level deep under "capabilities".
            QJsonObject caps = result.toObject();
            if (caps.contains(QStringLiteral("capabilities")) && caps[QStringLiteral("capabilities")].isObject())
                caps = caps[QStringLiteral("capabilities")].toObject();
            m_capabilities = Capabilities::fromJson(caps);
            m_retryCount = 0; // a successful session re-arms crash auto-restart
            m_initialized = true;
            emit initializeDone(m_capabilities);
        });
    m_process->write(m_jsonRpc->createRequest(id, "initialize", params));
}

void DapClient::sendInitializeHandshake()
{
    // adapterID is optional in the DAP spec, but debugpy/pydevd (and other
    // two-stage adapters that forward initialize to their server) reject an
    // initialize request without one.
    sendInitialize(QStringLiteral("Devpad"), QStringLiteral("devpad"));
}

// ── Session control ─────────────────────────────────────────────

void DapClient::launch(const QJsonObject& arguments)
{
    if (!isRunning() || !m_initialized)
        return;
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id,
                                      [this](const QJsonValue& /*result*/, const QJsonObject& error)
                                      {
                                          if (!error.isEmpty())
                                          {
                                              emit serverError(QStringLiteral("[launch] %1").arg(error["message"].toString()));
                                              return;
                                          }
                                          emit launchDone();
                                          checkConfigurationReady();
                                      });
    m_process->write(m_jsonRpc->createRequest(id, "launch", arguments));
}

void DapClient::attach(const QJsonObject& arguments)
{
    if (!isRunning() || !m_initialized)
        return;
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id,
                                      [this](const QJsonValue& /*result*/, const QJsonObject& error)
                                      {
                                          if (!error.isEmpty())
                                          {
                                              emit serverError(QStringLiteral("[attach] %1").arg(error["message"].toString()));
                                              return;
                                          }
                                          emit attachDone();
                                          checkConfigurationReady();
                                      });
    m_process->write(m_jsonRpc->createRequest(id, "attach", arguments));
}

void DapClient::disconnectDebug(bool terminateDebuggee)
{
    if (!isRunning())
        return;
    QJsonObject params{{"terminateDebuggee", terminateDebuggee}};
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id, [](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 2000);
    m_process->write(m_jsonRpc->createRequest(id, "disconnect", params));
}

void DapClient::terminate()
{
    if (!isRunning() || !m_initialized)
        return;
    QJsonObject params{{"terminateDebuggee", true}};
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id, [](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 2000);
    m_process->write(m_jsonRpc->createRequest(id, "terminate", params));
}

// ── Breakpoints ─────────────────────────────────────────────────

void DapClient::setSourceBreakpoints(const QString& sourcePath, const QList<int>& lines, const QJsonObject& sourceParams)
{
    if (!isRunning() || !m_configurationReady)
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

    // linesStartAt1 was negotiated in initialize: send 1-based lines.
    QJsonArray breakpointsArr;
    for (int line : lines)
    {
        QJsonObject bp;
        bp["line"] = line + 1;
        breakpointsArr.append(bp);
    }

    QJsonObject params{{"source", source}, {"breakpoints", breakpointsArr}, {"sourceModified", false}};
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id,
                                      [this, sourcePath](const QJsonValue& result, const QJsonObject& error)
                                      {
                                          if (!error.isEmpty())
                                          {
                                              emit serverError(QStringLiteral("[setBreakpoints] %1").arg(error["message"].toString()));
                                              return;
                                          }
                                          QList<Breakpoint> bps;
                                          if (result.isObject())
                                              for (const auto& v : result.toObject()["breakpoints"].toArray())
                                              {
                                                  Breakpoint b = Breakpoint::fromJson(v.toObject());
                                                  b.line = qMax(0, b.line - 1); // back to 0-based
                                                  b.sourcePath = sourcePath;
                                                  bps.append(b);
                                              }
                                          emit breakpointsSet(sourcePath, bps);
                                      });
    m_process->write(m_jsonRpc->createRequest(id, "setBreakpoints", params));
}

void DapClient::setExceptionBreakpoints(const QStringList& filters, const QJsonObject& filterOptions)
{
    if (!isRunning() || !m_configurationReady)
        return;
    QJsonArray filtersArr;
    for (const QString& f : filters)
        filtersArr.append(f);
    QJsonObject params{{"filters", filtersArr}, {"filterOptions", filterOptions}};
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id, [this](const QJsonValue& /*result*/, const QJsonObject& /*error*/) { emit exceptionBreakpointsSet(); });
    m_process->write(m_jsonRpc->createRequest(id, "setExceptionBreakpoints", params));
}

// ── Execution control ───────────────────────────────────────────

void DapClient::configurationDone()
{
    if (!isRunning() || !m_configurationReady)
        return;
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id, [](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "configurationDone", QJsonObject()));
}

void DapClient::threads()
{
    if (!isRunning() || !m_initialized)
        return;
    int id = m_jsonRpc->nextRequestSeq();
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
    int id = m_jsonRpc->nextRequestSeq();
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
    int id = m_jsonRpc->nextRequestSeq();
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
    int id = m_jsonRpc->nextRequestSeq();
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
    threadId = resolveThreadId(threadId);
    QJsonObject params{{"threadId", threadId}, {"singleThread", false}};
    int id = m_jsonRpc->nextRequestSeq();
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
    threadId = resolveThreadId(threadId);
    QJsonObject params{{"threadId", threadId}};
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id, [](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "pause", params));
}

void DapClient::stepOver(int threadId)
{
    if (!isRunning() || !m_initialized)
        return;
    threadId = resolveThreadId(threadId);
    QJsonObject params{{"threadId", threadId}};
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id, [](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "next", params));
}

void DapClient::stepInto(int threadId, int targetId)
{
    if (!isRunning() || !m_initialized)
        return;
    threadId = resolveThreadId(threadId);
    QJsonObject params{{"threadId", threadId}};
    if (targetId > 0)
        params["targetId"] = targetId;
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id, [](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "stepIn", params));
}

void DapClient::stepOut(int threadId, int targetId)
{
    if (!isRunning() || !m_initialized)
        return;
    threadId = resolveThreadId(threadId);
    QJsonObject params{{"threadId", threadId}};
    if (targetId > 0)
        params["targetId"] = targetId;
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id, [](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "stepOut", params));
}

void DapClient::stepBack(int threadId)
{
    if (!isRunning() || !m_initialized || !m_capabilities.supportsStepBack)
        return;
    threadId = resolveThreadId(threadId);
    QJsonObject params{{"threadId", threadId}};
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id, [](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "stepBack", params));
}

void DapClient::reverseContinue(int threadId)
{
    if (!isRunning() || !m_initialized || !m_capabilities.supportsStepBack)
        return;
    threadId = resolveThreadId(threadId);
    QJsonObject params{{"threadId", threadId}};
    int id = m_jsonRpc->nextRequestSeq();
    m_jsonRpc->registerPendingRequest(id, [](const QJsonValue& /*result*/, const QJsonObject& /*error*/) {}, 5000);
    m_process->write(m_jsonRpc->createRequest(id, "reverseContinue", params));
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
        {
#ifdef QT_DEBUG
            qDebug().noquote() << QString("[DAP %1] %2").arg(m_command, msg);
#endif
        }
    }
}

void DapClient::onProcessFinished(int /*exitCode*/, QProcess::ExitStatus status)
{
    m_initialized = false;
    m_configurationReady = false;
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
        emit serverStopped();
        return;
    }

    if (!m_process)
        return;

    // Clean up the dead process so it does not linger parented to this
    // object (the auto-restart timer creates a fresh one).
    QProcess* dead = m_process;
    m_process = nullptr;
    dead->disconnect(this);
    dead->deleteLater();
    m_stoppedThreadId = 0;
    m_configurationReady = false;

    const bool crashed = (status == QProcess::CrashExit && m_retryCount < 3);
    if (crashed)
    {
        // Keep the client alive in the manager so the restart lands on the
        // same session key; surface the crash as an error, not a session end.
        m_retryCount++;
#ifdef QT_DEBUG
        qDebug().noquote() << QString("Debug adapter crashed, restarting (attempt %1/3)...").arg(m_retryCount);
#endif
        emit serverError(tr("Debug adapter crashed; restarting (attempt %1/3)...").arg(m_retryCount));
        QTimer::singleShot(m_retryCount * 1000, this,
                           [this]()
                           {
                               if (!m_command.isEmpty())
                                   startServer(m_command, m_args);
                           });
        return;
    }

    emit serverStopped();
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

void DapClient::handleEvent(const QString& event, const QJsonObject& body)
{
    if (event == "initialized")
    {
        // Adapter is ready to accept configuration requests.
        checkConfigurationReady();
        return;
    }
    if (event == "stopped")
    {
        const StoppedEvent ev = StoppedEvent::fromJson(body);
        m_stoppedThreadId = ev.threadId; // used to resolve execution-control requests
        emit stopped(ev);
        return;
    }
    if (event == "terminated")
    {
        m_stoppedThreadId = 0;
        emit terminated(TerminatedEvent::fromJson(body));
        return;
    }
    if (event == "thread")
    {
        emit threadEvent(ThreadEvent::fromJson(body));
        return;
    }
    if (event == "breakpoint")
    {
        // Adapter lines are 1-based; hand 0-based lines to the editor.
        BreakpointEvent ev = BreakpointEvent::fromJson(body);
        ev.breakpoint.line = qMax(0, ev.breakpoint.line - 1);
        emit breakpointEvent(ev);
        return;
    }
    if (event == "output")
    {
        emit output(OutputEvent::fromJson(body));
        return;
    }
    if (event == "continued")
    {
        m_stoppedThreadId = 0;
        return;
    }
    if (event == "capabilities")
    {
        emit capabilitiesEvent(body);
        return;
    }
#ifdef QT_DEBUG
    qDebug().noquote() << QString("[DAP] unhandled event: %1").arg(event);
#endif
}

void DapClient::handleResponse(const QString& command, bool /*success*/, const QJsonValue& /*body*/, const QJsonObject& error)
{
    if (!error.isEmpty())
    {
        QString msg = error["message"].toString();
        QString code = QString::number(error["code"].toInt());
        if (command.isEmpty())
            emit serverError(QStringLiteral("[%1] %2").arg(code, msg));
        else
            emit serverError(QStringLiteral("[%1] %2: %3").arg(code, command, msg));
    }
}

void DapClient::handleReverseRequest(int seq, const QString& command, const QJsonObject& /*arguments*/)
{
    // We advertise no reverse-request capabilities (e.g. runInTerminal);
    // answer everything with an error so adapters do not block forever.
#ifdef QT_DEBUG
    qDebug().noquote() << QString("[DAP] reverse request not supported: %1").arg(command);
#endif
    if (m_process && m_process->state() == QProcess::Running)
        m_process->write(m_jsonRpc->createErrorResponse(seq, command, QStringLiteral("Not supported by this client")));
}

} // namespace dap
