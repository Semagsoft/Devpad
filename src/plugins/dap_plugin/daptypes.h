#ifndef DAPTYPES_H
#define DAPTYPES_H

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

namespace dap
{

// ────────────────────────────────────────────────────────────────
// Core value types
// ────────────────────────────────────────────────────────────────

struct Variable
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString value MEMBER value)
    Q_PROPERTY(QString type MEMBER type)
    Q_PROPERTY(QString evaluateName MEMBER evaluateName)
    Q_PROPERTY(int variablesReference MEMBER variablesReference)

public:
    QString name;
    QString value;
    QString type;
    QString evaluateName;
    int variablesReference = 0;

    static Variable fromJson(const QJsonObject& obj);
};

struct Scope
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(int variablesReference MEMBER variablesReference)
    Q_PROPERTY(bool expensive MEMBER expensive)

public:
    QString name;
    int variablesReference = 0;
    bool expensive = false;

    static Scope fromJson(const QJsonObject& obj);
};

struct StackFrame
{
    Q_GADGET
    Q_PROPERTY(int id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(int line MEMBER line)
    Q_PROPERTY(int column MEMBER column)
    Q_PROPERTY(QString sourcePath MEMBER sourcePath)
    Q_PROPERTY(QString sourceName MEMBER sourceName)

public:
    int id = 0;
    QString name;
    int line = 0;
    int column = 0;
    QString sourcePath; // file path on disk
    QString sourceName; // display name

    static StackFrame fromJson(const QJsonObject& obj);
};

struct Thread
{
    Q_GADGET
    Q_PROPERTY(int id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)

public:
    int id = 0;
    QString name;

    static Thread fromJson(const QJsonObject& obj);
};

struct Source
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString path MEMBER path)

public:
    QString name;
    QString path;

    static Source fromJson(const QJsonObject& obj);
};

struct Breakpoint
{
    Q_GADGET
    Q_PROPERTY(bool verified MEMBER verified)
    Q_PROPERTY(int line MEMBER line)
    Q_PROPERTY(int column MEMBER column)
    Q_PROPERTY(int id MEMBER id)
    Q_PROPERTY(QString message MEMBER message)
    Q_PROPERTY(QString sourcePath MEMBER sourcePath)

public:
    bool verified = false;
    int line = 0;
    int column = 0;
    int id = 0;
    QString message;
    QString sourcePath;

    static Breakpoint fromJson(const QJsonObject& obj);
};

// ────────────────────────────────────────────────────────────────
// Capabilities advertised by the debug adapter
// ────────────────────────────────────────────────────────────────

struct Capabilities
{
    bool supportsConfigurationDoneRequest = false;
    bool supportsSetVariable = false;
    bool supportsConditionalBreakpoints = false;
    bool supportsFunctionBreakpoints = false;
    bool supportsExceptionBreakpoints = false;
    bool supportsStepBack = false;
    bool supportsStepInTargetsRequest = false;
    bool supportsCompletionsRequest = false;
    bool supportsModulesRequest = false;
    bool supportsRestartRequest = false;
    bool supportsBreakpointLocationsRequest = false;
    bool supportsLoadedSourcesRequest = false;
    bool supportsLogPoints = false;
    bool supportsDelayedStackTraceLoading = false;
    bool supportsTerminateThreadsRequest = false;
    bool supportsTerminateRequest = false;

    static Capabilities fromJson(const QJsonObject& caps);
};

// ────────────────────────────────────────────────────────────────
// DAP events (server -> client, unsolicited notifications)
// ────────────────────────────────────────────────────────────────

struct StoppedEvent
{
    QString reason; // "step", "breakpoint", "exception", "pause", "entry"
    bool allThreadsStopped = false;
    int threadId = 0;
    bool preserveFocusHint = false;
    QString text;

    static StoppedEvent fromJson(const QJsonObject& obj);
};

struct TerminatedEvent
{
    int exitCode = 0;

    static TerminatedEvent fromJson(const QJsonObject& obj);
};

struct ThreadEvent
{
    QString reason; // "started" or "exited"
    int threadId = 0;

    static ThreadEvent fromJson(const QJsonObject& obj);
};

struct BreakpointEvent
{
    QString reason; // "new", "changed", "removed"
    Breakpoint breakpoint;

    static BreakpointEvent fromJson(const QJsonObject& obj);
};

struct OutputEvent
{
    int source = 0;   // 0=console, 1=stdout, 2=stderr, 3=telemetry
    QString category; // "console", "stdout", "stderr", "telemetry"
    QString output;
    QString threadId;
    bool handlesGrouped = false;

    static OutputEvent fromJson(const QJsonObject& obj);
};

} // namespace dap

#endif // DAPTYPES_H
