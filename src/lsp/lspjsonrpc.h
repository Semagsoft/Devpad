#ifndef LSPJSONRPC_H
#define LSPJSONRPC_H

#include <QObject>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <functional>

namespace lsp {

class LspJsonRpc : public QObject
{
    Q_OBJECT

public:
    explicit LspJsonRpc(QObject* parent = nullptr);

    void handleData(const QByteArray& data);
    QByteArray createRequest(int id, const QString& method, const QJsonObject& params);
    QByteArray createNotification(const QString& method, const QJsonObject& params);
    QByteArray createResponse(int id, const QJsonObject& result);

    using ResponseCallback = std::function<void(const QJsonValue&)>;
    void registerPendingRequest(int id, ResponseCallback callback, int timeoutMs = 30000);
    int nextRequestId();
    void cancelRequest(int id);

signals:
    void notificationReceived(const QString& method, const QJsonObject& params);
    void responseReceived(int id, const QJsonValue& result, const QJsonObject& error);

private:
    void processMessage(const QByteArray& message);

    QByteArray m_buffer;
    int m_contentLength = -1;
    int m_requestIdCounter = 0;
    QMap<int, ResponseCallback> m_pendingRequests;
};

} // namespace lsp

#endif // LSPJSONRPC_H
