#include "lsp/lspjsonrpc.h"
#include "lsp/lsptypes.h"

#include <QJsonArray>
#include <QSignalSpy>
#include <gtest/gtest.h>

using namespace lsp;

TEST(LspJsonRpcTest, CreateRequestFormat)
{
    LspJsonRpc rpc;
    QJsonObject params;
    params["query"] = QStringLiteral("test");

    QByteArray result = rpc.createRequest(1, QStringLiteral("textDocument/completion"), params);

    EXPECT_TRUE(result.contains("Content-Length:"));
    EXPECT_TRUE(result.contains("Content-Type: application/vscode-jsonrpc"));
    EXPECT_TRUE(result.contains("\"jsonrpc\":\"2.0\""));
    EXPECT_TRUE(result.contains("\"id\":1"));
    EXPECT_TRUE(result.contains("\"method\":\"textDocument/completion\""));
    EXPECT_TRUE(result.contains("\"params\":"));
    EXPECT_TRUE(result.contains("\"query\":\"test\""));
}

TEST(LspJsonRpcTest, CreateNotificationFormat)
{
    LspJsonRpc rpc;
    QJsonObject params;
    params["uri"] = QStringLiteral("file:///test.cpp");

    QByteArray result = rpc.createNotification(QStringLiteral("textDocument/didOpen"), params);

    EXPECT_TRUE(result.contains("Content-Length:"));
    EXPECT_FALSE(result.contains("\"id\""));
    EXPECT_TRUE(result.contains("\"method\":\"textDocument/didOpen\""));
    EXPECT_TRUE(result.contains("\"params\":"));
    EXPECT_TRUE(result.contains("\"uri\":\"file:///test.cpp\""));
}

TEST(LspJsonRpcTest, CreateResponseFormat)
{
    LspJsonRpc rpc;
    QJsonObject result;
    result["success"] = true;

    QByteArray data = rpc.createResponse(42, result);

    EXPECT_TRUE(data.contains("\"id\":42"));
    EXPECT_TRUE(data.contains("\"result\":"));
    EXPECT_TRUE(data.contains("\"success\":true"));
}

TEST(LspJsonRpcTest, NextRequestIdIncrements)
{
    LspJsonRpc rpc;
    EXPECT_EQ(rpc.nextRequestId(), 1);
    EXPECT_EQ(rpc.nextRequestId(), 2);
    EXPECT_EQ(rpc.nextRequestId(), 3);
}

TEST(LspJsonRpcTest, HandleDataNotification)
{
    LspJsonRpc rpc;
    QSignalSpy spy(&rpc, &LspJsonRpc::notificationReceived);

    QByteArray msg = rpc.createNotification(QStringLiteral("textDocument/publishDiagnostics"), QJsonObject{{QStringLiteral("uri"), QStringLiteral("file:///test.cpp")}});
    rpc.handleData(msg);

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    EXPECT_EQ(args[0].toString(), QStringLiteral("textDocument/publishDiagnostics"));
}

TEST(LspJsonRpcTest, HandleDataResponseWithCallback)
{
    LspJsonRpc rpc;
    QSignalSpy spy(&rpc, &LspJsonRpc::responseReceived);

    bool callbackCalled = false;
    int requestId = rpc.nextRequestId();
    rpc.registerPendingRequest(requestId,
                                [&callbackCalled](const QJsonValue& result)
                                {
                                    callbackCalled = true;
                                    EXPECT_EQ(result.toObject()["value"].toInt(), 42);
                                });

    QByteArray msg = rpc.createRequest(requestId, QStringLiteral("custom/request"), QJsonObject());
    // Replace the request with a response message by stripping the header and rebuilding
    QByteArray body;
    body = "{\"jsonrpc\":\"2.0\",\"id\":" + QByteArray::number(requestId) + ",\"result\":{\"value\":42}}";
    QByteArray header = "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    rpc.handleData(header + body);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(callbackCalled);
}

TEST(LspJsonRpcTest, CancelRequest)
{
    LspJsonRpc rpc;
    int id = rpc.nextRequestId();
    bool called = false;
    rpc.registerPendingRequest(id,
                                [&called](const QJsonValue&)
                                {
                                    called = true;
                                });
    rpc.cancelRequest(id);

    QByteArray body = "{\"jsonrpc\":\"2.0\",\"id\":" + QByteArray::number(id) + ",\"result\":{}}";
    QByteArray header = "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    rpc.handleData(header + body);

    EXPECT_FALSE(called);
}

TEST(LspJsonRpcTest, HandlesMultipleMessagesInOneBuffer)
{
    LspJsonRpc rpc;
    QSignalSpy spy(&rpc, &LspJsonRpc::notificationReceived);

    QByteArray msg1 = rpc.createNotification(QStringLiteral("test/one"), QJsonObject());
    QByteArray msg2 = rpc.createNotification(QStringLiteral("test/two"), QJsonObject());

    rpc.handleData(msg1 + msg2);

    ASSERT_EQ(spy.count(), 2);
    EXPECT_EQ(spy[0][0].toString(), QStringLiteral("test/one"));
    EXPECT_EQ(spy[1][0].toString(), QStringLiteral("test/two"));
}

TEST(LspJsonRpcTest, HandlesPartialData)
{
    LspJsonRpc rpc;
    QSignalSpy spy(&rpc, &LspJsonRpc::notificationReceived);

    QByteArray fullMsg = rpc.createNotification(QStringLiteral("test/partial"), QJsonObject());

    // Send only the first half
    int mid = fullMsg.size() / 2;
    rpc.handleData(fullMsg.left(mid));

    EXPECT_EQ(spy.count(), 0);

    // Send the rest
    rpc.handleData(fullMsg.mid(mid));

    ASSERT_EQ(spy.count(), 1);
}

TEST(LspJsonRpcTest, RequestHasUniqueIds)
{
    LspJsonRpc rpc;
    int id1 = rpc.nextRequestId();
    int id2 = rpc.nextRequestId();
    EXPECT_NE(id1, id2);

    QByteArray req1 = rpc.createRequest(id1, QStringLiteral("test/one"), QJsonObject());
    QByteArray req2 = rpc.createRequest(id2, QStringLiteral("test/two"), QJsonObject());

    EXPECT_TRUE(req1.contains("\"id\":1"));
    EXPECT_TRUE(req2.contains("\"id\":2"));
}

TEST(LspJsonRpcTest, ResponseForUnknownIdIsIgnored)
{
    LspJsonRpc rpc;
    QSignalSpy spy(&rpc, &LspJsonRpc::responseReceived);

    QByteArray body = "{\"jsonrpc\":\"2.0\",\"id\":9999,\"result\":{}}";
    QByteArray header = "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    rpc.handleData(header + body);

    ASSERT_EQ(spy.count(), 1);
    // No crash despite unknown id
}
