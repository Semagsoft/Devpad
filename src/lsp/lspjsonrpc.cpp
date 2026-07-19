#include "lspjsonrpc.h"

#include <QJsonParseError>

namespace lsp
{

LspJsonRpc::LspJsonRpc(QObject* parent) : QObject(parent)
{
}

int LspJsonRpc::nextRequestId()
{
    return ++m_requestIdCounter;
}

void LspJsonRpc::registerPendingRequest(int id, const ResponseCallback& callback, int timeoutMs)
{
    m_pendingRequests[id] = callback;
    if (timeoutMs > 0)
    {
        QTimer::singleShot(timeoutMs, this,
                           [this, id]()
                           {
                               auto it = m_pendingRequests.find(id);
                               if (it != m_pendingRequests.end())
                               {
                                   m_pendingRequests.erase(it);
                               }
                           });
    }
}

void LspJsonRpc::cancelRequest(int id)
{
    m_pendingRequests.remove(id);
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
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(message, &error);
    if (error.error != QJsonParseError::NoError)
        return;

    QJsonObject obj = doc.object();

    QJsonValue paramsVal = obj.value("params");
    QJsonValue resultVal = obj.value("result");
    QJsonObject errObj = obj.value("error").toObject();

    if (obj.contains("method"))
    {
        QString method = obj["method"].toString();
        QJsonObject params = paramsVal.toObject();

        if (obj.contains("id"))
        {
            int id = obj["id"].toInt();
            emit responseReceived(id, resultVal, errObj);

            auto it = m_pendingRequests.find(id);
            if (it != m_pendingRequests.end())
            {
                if (it.value() && errObj.isEmpty())
                    it.value()(resultVal);
                m_pendingRequests.erase(it);
            }
        }
        else
        {
            emit notificationReceived(method, params);
        }
    }
    else if (obj.contains("id"))
    {
        int id = obj["id"].toInt();
        emit responseReceived(id, resultVal, errObj);

        auto it = m_pendingRequests.find(id);
        if (it != m_pendingRequests.end())
        {
            if (it.value() && errObj.isEmpty())
                it.value()(resultVal);
            m_pendingRequests.erase(it);
        }
    }
}

QByteArray LspJsonRpc::createRequest(int id, const QString& method, const QJsonObject& params)
{
    QJsonObject obj;
    obj["jsonrpc"] = QStringLiteral("2.0");
    obj["id"] = id;
    obj["method"] = method;
    obj["params"] = params;

    QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray header = QString("Content-Length: %1\r\nContent-Type: application/vscode-jsonrpc; charset=utf-8\r\n\r\n").arg(json.size()).toUtf8();
    return header + json;
}

QByteArray LspJsonRpc::createNotification(const QString& method, const QJsonObject& params)
{
    QJsonObject obj;
    obj["jsonrpc"] = QStringLiteral("2.0");
    obj["method"] = method;
    obj["params"] = params;

    QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray header = QString("Content-Length: %1\r\nContent-Type: application/vscode-jsonrpc; charset=utf-8\r\n\r\n").arg(json.size()).toUtf8();
    return header + json;
}

QByteArray LspJsonRpc::createResponse(int id, const QJsonObject& result)
{
    QJsonObject obj;
    obj["jsonrpc"] = QStringLiteral("2.0");
    obj["id"] = id;
    obj["result"] = result;

    QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    QByteArray header = QString("Content-Length: %1\r\nContent-Type: application/vscode-jsonrpc; charset=utf-8\r\n\r\n").arg(json.size()).toUtf8();
    return header + json;
}

} // namespace lsp
