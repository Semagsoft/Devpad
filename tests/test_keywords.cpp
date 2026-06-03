#include <gtest/gtest.h>
#include <QSet>
#include "keywords.h"

class KeywordsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(KeywordsTest, CppKeywordsNotEmpty) {
    EXPECT_FALSE(cppKeywords().isEmpty());
    EXPECT_TRUE(cppKeywords().contains("int"));
    EXPECT_TRUE(cppKeywords().contains("class"));
    EXPECT_TRUE(cppKeywords().contains("constexpr"));
    EXPECT_TRUE(cppKeywords().contains("nullptr"));
    EXPECT_TRUE(cppKeywords().contains("auto"));
}

TEST_F(KeywordsTest, CsharpKeywordsNotEmpty) {
    EXPECT_FALSE(csharpKeywords().isEmpty());
    EXPECT_TRUE(csharpKeywords().contains("class"));
    EXPECT_TRUE(csharpKeywords().contains("async"));
    EXPECT_TRUE(csharpKeywords().contains("await"));
}

TEST_F(KeywordsTest, JavaKeywordsNotEmpty) {
    EXPECT_FALSE(javaKeywords().isEmpty());
    EXPECT_TRUE(javaKeywords().contains("class"));
    EXPECT_TRUE(javaKeywords().contains("interface"));
    EXPECT_TRUE(javaKeywords().contains("extends"));
}

TEST_F(KeywordsTest, PythonKeywordsNotEmpty) {
    EXPECT_FALSE(pythonKeywords().isEmpty());
    EXPECT_TRUE(pythonKeywords().contains("def"));
    EXPECT_TRUE(pythonKeywords().contains("class"));
    EXPECT_TRUE(pythonKeywords().contains("async"));
    EXPECT_TRUE(pythonKeywords().contains("await"));
}

TEST_F(KeywordsTest, JavascriptKeywordsNotEmpty) {
    EXPECT_FALSE(javascriptKeywords().isEmpty());
    EXPECT_TRUE(javascriptKeywords().contains("function"));
    EXPECT_TRUE(javascriptKeywords().contains("const"));
    EXPECT_TRUE(javascriptKeywords().contains("let"));
    EXPECT_TRUE(javascriptKeywords().contains("async"));
}

TEST_F(KeywordsTest, HtmlKeywordsNotEmpty) {
    EXPECT_FALSE(htmlKeywords().isEmpty());
    EXPECT_TRUE(htmlKeywords().contains("div"));
    EXPECT_TRUE(htmlKeywords().contains("html"));
    EXPECT_TRUE(htmlKeywords().contains("body"));
}

TEST_F(KeywordsTest, CssKeywordsNotEmpty) {
    EXPECT_FALSE(cssKeywords().isEmpty());
    EXPECT_TRUE(cssKeywords().contains("color"));
    EXPECT_TRUE(cssKeywords().contains("display"));
    EXPECT_TRUE(cssKeywords().contains("flex"));
}

TEST_F(KeywordsTest, XmlKeywordsNotEmpty) {
    EXPECT_FALSE(xmlKeywords().isEmpty());
    EXPECT_TRUE(xmlKeywords().contains("xml"));
    EXPECT_TRUE(xmlKeywords().contains("version"));
}

TEST_F(KeywordsTest, SqlKeywordsNotEmpty) {
    EXPECT_FALSE(sqlKeywords().isEmpty());
    EXPECT_TRUE(sqlKeywords().contains("SELECT"));
    EXPECT_TRUE(sqlKeywords().contains("FROM"));
    EXPECT_TRUE(sqlKeywords().contains("WHERE"));
}

TEST_F(KeywordsTest, BashKeywordsNotEmpty) {
    EXPECT_FALSE(bashKeywords().isEmpty());
    EXPECT_TRUE(bashKeywords().contains("if"));
    EXPECT_TRUE(bashKeywords().contains("then"));
    EXPECT_TRUE(bashKeywords().contains("fi"));
    EXPECT_TRUE(bashKeywords().contains("echo"));
}

TEST_F(KeywordsTest, CMakeKeywordsNotEmpty) {
    EXPECT_FALSE(cmakeKeywords().isEmpty());
    EXPECT_TRUE(cmakeKeywords().contains("add_executable"));
    EXPECT_TRUE(cmakeKeywords().contains("target_link_libraries"));
    EXPECT_TRUE(cmakeKeywords().contains("cmake_minimum_required"));
    EXPECT_TRUE(cmakeKeywords().contains("CMAKE_CURRENT_SOURCE_DIR"));
}

TEST_F(KeywordsTest, TypescriptKeywordsNotEmpty) {
    EXPECT_FALSE(typescriptKeywords().isEmpty());
    EXPECT_TRUE(typescriptKeywords().contains("interface"));
    EXPECT_TRUE(typescriptKeywords().contains("type"));
    EXPECT_TRUE(typescriptKeywords().contains("any"));
}

TEST_F(KeywordsTest, RustKeywordsNotEmpty) {
    EXPECT_FALSE(rustKeywords().isEmpty());
    EXPECT_TRUE(rustKeywords().contains("fn"));
    EXPECT_TRUE(rustKeywords().contains("let"));
    EXPECT_TRUE(rustKeywords().contains("match"));
    EXPECT_TRUE(rustKeywords().contains("impl"));
}

TEST_F(KeywordsTest, GoKeywordsNotEmpty) {
    EXPECT_FALSE(goKeywords().isEmpty());
    EXPECT_TRUE(goKeywords().contains("func"));
    EXPECT_TRUE(goKeywords().contains("defer"));
    EXPECT_TRUE(goKeywords().contains("go"));
    EXPECT_TRUE(goKeywords().contains("range"));
}

TEST_F(KeywordsTest, MarkdownKeywordsNotEmpty) {
    EXPECT_FALSE(markdownKeywords().isEmpty());
}

TEST_F(KeywordsTest, NoDuplicatesWithinLanguage) {
    auto check = [](const QStringList &kws) {
        QSet<QString> unique;
        for (const auto &kw : kws) {
            unique.insert(kw);
        }
        EXPECT_EQ(kws.size(), unique.size());
    };
    check(cppKeywords());
    check(csharpKeywords());
    check(javaKeywords());
    check(pythonKeywords());
    check(javascriptKeywords());
    check(typescriptKeywords());
    check(rustKeywords());
    check(goKeywords());
    check(markdownKeywords());
    check(htmlKeywords());
    check(cssKeywords());
    check(xmlKeywords());
    check(sqlKeywords());
    check(bashKeywords());
    check(cmakeKeywords());
}
