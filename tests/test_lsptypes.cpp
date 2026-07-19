#include "lsp/lsptypes.h"

#include <gtest/gtest.h>

using namespace lsp;

TEST(LspTypesTest, UriFromPathConvertsLocalPath)
{
    QString result = uriFromPath("/home/user/file.cpp");
    EXPECT_TRUE(result.startsWith("file://"));
    EXPECT_TRUE(result.contains("/home/user/file.cpp"));
}

TEST(LspTypesTest, UriFromPathSkipsExistingFileScheme)
{
    QString result = uriFromPath("file:///home/user/file.cpp");
    EXPECT_EQ(result, "file:///home/user/file.cpp");
}

TEST(LspTypesTest, PathFromUriConvertsToLocalPath)
{
    QString result = pathFromUri("file:///home/user/file.cpp");
    EXPECT_EQ(result, "/home/user/file.cpp");
}

TEST(LspTypesTest, PathFromUriHandlesEmpty)
{
    QString result = pathFromUri(QString());
    EXPECT_TRUE(result.isEmpty());
}

TEST(LspTypesTest, PathFromUriHandlesNonFileScheme)
{
    QString result = pathFromUri("https://example.com/file");
    EXPECT_TRUE(result.isEmpty());
}

TEST(LspTypesTest, ServerCapabilitiesFromJsonEmpty)
{
    ServerCapabilities caps = ServerCapabilities::fromJson(QJsonObject());
    EXPECT_FALSE(caps.completionProvider);
    EXPECT_FALSE(caps.definitionProvider);
    EXPECT_FALSE(caps.referencesProvider);
    EXPECT_FALSE(caps.hoverProvider);
    EXPECT_FALSE(caps.documentSymbolProvider);
    EXPECT_FALSE(caps.signatureHelpProvider);
    EXPECT_FALSE(caps.formattingProvider);
    EXPECT_FALSE(caps.codeActionProvider);
    EXPECT_FALSE(caps.renameProvider);
    EXPECT_FALSE(caps.documentHighlightProvider);
    EXPECT_FALSE(caps.diagnosticProvider);
}

TEST(LspTypesTest, ServerCapabilitiesFromJsonFull)
{
    QJsonObject caps;
    QJsonObject td;
    td["completion"] = QJsonObject();
    td["definition"] = QJsonObject();
    td["references"] = QJsonObject();
    td["hover"] = QJsonObject();
    td["documentSymbol"] = QJsonObject();
    td["signatureHelp"] = QJsonObject();
    td["formatting"] = QJsonObject();
    td["codeAction"] = QJsonObject();
    td["rename"] = QJsonObject();
    td["documentHighlight"] = QJsonObject();
    caps["textDocument"] = td;
    caps["diagnostic"] = QJsonObject();
    caps["experimental"] = QJsonObject();

    ServerCapabilities result = ServerCapabilities::fromJson(caps);
    EXPECT_TRUE(result.completionProvider);
    EXPECT_TRUE(result.definitionProvider);
    EXPECT_TRUE(result.referencesProvider);
    EXPECT_TRUE(result.hoverProvider);
    EXPECT_TRUE(result.documentSymbolProvider);
    EXPECT_TRUE(result.signatureHelpProvider);
    EXPECT_TRUE(result.formattingProvider);
    EXPECT_TRUE(result.codeActionProvider);
    EXPECT_TRUE(result.renameProvider);
    EXPECT_TRUE(result.documentHighlightProvider);
    EXPECT_TRUE(result.diagnosticProvider);
}

TEST(LspTypesTest, PositionToJsonAndBack)
{
    Position pos{5, 10};
    QJsonObject json = pos.toJson();
    EXPECT_EQ(json["line"].toInt(), 5);
    EXPECT_EQ(json["character"].toInt(), 10);

    Position parsed = Position::fromJson(json);
    EXPECT_EQ(parsed.line, 5);
    EXPECT_EQ(parsed.character, 10);
}

TEST(LspTypesTest, RangeToJsonAndBack)
{
    Range range{{1, 2}, {3, 4}};
    QJsonObject json = range.toJson();
    EXPECT_EQ(json["start"].toObject()["line"].toInt(), 1);
    EXPECT_EQ(json["end"].toObject()["line"].toInt(), 3);

    Range parsed = Range::fromJson(json);
    EXPECT_EQ(parsed.start.line, 1);
    EXPECT_EQ(parsed.end.line, 3);
}

TEST(LspTypesTest, DiagnosticFromJsonError)
{
    QJsonObject obj;
    obj["range"] = QJsonObject{{"start", QJsonObject{{"line", 0}, {"character", 0}}}, {"end", QJsonObject{{"line", 0}, {"character", 5}}}};
    obj["message"] = QStringLiteral("Test error");
    obj["severity"] = 1;
    obj["source"] = QStringLiteral("test");

    Diagnostic d = Diagnostic::fromJson(obj);
    EXPECT_EQ(d.message, "Test error");
    EXPECT_EQ(d.severity, "Error");
    EXPECT_EQ(d.severityLevel, 1);
    EXPECT_EQ(d.source, "test");
}

TEST(LspTypesTest, DiagnosticFromJsonWarning)
{
    QJsonObject obj;
    obj["range"] = QJsonObject{{"start", QJsonObject{{"line", 1}, {"character", 0}}}, {"end", QJsonObject{{"line", 1}, {"character", 10}}}};
    obj["message"] = QStringLiteral("Test warning");
    obj["severity"] = 2;
    obj["source"] = QStringLiteral("test");

    Diagnostic d = Diagnostic::fromJson(obj);
    EXPECT_EQ(d.message, "Test warning");
    EXPECT_EQ(d.severity, "Warning");
    EXPECT_EQ(d.severityLevel, 2);
}
