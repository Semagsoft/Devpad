#include "lspjsonrpc.h"

#include <QDebug>
#include <QJsonParseError>
#include <QPointer>

namespace lsp
{

namespace
{
// Canonicalize an id for pending-request matching. Per JSON-RPC the id is
// number | string (string ids are rare but legal), so the prefixed forms
// ("n:5" vs "s:5") keep the two spaces from colliding.
QString canonicalId(const QJsonValue& id)
{
    if (id.isString())
        return QStringLiteral("s:") + id.toString();
    if (id.isDouble())
        return QStringLiteral("n:") + QString::number(static_cast<qint64>(id.toDouble()));
    return QString(); // null/undefined ids do not participate in matching
}

QString canonicalId(int id)
{
    return QStringLiteral("n:") + QString::number(static_cast<qint64>(id));
}

QJsonObject errorObject(int code, const QString& message)
{
    QJsonObject err;
    err[QStringLiteral("code")] = code;
    err[QStringLiteral("message")] = message;
    return err;
}
} // namespace

LspJsonRpc::LspJsonRpc(QObject* parent) : QObject(parent)
{
}

int LspJsonRpc::nextRequestId()
{
    return ++m_requestIdCounter;
}

void LspJsonRpc::registerPendingRequest(int id, ResponseCallback callback, int timeoutMs)
{
    m_pendingRequests.insert(canonicalId(id), std::move(callback));
    if (timeoutMs > 0)
    {
        QPointer<LspJsonRpc> guard(this);
        QTimer::singleShot(timeoutMs, this,
                           [guard, id]()
                           {
                               if (guard)
                                   guard->invokeTimeout(id);
                           });
    }
}

void LspJsonRpc::invokeTimeout(int id)
{
    auto it = m_pendingRequests.find(canonicalId(id));
    if (it == m_pendingRequests.end())
        return;

    ResponseCallback cb = it.value();
    m_pendingRequests.erase(it);
    QJsonObject err = errorObject(-32000, QStringLiteral("Request timed out"));
    emit responseReceived(QVariant(id), QJsonValue(), err);
    if (cb)
        cb(QJsonValue(), err);
}

void LspJsonRpc::cancelRequest(int id)
{
    m_pendingRequests.remove(canonicalId(id));
}

void LspJsonRpc::failPendingRequests(int code, const QString& message)
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

void LspJsonRpc::handleData(const QByteArray& data)
{
    m_buffer.append(data);

    while (true)
    {
        if (m_contentLength < 0)
        {
            int headerEnd = m_buffer.indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;

            QByteArray header = m_buffer.left(headerEnd);
            m_buffer.remove(0, headerEnd + 4);

            for (const QByteArray& line : header.split('\n'))
            {
                QByteArray l = line.trimmed();
                if (l.startsWith("Content-Length:"))
                {
                    m_contentLength = l.mid(15).trimmed().toInt();
                }
                else if (l.startsWith("Content-Type:"))
                {
                    // Consume but ignore — we only support UTF-8 JSON-RPC
                }
            }
        }

        if (m_contentLength < 0 || m_buffer.size() < m_contentLength)
            return;

        QByteArray message = m_buffer.left(m_contentLength);
        m_buffer.remove(0, m_contentLength);
        m_contentLength = -1;

        processMessage(message);
    }
}

void LspJsonRpc::processMessage(const QByteArray& message)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(message, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        // A malformed message can never be matched to its request id again;
        // fail outstanding requests immediately instead of stranding them
        // until their timeout fires.
        qWarning() << "LSP: dropping malformed JSON-RPC message:" << parseError.errorString();
        failPendingRequests(-32700, QStringLiteral("Malformed message; in-flight requests failed"));
        return;
    }

    QJsonObject obj = doc.object();

    QJsonValue paramsVal = obj.value("params");
    QJsonValue resultVal = obj.value("result");
    QJsonObject errObj;
    if (obj.contains("error") && obj["error"].isObject())
        errObj = obj["error"].toObject();

    const bool hasMethod = obj.contains("method");
    const bool hasId = obj.contains("id");

    if (hasMethod && hasId)
    {
        // Server -> client request (per JSON-RPC). It is not a response to any
        // pending client request; the client must answer it explicitly.
        emit serverRequestReceived(obj["id"].toVariant(), obj["method"].toString(), paramsVal.toObject());
        return;
    }

    if (hasMethod)
    {
        emit notificationReceived(obj["method"].toString(), paramsVal.toObject());
        return;
    }

    // Server -> client response to one of our requests.
    const QString key = canonicalId(obj["id"]);
    emit responseReceived(obj["id"].toVariant(), resultVal, errObj);
    resolveResponse(key, resultVal, errObj);
}

void LspJsonRpc::resolveResponse(const QString& key, const QJsonValue& result, const QJsonObject& error)
{
    if (key.isEmpty())
        return;

    auto it = m_pendingRequests.find(key);
    if (it == m_pendingRequests.end())
        return;

    ResponseCallback cb = it.value();
    m_pendingRequests.erase(it);
    if (cb)
        cb(result, error);
}

QByteArray LspJsonRpc::frame(const QJsonObject& obj) const
{
    QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray header = QString("Content-Length: %1\r\nContent-Type: application/vscode-jsonrpc; charset=utf-8\r\n\r\n").arg(json.size()).toUtf8();
    return header + json;
}

QByteArray LspJsonRpc::createRequest(int id, const QString& method, const QJsonObject& params)
{
    QJsonObject obj;
    obj["jsonrpc"] = QStringLiteral("2.0");
    obj["id"] = id;
    obj["method"] = method;
    obj["params"] = params;
    return frame(obj);
}

QByteArray LspJsonRpc::createNotification(const QString& method, const QJsonObject& params)
{
    QJsonObject obj;
    obj["jsonrpc"] = QStringLiteral("2.0");
    obj["method"] = method;
    obj["params"] = params;
    return frame(obj);
}

QByteArray LspJsonRpc::createResponse(const QVariant& id, const QJsonObject& result)
{
    QJsonObject obj;
    obj["jsonrpc"] = QStringLiteral("2.0");
    obj["id"] = QJsonValue::fromVariant(id);
    obj["result"] = result;
    return frame(obj);
}

QByteArray LspJsonRpc::createErrorResponse(const QVariant& id, int code, const QString& message)
{
    QJsonObject obj;
    obj["jsonrpc"] = QStringLiteral("2.0");
    obj["id"] = QJsonValue::fromVariant(id);
    obj["error"] = errorObject(code, message);
    return frame(obj);
}

} // namespace lsp
