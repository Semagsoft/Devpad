/*
 * Devpad - Devpad Code Editor
 * Copyright (C) 2026 Semagsoft
 *
 * DAP (Debug Adapter Protocol) JSON envelope. Speaks DAP messages rather
 * than JSON-RPC: requests carry {seq, type:"request", command, arguments},
 * responses carry {seq, type:"response", request_seq, success, command},
 * events carry {seq, type:"event", event, body}. Framing is the same
 * Content-Length framing as LSP (shared via plugins::ContentLengthFramer).
 */

#ifndef DAPJSONRPC_H
#define DAPJSONRPC_H

#include "../contentlengthframer.h"

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QTimer>
#include <functional>

namespace dap
{

class DapJsonRpc : public QObject
{
    Q_OBJECT

public:
    explicit DapJsonRpc(QObject* parent = nullptr);

    void handleData(const QByteArray& data);

    int nextRequestSeq();
    QByteArray createRequest(int seq, const QString& command, const QJsonObject& arguments);
    QByteArray createResponse(int requestSeq, const QString& command, bool success, const QString& message = QString(),
                              const QJsonValue& body = QJsonValue::Undefined);
    QByteArray createErrorResponse(int requestSeq, const QString& command, const QString& message);

    using ResponseCallback = std::function<void(const QJsonValue& body, const QJsonObject& error)>;
    void registerPendingRequest(int seq, ResponseCallback callback, int timeoutMs = 30000);
    void failPendingRequests(int code, const QString& message);

signals:
    void eventReceived(const QString& event, const QJsonObject& body);
    void responseReceived(const QString& command, bool success, const QJsonValue& body, const QJsonObject& error);
    // Adapter -> client request (e.g. runInTerminal). Must be answered.
    void reverseRequestReceived(int seq, const QString& command, const QJsonObject& arguments);

private:
    void processMessage(const QByteArray& message);
    void resolveResponse(int seq, const QJsonValue& body, const QJsonObject& error);
    void invokeTimeout(int seq);

    plugins::ContentLengthFramer m_framer;
    int m_requestSeqCounter = 0;
    QHash<int, ResponseCallback> m_pendingRequests;
};

} // namespace dap

#endif // DAPJSONRPC_H
