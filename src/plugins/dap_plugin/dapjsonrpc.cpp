#include "dapjsonrpc.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QPointer>

namespace dap
{

namespace
{
QJsonObject errorObject(int code, const QString& message)
{
    QJsonObject err;
    err[QStringLiteral("code")] = code;
    err[QStringLiteral("message")] = message;
    return err;
}

constexpr int kCommandFailed = 3000; // DAP-reserved range for command failures
} // namespace

DapJsonRpc::DapJsonRpc(QObject* parent) : QObject(parent)
{
}

int DapJsonRpc::nextRequestSeq()
{
    return ++m_requestSeqCounter;
}

void DapJsonRpc::registerPendingRequest(int seq, ResponseCallback callback, int timeoutMs)
{
    m_pendingRequests.insert(seq, callback);
    if (timeoutMs > 0)
    {
        QPointer<DapJsonRpc> guard(this);
        QTimer::singleShot(timeoutMs, this,
                           [guard, seq]()
                           {
                               if (guard)
                                   guard->invokeTimeout(seq);
                           });
    }
}

void DapJsonRpc::invokeTimeout(int seq)
{
    auto it = m_pendingRequests.find(seq);
    if (it == m_pendingRequests.end())
        return;

    ResponseCallback cb = it.value();
    m_pendingRequests.erase(it);
    QJsonObject err = errorObject(kCommandFailed, QStringLiteral("Request timed out"));
    emit responseReceived(QString(), false, QJsonValue(), err);
    if (cb)
        cb(QJsonValue(), err);
}

void DapJsonRpc::failPendingRequests(int code, const QString& message)
{
    if (m_pendingRequests.isEmpty())
        return;

    QJsonObject err = errorObject(code, message);
    auto pending = m_pendingRequests;
    m_pendingRequests.clear();
    for (auto it = pending.begin(); it != pending.end(); ++it)
    {
        if (it.value())
            it.value()(QJsonValue(), err);
    }
}

void DapJsonRpc::handleData(const QByteArray& data)
{
    m_framer.handleData(data, [this](const QByteArray& message) { processMessage(message); });
}

void DapJsonRpc::processMessage(const QByteArray& message)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "DAP: dropping malformed message:" << parseError.errorString();
        return;
    }

    const QJsonObject obj = doc.object();
    const QString type = obj.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("event"))
    {
        emit eventReceived(obj.value(QStringLiteral("event")).toString(), obj.value(QStringLiteral("body")).toObject());
        return;
    }

    if (type == QStringLiteral("request"))
    {
        // Adapter -> client request. Must be answered by the client.
        emit reverseRequestReceived(obj.value(QStringLiteral("seq")).toInt(), obj.value(QStringLiteral("command")).toString(),
                                    obj.value(QStringLiteral("arguments")).toObject());
        return;
    }

    if (type == QStringLiteral("response"))
    {
        const int requestSeq = obj.value(QStringLiteral("request_seq")).toInt(-1);
        const bool success = obj.value(QStringLiteral("success")).toBool(false);
        const QString command = obj.value(QStringLiteral("command")).toString();
        const QJsonValue body = obj.value(QStringLiteral("body"));

        QJsonObject err;
        if (!success)
            err = errorObject(kCommandFailed, obj.value(QStringLiteral("message")).toString(QStringLiteral("Debug adapter request failed")));

        emit responseReceived(command, success, body, err);
        resolveResponse(requestSeq, body, err);
        return;
    }

    qWarning() << "DAP: dropping message with unknown type:" << type;
}

void DapJsonRpc::resolveResponse(int seq, const QJsonValue& body, const QJsonObject& error)
{
    if (seq < 0)
        return;

    auto it = m_pendingRequests.find(seq);
    if (it == m_pendingRequests.end())
        return;

    ResponseCallback cb = it.value();
    m_pendingRequests.erase(it);
    if (cb)
        cb(body, error);
}

QByteArray DapJsonRpc::createRequest(int seq, const QString& command, const QJsonObject& arguments)
{
    QJsonObject obj;
    obj[QStringLiteral("seq")] = seq;
    obj[QStringLiteral("type")] = QStringLiteral("request");
    obj[QStringLiteral("command")] = command;
    if (!arguments.isEmpty())
        obj[QStringLiteral("arguments")] = arguments;
    return m_framer.frame(obj);
}

QByteArray DapJsonRpc::createResponse(int requestSeq, const QString& command, bool success, const QString& message, const QJsonValue& body)
{
    QJsonObject obj;
    obj[QStringLiteral("seq")] = nextRequestSeq();
    obj[QStringLiteral("type")] = QStringLiteral("response");
    obj[QStringLiteral("request_seq")] = requestSeq;
    obj[QStringLiteral("success")] = success;
    obj[QStringLiteral("command")] = command;
    if (!message.isEmpty())
        obj[QStringLiteral("message")] = message;
    if (!body.isUndefined())
        obj[QStringLiteral("body")] = body;
    return m_framer.frame(obj);
}

QByteArray DapJsonRpc::createErrorResponse(int requestSeq, const QString& command, const QString& message)
{
    return createResponse(requestSeq, command, false, message);
}

} // namespace dap
