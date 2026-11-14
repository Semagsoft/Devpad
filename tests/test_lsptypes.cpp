#include "lsp/lsptypes.h"

#include <array>

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

TEST(LspTypesTest, ServerCapabilitiesExplicitFalseIsNotSupported)
{
    QJsonObject caps;
    caps["hoverProvider"] = false;
    caps["completionProvider"] = QJsonObject{{"triggerCharacters", QJsonArray{"."}}};
    caps["definitionProvider"] = QJsonObject{};
    caps["referencesProvider"] = true;

    ServerCapabilities result = ServerCapabilities::fromJson(caps);
    EXPECT_FALSE(result.hoverProvider);
    EXPECT_TRUE(result.completionProvider);
    EXPECT_TRUE(result.definitionProvider);
    EXPECT_TRUE(result.referencesProvider);
    EXPECT_EQ(result.completionTriggerChars.size(), 1);
    EXPECT_EQ(result.completionTriggerChars.first(), ".");
}

TEST(LspTypesTest, ServerCapabilitiesFalseEmbeddedInTextDocument)
{
    QJsonObject td;
    td["completion"] = false;
    td["hover"] = QJsonObject{};
    QJsonObject caps;
    caps["textDocument"] = td;

    ServerCapabilities result = ServerCapabilities::fromJson(caps);
    EXPECT_FALSE(result.completionProvider);
    EXPECT_TRUE(result.hoverProvider);
}

TEST(LspTypesTest, ServerCapabilitiesWorkspaceSymbol)
{
    QJsonObject caps;
    caps["workspaceSymbolProvider"] = true;
    EXPECT_TRUE(ServerCapabilities::fromJson(caps).workspaceSymbolProvider);

    caps = QJsonObject{};
    caps["workspace"] = QJsonObject{{"symbol", QJsonObject{}}};
    EXPECT_TRUE(ServerCapabilities::fromJson(caps).workspaceSymbolProvider);

    caps = QJsonObject{};
    caps["workspace"] = QJsonObject{{"symbol", false}};
    EXPECT_FALSE(ServerCapabilities::fromJson(caps).workspaceSymbolProvider);

    caps = QJsonObject{};
    EXPECT_FALSE(ServerCapabilities::fromJson(caps).workspaceSymbolProvider);
}

TEST(LspTypesTest, DiagnosticCodeStringIsPreserved)
{
    QJsonObject obj;
    obj["range"] = QJsonObject{{"start", QJsonObject{{"line", 0}, {"character", 0}}}, {"end", QJsonObject{{"line", 0}, {"character", 1}}}};
    obj["message"] = QStringLiteral("ts error");
    obj["severity"] = 1;

    obj["code"] = QStringLiteral("TS2304");
    EXPECT_EQ(Diagnostic::fromJson(obj).code, "TS2304");

    obj["code"] = 2304; // numeric codes are stringified
    EXPECT_EQ(Diagnostic::fromJson(obj).code, "2304");
}

TEST(LspTypesTest, SymbolKindName)
{
    EXPECT_EQ(symbolKindName(1), "File");
    EXPECT_EQ(symbolKindName(5), "Class");
    EXPECT_EQ(symbolKindName(6), "Method");
    EXPECT_EQ(symbolKindName(12), "Function");
    EXPECT_EQ(symbolKindName(14), "Constant");
    EXPECT_EQ(symbolKindName(23), "Struct");
    EXPECT_EQ(symbolKindName(26), "TypeParameter");
    EXPECT_EQ(symbolKindName(0), QString());
    EXPECT_EQ(symbolKindName(27), QString());
    EXPECT_EQ(symbolKindName(-1), QString());
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

TEST(LspTypesTest, ByteOffsetToUtf16AsciiIsIdentity)
{
    // Pure ASCII: one byte == one UTF-16 code unit.
    const QString line = QStringLiteral("hello");
    EXPECT_EQ(byteOffsetToUtf16(line, 0), 0);
    EXPECT_EQ(byteOffsetToUtf16(line, 1), 1);
    EXPECT_EQ(byteOffsetToUtf16(line, 5), 5);
}

TEST(LspTypesTest, ByteOffsetToUtf16MultiByte)
{
    // "café": é is U+00E9 = 2 UTF-8 bytes (C3 A9), 1 UTF-16 code unit.
    // Valid byte boundaries: 0, 1, 2, 3, 5.
    const QString line = QStringLiteral("café");
    EXPECT_EQ(byteOffsetToUtf16(line, 0), 0);   // start
    EXPECT_EQ(byteOffsetToUtf16(line, 1), 1);   // after 'c'
    EXPECT_EQ(byteOffsetToUtf16(line, 2), 2);   // after 'ca'
    EXPECT_EQ(byteOffsetToUtf16(line, 3), 3);   // start of é
    EXPECT_EQ(byteOffsetToUtf16(line, 5), 4);   // after é (end of line)
    EXPECT_EQ(byteOffsetToUtf16(line, 100), 4); // beyond end clamps to line length
}

TEST(LspTypesTest, Utf16ToByteOffsetMultiByte)
{
    const QString line = QStringLiteral("café");
    EXPECT_EQ(utf16ToByteOffset(line, 0), 0);   // start
    EXPECT_EQ(utf16ToByteOffset(line, 1), 1);   // after 'c'
    EXPECT_EQ(utf16ToByteOffset(line, 2), 2);   // after 'ca'
    EXPECT_EQ(utf16ToByteOffset(line, 3), 3);   // after 'caf' (start of é)
    EXPECT_EQ(utf16ToByteOffset(line, 4), 5);   // after é (end of line)
    EXPECT_EQ(utf16ToByteOffset(line, 100), 5); // beyond end clamps to byte size
}

TEST(LspTypesTest, Utf16ToByteOffsetAstral)
{
    // "a😀b": 'a' (1 byte), 😀 U+1F600 (4 UTF-8 bytes, 2 UTF-16 units), 'b' (1 byte).
    // Valid UTF-16 boundaries: 0, 1, 3, 4.
    const QString line = QStringLiteral("a😀b");
    EXPECT_EQ(utf16ToByteOffset(line, 0), 0); // start
    EXPECT_EQ(utf16ToByteOffset(line, 1), 1); // after 'a' (start of emoji)
    EXPECT_EQ(utf16ToByteOffset(line, 3), 5); // after emoji (before 'b')
    EXPECT_EQ(utf16ToByteOffset(line, 4), 6); // after 'b' (end of line)
}

TEST(LspTypesTest, ByteOffsetToUtf16RoundTripAtBoundaries)
{
    // "naïve日😀": n(1) a(1) ï(2) v(1) e(1) 日(3) 😀(4) = 13 UTF-8 bytes.
    // Valid byte boundaries: 0, 1, 2, 4, 5, 6, 9, 13.
    const QString line = QStringLiteral("naïve日😀");
    std::array<int, 8> offsets = {0, 1, 2, 4, 5, 6, 9, 13};
    for (int byteOffset : offsets)
    {
        int utf16 = byteOffsetToUtf16(line, byteOffset);
        int back = utf16ToByteOffset(line, utf16);
        EXPECT_EQ(back, byteOffset) << "round-trip failed at byte offset " << byteOffset;
    }
}

TEST(LspTypesTest, CharIndexToUtf16ColAscii)
{
    const QString line = QStringLiteral("hello");
    EXPECT_EQ(charIndexToUtf16Col(line, 0), 0);
    EXPECT_EQ(charIndexToUtf16Col(line, 1), 1);
    EXPECT_EQ(charIndexToUtf16Col(line, 5), 5);
    EXPECT_EQ(utf16ColToCharIndex(line, 0), 0);
    EXPECT_EQ(utf16ColToCharIndex(line, 1), 1);
    EXPECT_EQ(utf16ColToCharIndex(line, 5), 5);
}

TEST(LspTypesTest, CharIndexAndUtf16ColForMultiByte)
{
    // "café日😀": c a f é 日 😀
    // é is U+00E9 (2 UTF-8 bytes, 1 unit), 日 is U+65E5 (3 bytes, 1 unit),
    // 😀 is U+1F600 (4 bytes, 2 units).
    const QString line = QStringLiteral("café日😀");
    // code point indices: c=0 a=1 f=2 é=3 日=4 😀=5; utf16 cols: c a f é(1) 日(1) 😀(2)
    EXPECT_EQ(charIndexToUtf16Col(line, 0), 0);
    EXPECT_EQ(charIndexToUtf16Col(line, 3), 3);   // é
    EXPECT_EQ(charIndexToUtf16Col(line, 4), 4);   // 日
    EXPECT_EQ(charIndexToUtf16Col(line, 5), 5);   // start of 😀
    EXPECT_EQ(charIndexToUtf16Col(line, 6), 7);   // after 😀

    EXPECT_EQ(utf16ColToCharIndex(line, 0), 0);
    EXPECT_EQ(utf16ColToCharIndex(line, 3), 3);   // é
    EXPECT_EQ(utf16ColToCharIndex(line, 4), 4);   // 日
    EXPECT_EQ(utf16ColToCharIndex(line, 5), 5);   // start of 😀
    EXPECT_EQ(utf16ColToCharIndex(line, 7), 6);   // after 😀
    // A UTF-16 column inside the astral character clamps to its start.
    EXPECT_EQ(utf16ColToCharIndex(line, 6), 5);
}

TEST(LspTypesTest, CharIndexToByteOffsetMultiByte)
{
    // "a日😀": a(1,1 unit) 日(3 bytes,1 unit) 😀(4 bytes,2 units)
    const QString line = QStringLiteral("a日😀");
    EXPECT_EQ(charIndexToByteOffset(line, 0), 0);
    EXPECT_EQ(charIndexToByteOffset(line, 1), 1);   // after 'a'
    EXPECT_EQ(charIndexToByteOffset(line, 2), 4);   // after 日
    EXPECT_EQ(charIndexToByteOffset(line, 3), 8);   // after 😀
    EXPECT_EQ(byteOffsetToCharIndex(line, 0), 0);
    EXPECT_EQ(byteOffsetToCharIndex(line, 1), 1);   // after 'a'
    EXPECT_EQ(byteOffsetToCharIndex(line, 2), 1);   // mid-日 clamps to 'a'
    EXPECT_EQ(byteOffsetToCharIndex(line, 4), 2);   // after 日
    EXPECT_EQ(byteOffsetToCharIndex(line, 5), 2);   // mid-😀 clamps
    EXPECT_EQ(byteOffsetToCharIndex(line, 8), 3);   // after 😀
}

TEST(LspTypesTest, AllConversionsAgreeOnRoundTrip)
{
    // "naïve日😀x": n a ï v e 日 😀 x
    // byte boundaries: 0 1 2 4 5 6 9 13 14
    // utf16 columns:   0 1 2 3 4 5 6 8 9   (😀 is two UTF-16 units)
    const QString line = QStringLiteral("naïve日😀x");
    // code points:     0 1 2 3 4 5 6 7 8
    std::array<int, 8> cp = {0, 1, 2, 3, 4, 5, 6, 7};
    for (int c : cp)
    {
        int utf16 = charIndexToUtf16Col(line, c);
        int bytes = charIndexToByteOffset(line, c);
        EXPECT_EQ(utf16ToByteOffset(line, utf16), bytes);
        EXPECT_EQ(utf16ColToCharIndex(line, utf16), c);
        EXPECT_EQ(byteOffsetToCharIndex(line, bytes), c);
        EXPECT_EQ(byteOffsetToUtf16(line, bytes), utf16);
    }
}

TEST(LspTypesTest, ConversionClampsBeyondLineEnd)
{
    const QString line = QStringLiteral("ab");
    EXPECT_EQ(utf16ColToCharIndex(line, 10), 2);
    EXPECT_EQ(charIndexToUtf16Col(line, 10), 2);
    EXPECT_EQ(charIndexToByteOffset(line, 10), 2);
    EXPECT_EQ(byteOffsetToCharIndex(line, 10), 2);
}

TEST(LspTypesTest, ConversionOnEmptyLine)
{
    const QString empty;
    EXPECT_EQ(utf16ColToCharIndex(empty, 0), 0);
    EXPECT_EQ(charIndexToUtf16Col(empty, 0), 0);
    EXPECT_EQ(charIndexToByteOffset(empty, 0), 0);
    EXPECT_EQ(byteOffsetToCharIndex(empty, 0), 0);
    EXPECT_EQ(utf16ColToCharIndex(empty, 5), 0);
}

TEST(LspTypesTest, ByteOffsetToUtf16EdgeCases)
{
    const QString line = QStringLiteral("test");
    EXPECT_EQ(byteOffsetToUtf16(line, -1), 0);
    EXPECT_EQ(byteOffsetToUtf16(line, 0), 0);
    EXPECT_EQ(utf16ToByteOffset(line, -1), 0);
    EXPECT_EQ(utf16ToByteOffset(line, 0), 0);
    const QString empty;
    EXPECT_EQ(byteOffsetToUtf16(empty, 0), 0);
    EXPECT_EQ(utf16ToByteOffset(empty, 0), 0);
}
