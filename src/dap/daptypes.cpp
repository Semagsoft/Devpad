#include "daptypes.h"

namespace dap
{

Variable Variable::fromJson(const QJsonObject& obj)
{
    Variable v;
    v.name = obj.value("name").toString();
    v.value = obj.value("value").toString();
    v.type = obj.value("type").toString();
    v.evaluateName = obj.value("evaluateName").toString();
    v.variablesReference = obj.value("variablesReference").toInt(0);
    return v;
}

Scope Scope::fromJson(const QJsonObject& obj)
{
    Scope s;
    s.name = obj.value("name").toString();
    s.variablesReference = obj.value("variablesReference").toInt(0);
    s.expensive = obj.value("expensive").toBool(false);
    return s;
}

StackFrame StackFrame::fromJson(const QJsonObject& obj)
{
    StackFrame f;
    f.id = obj.value("id").toInt(0);
    f.name = obj.value("name").toString();
    f.line = obj.value("line").toInt(0);
    f.column = obj.value("column").toInt(0);
    QJsonObject source = obj.value("source").toObject();
    f.sourcePath = source.value("path").toString();
    f.sourceName = source.value("name").toString();
    return f;
}

Thread Thread::fromJson(const QJsonObject& obj)
{
    Thread t;
    t.id = obj.value("id").toInt(0);
    t.name = obj.value("name").toString();
    return t;
}

Source Source::fromJson(const QJsonObject& obj)
{
    Source s;
    s.name = obj.value("name").toString();
    s.path = obj.value("path").toString();
    return s;
}

Breakpoint Breakpoint::fromJson(const QJsonObject& obj)
{
    Breakpoint b;
    b.verified = obj.value("verified").toBool(false);
    b.line = obj.value("line").toInt(0);
    b.column = obj.value("column").toInt(0);
    b.id = obj.value("id").toInt(0);
    b.message = obj.value("message").toString();
    QJsonObject source = obj.value("source").toObject();
    b.sourcePath = source.value("path").toString();
    return b;
}

Capabilities Capabilities::fromJson(const QJsonObject& caps)
{
    Capabilities c;
    c.supportsConfigurationDoneRequest = caps.value("supportsConfigurationDoneRequest").toBool(false);
    c.supportsSetVariable = caps.value("supportsSetVariable").toBool(false);
    c.supportsConditionalBreakpoints = caps.value("supportsConditionalBreakpoints").toBool(false);
    c.supportsFunctionBreakpoints = caps.value("supportsFunctionBreakpoints").toBool(false);
    c.supportsExceptionBreakpoints = caps.value("supportsExceptionBreakpoints").toBool(false);
    c.supportsStepBack = caps.value("supportsStepBack").toBool(false);
    c.supportsStepInTargetsRequest = caps.value("supportsStepInTargetsRequest").toBool(false);
    c.supportsCompletionsRequest = caps.value("supportsCompletionsRequest").toBool(false);
    c.supportsModulesRequest = caps.value("supportsModulesRequest").toBool(false);
    c.supportsRestartRequest = caps.value("supportsRestartRequest").toBool(false);
    c.supportsBreakpointLocationsRequest = caps.value("supportsBreakpointLocationsRequest").toBool(false);
    c.supportsLoadedSourcesRequest = caps.value("supportsLoadedSourcesRequest").toBool(false);
    c.supportsLogPoints = caps.value("supportsLogPoints").toBool(false);
    c.supportsDelayedStackTraceLoading = caps.value("supportsDelayedStackTraceLoading").toBool(false);
    c.supportsTerminateThreadsRequest = caps.value("supportsTerminateThreadsRequest").toBool(false);
    c.supportsTerminateRequest = caps.value("supportsTerminateRequest").toBool(false);
    return c;
}

StoppedEvent StoppedEvent::fromJson(const QJsonObject& obj)
{
    StoppedEvent e;
    e.reason = obj.value("reason").toString();
    e.allThreadsStopped = obj.value("allThreadsStopped").toBool(false);
    e.threadId = obj.value("threadId").toInt(0);
    e.preserveFocusHint = obj.value("preserveFocusHint").toBool(false);
    e.text = obj.value("text").toString();
    return e;
}

TerminatedEvent TerminatedEvent::fromJson(const QJsonObject& obj)
{
    TerminatedEvent e;
    e.exitCode = obj.value("exitCode").toInt(0);
    return e;
}

ThreadEvent ThreadEvent::fromJson(const QJsonObject& obj)
{
    ThreadEvent e;
    e.reason = obj.value("reason").toString();
    e.threadId = obj.value("threadId").toInt(0);
    return e;
}

BreakpointEvent BreakpointEvent::fromJson(const QJsonObject& obj)
{
    BreakpointEvent e;
    e.reason = obj.value("reason").toString();
    e.breakpoint = Breakpoint::fromJson(obj.value("breakpoint").toObject());
    return e;
}

OutputEvent OutputEvent::fromJson(const QJsonObject& obj)
{
    OutputEvent e;
    e.source = obj.value("source").toInt(0);
    e.category = obj.value("category").toString();
    e.output = obj.value("output").toString();
    e.threadId = obj.value("threadId").toString();
    e.handlesGrouped = obj.value("handlesGrouped").toBool(false);
    return e;
}

} // namespace dap
