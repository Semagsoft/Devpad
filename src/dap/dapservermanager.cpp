#include "dapservermanager.h"

#include "logger.h"
#include "settingsmanager.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace dap
{

DapServerManager::DapServerManager(QObject* parent) : QObject(parent)
{
}

DapServerManager::~DapServerManager()
{
    stopAllSessions();
}

DapClient* DapServerManager::clientForSession(const QString& sessionKey) const
{
    return m_clients.value(sessionKey);
}

DapClient* DapServerManager::currentClient() const
{
    if (m_clients.isEmpty())
        return nullptr;
    return m_clients.begin().value();
}

void DapServerManager::startSession(const QString& sessionKey, const QString& language,
                                    const QString& command, const QStringList& args)
{
    if (m_clients.contains(sessionKey))
    {
        // Session already exists; bring it to the fore if running.
        return;
    }

    QString resolvedCommand = command;
    QStringList resolvedArgs = args;
    if (resolvedCommand.isEmpty())
    {
        if (!language.isEmpty())
        {
            if (m_adapterCommands.contains(language))
                resolvedCommand = m_adapterCommands.value(language);
            else
                resolvedCommand = defaultAdapterCommands().value(language);
        }
        if (resolvedCommand.isEmpty())
        {
            emit clientError(sessionKey, tr("No debug adapter configured for language %1").arg(language));
            return;
        }
        if (resolvedArgs.isEmpty() && !language.isEmpty())
        {
            if (m_adapterArgs.contains(language))
                resolvedArgs = m_adapterArgs.value(language);
            else
                resolvedArgs = defaultAdapterArgs(language);
        }
    }

    auto* client = new DapClient(this);
    m_clients.insert(sessionKey, client);
    if (!language.isEmpty())
        m_sessionLanguage.insert(sessionKey, language);

    // Forward client signals, tagging them with the session key.
    connect(client, &DapClient::serverStarted, this,
            [this, sessionKey]() { emit clientStarted(sessionKey); });
    connect(client, &DapClient::serverStopped, this,
            [this, sessionKey]()
            {
                m_clients.remove(sessionKey);
                m_sessionLanguage.remove(sessionKey);
                emit clientStopped(sessionKey);
            });
    connect(client, &DapClient::serverError, this,
            [this, sessionKey](const QString& error) { emit clientError(sessionKey, error); });
    connect(client, &DapClient::initializeDone, this,
            [this, sessionKey](const Capabilities& caps) { emit initializeDone(sessionKey, caps); });
    connect(client, &DapClient::launchDone, this, [this, sessionKey]() { emit launchDone(sessionKey); });
    connect(client, &DapClient::attachDone, this, [this, sessionKey]() { emit attachDone(sessionKey); });
    connect(client, &DapClient::breakpointsSet, this,
            [this, sessionKey](const QString& path, const QList<Breakpoint>& bps)
            {
                emit breakpointsSet(sessionKey, path, bps);
            });
    connect(client, &DapClient::exceptionBreakpointsSet, this,
            [this, sessionKey]() { emit exceptionBreakpointsSet(sessionKey); });
    connect(client, &DapClient::stopped, this,
            [this, sessionKey](const StoppedEvent& ev) { emit stopped(sessionKey, ev); });
    connect(client, &DapClient::terminated, this,
            [this, sessionKey](const TerminatedEvent& ev) { emit terminated(sessionKey, ev); });
    connect(client, &DapClient::threadEvent, this,
            [this, sessionKey](const ThreadEvent& ev) { emit threadEvent(sessionKey, ev); });
    connect(client, &DapClient::breakpointEvent, this,
            [this, sessionKey](const BreakpointEvent& ev) { emit breakpointEvent(sessionKey, ev); });
    connect(client, &DapClient::output, this,
            [this, sessionKey](const OutputEvent& ev) { emit output(sessionKey, ev); });
    connect(client, &DapClient::capabilitiesEvent, this,
            [this, sessionKey](const QJsonObject& caps) { emit capabilitiesEvent(sessionKey, caps); });
    connect(client, &DapClient::threadsReady, this,
            [this, sessionKey](const QList<Thread>& threads) { emit threadsReady(sessionKey, threads); });
    connect(client, &DapClient::stackTraceReady, this,
            [this, sessionKey](const QList<StackFrame>& frames, int total)
            { emit stackTraceReady(sessionKey, frames, total); });
    connect(client, &DapClient::scopesReady, this,
            [this, sessionKey](const QList<Scope>& scopes) { emit scopesReady(sessionKey, scopes); });
    connect(client, &DapClient::variablesReady, this,
            [this, sessionKey](const QList<Variable>& variables) { emit variablesReady(sessionKey, variables); });
    connect(client, &DapClient::continueDone, this,
            [this, sessionKey](int threadId, bool allThreads)
            { emit continueDone(sessionKey, threadId, allThreads); });

    client->startServer(resolvedCommand, resolvedArgs);
}

void DapServerManager::stopSession(const QString& sessionKey)
{
    if (auto* client = m_clients.take(sessionKey))
    {
        m_sessionLanguage.remove(sessionKey);
        client->stopServer();
        client->deleteLater();
        emit clientStopped(sessionKey);
    }
}

void DapServerManager::stopAllSessions()
{
    auto keys = m_clients.keys();
    for (const QString& key : keys)
        stopSession(key);
}

bool DapServerManager::isDebugging(const QString& sessionKey) const
{
    if (auto* client = m_clients.value(sessionKey))
        return client->isRunning();
    return false;
}

void DapServerManager::registerAdapter(const QString& language, const QString& command,
                                       const QStringList& args)
{
    if (language.isEmpty() || command.isEmpty())
        return;
    m_adapterCommands.insert(language, command);
    m_adapterArgs.insert(language, args);
}

void DapServerManager::unregisterAdapter(const QString& language)
{
    if (language.isEmpty())
        return;
    m_adapterCommands.remove(language);
    m_adapterArgs.remove(language);
}

QHash<QString, QString> DapServerManager::defaultAdapterCommands()
{
    return {
        {"cpp", "lldb-vscode"},
        {"c", "lldb-vscode"},
        {"rust", "lldb-vscode"},
        {"python", "debugpy"},
        {"javascript", "vscode-chrome-debug-core"},
        {"typescript", "vscode-chrome-debug-core"},
        {"go", "dlv"},
    };
}

QStringList DapServerManager::defaultAdapterArgs(const QString& language)
{
    if (language == "python")
        return {};
    if (language == "go")
        return {"dap", "--listen=127.0.0.1:0", "--log-dest=3"};
    return {};
}

} // namespace dap
