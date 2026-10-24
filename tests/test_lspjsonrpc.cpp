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

    QByteArray msg = rpc.createNotification(QStringLiteral("textDocument/publishDiagnostics"),
                                            QJsonObject{{QStringLiteral("uri"), QStringLiteral("file:///test.cpp")}});
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
                               [&callbackCalled](const QJsonValue& result, const QJsonObject& /*error*/)
                               {
                                   callbackCalled = true;
                                   EXPECT_EQ(result.toObject()["value"].toInt(), 42);
                               });

    QByteArray msg = rpc.createRequest(requestId, QStringLiteral("custom/request"), QJsonObject());
    // Replace the request with a response message by stripping the header and rebuilding
    QByteArray body;
    body = R"({"jsonrpc":"2.0","id":)" + QByteArray::number(requestId) + R"(,"result":{"value":42}})";
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
    rpc.registerPendingRequest(id, [&called](const QJsonValue&, const QJsonObject&) { called = true; });
    rpc.cancelRequest(id);

    QByteArray body = R"({"jsonrpc":"2.0","id":)" + QByteArray::number(id) + R"(,"result":{}})";
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

    QByteArray body = R"({"jsonrpc":"2.0","id":9999,"result":{}})";
    QByteArray header = "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    rpc.handleData(header + body);

    ASSERT_EQ(spy.count(), 1);
    // No crash despite unknown id
}

TEST(LspJsonRpcTest, ServerRequestDoesNotResolvePendingRequest)
{
    // A message with both method and id is a server -> client request, not a
    // response to one of our outstanding requests, even when ids collide.
    LspJsonRpc rpc;
    QSignalSpy spy(&rpc, &LspJsonRpc::serverRequestReceived);

    bool callbackCalled = false;
    int id = rpc.nextRequestId(); // counter runs 1, 2, ... so this is id 1
    rpc.registerPendingRequest(id,
                               [&callbackCalled](const QJsonValue&, const QJsonObject&)
                               {
                                   callbackCalled = true;
                               });

    QByteArray body = R"({"jsonrpc":"2.0","id":1,"method":"workspace/applyEdit","params":{}})";
    QByteArray header = "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    rpc.handleData(header + body);

    EXPECT_FALSE(callbackCalled);
    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy[0][1].toString(), QStringLiteral("workspace/applyEdit"));
}

TEST(LspJsonRpcTest, ServerRequestWithStringIdKeepsIdType)
{
    // Ids may be strings per JSON-RPC; they must reach the handler unmodified
    // so a reply can echo them (toInt() would mangle them to 0).
    LspJsonRpc rpc;
    QSignalSpy spy(&rpc, &LspJsonRpc::serverRequestReceived);

    QByteArray body = R"({"jsonrpc":"2.0","id":"req-1","method":"window/showMessageRequest","params":{}})";
    QByteArray header = "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    rpc.handleData(header + body);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy[0][0].toString(), QStringLiteral("req-1"));
}

TEST(LspJsonRpcTest, ErrorResponsePropagatesToCallback)
{
    LspJsonRpc rpc;
    bool called = false;
    QJsonValue receivedResult;
    QJsonObject receivedError;
    int id = rpc.nextRequestId();
    rpc.registerPendingRequest(id,
                               [&called, &receivedResult, &receivedError](const QJsonValue& result, const QJsonObject& error)
                               {
                                   called = true;
                                   receivedResult = result;
                                   receivedError = error;
                               });

    QByteArray body = R"({"jsonrpc":"2.0","id":)" + QByteArray::number(id) + R"(,"error":{"code":-32602,"message":"Invalid params"}})";
    QByteArray header = "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    rpc.handleData(header + body);

    EXPECT_TRUE(called);
    EXPECT_TRUE(receivedResult.isUndefined());
    EXPECT_EQ(receivedError["code"].toInt(), -32602);
    EXPECT_EQ(receivedError["message"].toString(), QStringLiteral("Invalid params"));
}

TEST(LspJsonRpcTest, MalformedMessageFailsPendingRequests)
{
    LspJsonRpc rpc;
    bool called = false;
    QJsonObject receivedError;
    int id = rpc.nextRequestId();
    rpc.registerPendingRequest(id,
                               [&called, &receivedError](const QJsonValue&, const QJsonObject& error)
                               {
                                   called = true;
                                   receivedError = error;
                               });

    rpc.handleData("Content-Length: 10\r\n\r\nnot-a-json");

    EXPECT_TRUE(called);
    EXPECT_EQ(receivedError["code"].toInt(), -32700);
}

TEST(LspJsonRpcTest, FailPendingRequestsInvokesAllCallbacks)
{
    LspJsonRpc rpc;
    int calls = 0;
    QJsonObject receivedError;
    QList<int> ids;
    for (int i = 0; i < 3; ++i)
    {
        int id = rpc.nextRequestId();
        ids.append(id);
        rpc.registerPendingRequest(id,
                                   [&calls, &receivedError](const QJsonValue&, const QJsonObject& error)
                                   {
                                       ++calls;
                                       receivedError = error;
                                   });
    }

    rpc.failPendingRequests(-32001, QStringLiteral("Language server stopped"));

    EXPECT_EQ(calls, 3);
    EXPECT_EQ(receivedError["code"].toInt(), -32001);

    // After failing, late responses for those ids must not invoke callbacks again
    QByteArray body = R"({"jsonrpc":"2.0","id":)" + QByteArray::number(ids.first()) + R"(,"result":{}})";
    QByteArray header = "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n";
    rpc.handleData(header + body);
    EXPECT_EQ(calls, 3);
}
