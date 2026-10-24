#ifndef LSPJSONRPC_H
#define LSPJSONRPC_H

#include <QByteArray>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <QVariant>
#include <functional>

namespace lsp
{

class LspJsonRpc : public QObject
{
    Q_OBJECT

public:
    explicit LspJsonRpc(QObject* parent = nullptr);

    void handleData(const QByteArray& data);
    QByteArray createRequest(int id, const QString& method, const QJsonObject& params);
    QByteArray createNotification(const QString& method, const QJsonObject& params);
    // Responses echo the server's original id, which per JSON-RPC may be a
    // number or a string — hence the QVariant.
    QByteArray createResponse(const QVariant& id, const QJsonObject& result);
    QByteArray createErrorResponse(const QVariant& id, int code, const QString& message);

    using ResponseCallback = std::function<void(const QJsonValue& result, const QJsonObject& error)>;
    void registerPendingRequest(int id, ResponseCallback callback, int timeoutMs = 30000);
    int nextRequestId();
    void cancelRequest(int id);
    // Invoke every outstanding callback with an error response (used when the
    // process dies or the protocol breaks and requests can no longer be
    // answered by the server).
    void failPendingRequests(int code, const QString& message);

signals:
    void notificationReceived(const QString& method, const QJsonObject& params);
    void responseReceived(const QVariant& id, const QJsonValue& result, const QJsonObject& error);
    // Server -> client request (method + id): must be answered by the client,
    // never treated as a response to one of our requests.
    void serverRequestReceived(const QVariant& id, const QString& method, const QJsonObject& params);

private:
    void processMessage(const QByteArray& message);
    void resolveResponse(const QString& key, const QJsonValue& result, const QJsonObject& error);
    void invokeTimeout(int id);

    QByteArray frame(const QJsonObject& obj) const;

    QByteArray m_buffer;
    int m_contentLength = -1;
    bool m_discardingOversized = false;
    int m_requestIdCounter = 0;
    // Keyed by a canonical id string ("n:<num>" / "s:<str>") so numeric and
    // string ids cannot collide during pending-request matching.
    QHash<QString, ResponseCallback> m_pendingRequests;
};

} // namespace lsp

#endif // LSPJSONRPC_H
