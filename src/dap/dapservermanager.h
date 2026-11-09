#ifndef DAPSERVERMANAGER_H
#define DAPSERVERMANAGER_H

#include "dapclient.h"
#include "daptypes.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>

namespace dap
{

// Manages active DAP debug sessions. A session is keyed by a QString (the
// file URI being debugged, or a language id for language-specific adapters).
// Multiple concurrent sessions are supported, though the UI typically drives
// one at a time via the debug panel.
class DapServerManager : public QObject
{
    Q_OBJECT

public:
    explicit DapServerManager(QObject* parent = nullptr);
    ~DapServerManager() override;

    DapClient* clientForSession(const QString& sessionKey) const;
    DapClient* currentClient() const;

    // Starts (or reuses) a DAP client for the given session key and language,
    // then performs the initialize handshake.
    void startSession(const QString& sessionKey, const QString& language,
                      const QString& command, const QStringList& args);

    void stopSession(const QString& sessionKey);
    void stopAllSessions();

    bool isDebugging(const QString& sessionKey) const;

    // Registers a default debug adapter command/args for a language.
    void registerAdapter(const QString& language, const QString& command, const QStringList& args);
    void unregisterAdapter(const QString& language);

    QList<QString> sessionKeys() const
    {
        return m_clients.keys();
    }

    static QHash<QString, QString> defaultAdapterCommands();
    static QStringList defaultAdapterArgs(const QString& language);

signals:
    void clientStarted(const QString& sessionKey);
    void clientStopped(const QString& sessionKey);
    void clientError(const QString& sessionKey, const QString& error);

    // Forwarded DAP client signals (sessionKey + payload)
    void initializeDone(const QString& sessionKey, const Capabilities& capabilities);
    void launchDone(const QString& sessionKey);
    void attachDone(const QString& sessionKey);
    void breakpointsSet(const QString& sessionKey, const QString& sourcePath,
                        const QList<Breakpoint>& breakpoints);
    void exceptionBreakpointsSet(const QString& sessionKey);
    void stopped(const QString& sessionKey, const StoppedEvent& ev);
    void terminated(const QString& sessionKey, const TerminatedEvent& ev);
    void threadEvent(const QString& sessionKey, const ThreadEvent& ev);
    void breakpointEvent(const QString& sessionKey, const BreakpointEvent& ev);
    void output(const QString& sessionKey, const OutputEvent& ev);
    void capabilitiesEvent(const QString& sessionKey, const QJsonObject& capabilities);

    void threadsReady(const QString& sessionKey, const QList<Thread>& threads);
    void stackTraceReady(const QString& sessionKey, const QList<StackFrame>& frames, int totalFrames);
    void scopesReady(const QString& sessionKey, const QList<Scope>& scopes);
    void variablesReady(const QString& sessionKey, const QList<Variable>& variables);
    void continueDone(const QString& sessionKey, int threadId, bool allThreadsContinued);

private:
    QHash<QString, DapClient*> m_clients;             // sessionKey -> client
    QHash<QString, QString> m_sessionLanguage;        // sessionKey -> language
    QHash<QString, QString> m_adapterCommands;        // language -> command
    QHash<QString, QStringList> m_adapterArgs;        // language -> args
};

} // namespace dap

#endif // DAPSERVERMANAGER_H
