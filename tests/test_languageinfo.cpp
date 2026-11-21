#include "languageinfo.h"
#include "theme.h"

#include <Qsci/qsciapis.h>
#include <Qsci/qscilexer.h>
#include <Qsci/qscilexercpp.h>
#include <Qsci/qscilexerjavascript.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

static QString lexerClassName(QsciLexer* lexer)
{
    return QString::fromLatin1(lexer->metaObject()->className());
}

class LanguageInfoTest : public ::testing::Test
{
};

TEST_F(LanguageInfoTest, LanguageTableIsNotEmpty)
{
    const auto& table = languageTable();
    EXPECT_GT(table.size(), 0);
}

TEST_F(LanguageInfoTest, EachLanguageHasName)
{
    for (const auto& lang : languageTable())
    {
        EXPECT_FALSE(lang.name.isEmpty());
    }
}

TEST_F(LanguageInfoTest, EachLanguageHasLexerClassName)
{
    for (const auto& lang : languageTable())
    {
        EXPECT_FALSE(lang.lexerClassName.isEmpty());
    }
}

TEST_F(LanguageInfoTest, EachLanguageHasFactory)
{
    for (const auto& lang : languageTable())
    {
        EXPECT_TRUE(static_cast<bool>(lang.factory));
    }
}

TEST_F(LanguageInfoTest, EachLanguageHasKeywords)
{
    for (const auto& lang : languageTable())
    {
        EXPECT_TRUE(static_cast<bool>(lang.keywords));
    }
}

TEST_F(LanguageInfoTest, EachLanguageHasThemeApplicator)
{
    for (const auto& lang : languageTable())
    {
        EXPECT_TRUE(static_cast<bool>(lang.themeApplicator));
    }
}

TEST_F(LanguageInfoTest, FactoryCreatesLexer)
{
    for (const auto& lang : languageTable())
    {
        QsciLexer* lexer = lang.factory(nullptr);
        EXPECT_NE(lexer, nullptr) << "factory returned null for " << lang.name.toStdString();
        delete lexer;
    }
}

TEST_F(LanguageInfoTest, FactoryCreatesCorrectLexerType)
{
    for (const auto& lang : languageTable())
    {
        QsciLexer* lexer = lang.factory(nullptr);
        ASSERT_NE(lexer, nullptr);
        QString actual = lexerClassName(lexer);
        EXPECT_EQ(actual, lang.lexerClassName) << lang.name.toStdString() << " expected " << lang.lexerClassName.toStdString() << " but got "
                                               << actual.toStdString();
        delete lexer;
    }
}

TEST_F(LanguageInfoTest, FindLanguageReturnsNullForUnknown)
{
    EXPECT_EQ(findLanguage("nonexistent_language_xyz"), nullptr);
}

TEST_F(LanguageInfoTest, FindLanguageReturnsCorrectEntry)
{
    const auto* lang = findLanguage("javascript");
    ASSERT_NE(lang, nullptr);
    EXPECT_EQ(lang->name, "javascript");
    EXPECT_EQ(lang->lexerClassName, "QsciLexerJavaScript");

    const auto* jsonLang = findLanguage("json");
    ASSERT_NE(jsonLang, nullptr);
    EXPECT_EQ(jsonLang->name, "json");
    EXPECT_EQ(jsonLang->lexerClassName, "QsciLexerJSON");

    const auto* luaLang = findLanguage("lua");
    ASSERT_NE(luaLang, nullptr);
    EXPECT_EQ(luaLang->name, "lua");
    EXPECT_EQ(luaLang->lexerClassName, "QsciLexerLua");
}

TEST_F(LanguageInfoTest, PluginLanguageRegistrationIncludesCommentSyntax)
{
    const QString id = QStringLiteral("test_plugin_language");
    ASSERT_TRUE(registerPluginLanguage(id, QStringLiteral("QsciLexerCPP"), [](QObject* parent) -> QObject* { return new QsciLexerCPP(parent); }));
    ASSERT_NE(findLanguage(id), nullptr);
    EXPECT_TRUE(registerPluginLanguageKeywords(id, {QStringLiteral("alpha"), QStringLiteral("beta")}));
    EXPECT_EQ(findLanguage(id)->keywords().size(), 2);
    EXPECT_TRUE(registerPluginLanguageCommentSyntax(id, QStringLiteral("//"), QStringLiteral("/*"), QStringLiteral("*/")));

    const CommentSyntax* syntax = commentSyntaxForLanguage(id);
    ASSERT_NE(syntax, nullptr);
    EXPECT_EQ(syntax->lineComment, QStringLiteral("//"));
    EXPECT_EQ(syntax->blockCommentStart, QStringLiteral("/*"));
    EXPECT_EQ(syntax->blockCommentEnd, QStringLiteral("*/"));

    // Duplicate registration is rejected; unload cleanup removes everything.
    EXPECT_FALSE(registerPluginLanguageCommentSyntax(id, QString(), QString(), QString()));
    EXPECT_TRUE(unregisterPluginLanguageCommentSyntax(id));
    syntax = commentSyntaxForLanguage(id);
    ASSERT_NE(syntax, nullptr);
    EXPECT_TRUE(syntax->lineComment.isEmpty());
    EXPECT_TRUE(syntax->blockCommentStart.isEmpty());
    EXPECT_TRUE(syntax->blockCommentEnd.isEmpty());
    EXPECT_TRUE(unregisterPluginLanguageKeywords(id));
    EXPECT_TRUE(unregisterPluginLanguage(id));
    EXPECT_EQ(findLanguage(id), nullptr);
    EXPECT_EQ(commentSyntaxForLanguage(id), nullptr);
}

TEST_F(LanguageInfoTest, ThemeApplicatorCacheContainsExpectedKeys)
{
    const auto& cache = themeApplicatorCache();
    EXPECT_TRUE(cache.contains("QsciLexerCPP"));
    EXPECT_TRUE(cache.contains("QsciLexerPython"));
    EXPECT_TRUE(cache.contains("QsciLexerHTML"));
    EXPECT_TRUE(cache.contains("QsciLexerJavaScript"));
    EXPECT_TRUE(cache.contains("QsciLexerCSharp"));
    EXPECT_TRUE(cache.contains("QsciLexerJava"));
    EXPECT_TRUE(cache.contains("QsciLexerCSS"));
    EXPECT_TRUE(cache.contains("QsciLexerXML"));
    EXPECT_TRUE(cache.contains("QsciLexerSQL"));
    EXPECT_TRUE(cache.contains("QsciLexerBash"));
    EXPECT_TRUE(cache.contains("QsciLexerCMake"));
    EXPECT_TRUE(cache.contains("QsciLexerMarkdown"));
}

TEST_F(LanguageInfoTest, ThemeApplicatorDoesNotCrash)
{
    ThemeColors colors = getThemeColors(ThemeId::Light);
    for (const auto& lang : languageTable())
    {
        QsciLexer* lexer = lang.factory(nullptr);
        ASSERT_NE(lexer, nullptr);
        EXPECT_NO_THROW(lang.themeApplicator(lexer, colors)) << "themeApplicator crashed for " << lang.name.toStdString();
        delete lexer;
    }
}

TEST_F(LanguageInfoTest, JavaScriptApplicatorCoversDarkThemeStyles)
{
    const ThemeColors dark = getThemeColors(ThemeId::Dark);

    for (const QString& name : {QStringLiteral("javascript")})
    {
        const LanguageInfo* lang = findLanguage(name);
        ASSERT_NE(lang, nullptr);
        EXPECT_EQ(lang->lexerClassName, "QsciLexerJavaScript");

        QsciLexer* lexer = lang->factory(nullptr);
        ASSERT_NE(lexer, nullptr);
        lang->themeApplicator(lexer, dark);
        auto* js = static_cast<QsciLexerJavaScript*>(lexer);

        EXPECT_EQ(js->color(QsciLexerJavaScript::Operator), dark.operator_);
        EXPECT_EQ(js->color(QsciLexerJavaScript::Default), dark.operator_);
        EXPECT_EQ(js->color(QsciLexerJavaScript::CommentDoc), dark.comment);
        EXPECT_EQ(js->color(QsciLexerJavaScript::CommentLineDoc), dark.comment);
        EXPECT_EQ(js->color(QsciLexerJavaScript::UnclosedString), dark.string);
        EXPECT_EQ(js->color(QsciLexerJavaScript::Regex), dark.foreground);
        EXPECT_NE(js->color(QsciLexerJavaScript::Regex), QColor(0x3f, 0x7f, 0x3f));

        delete lexer;
    }
}

TEST_F(LanguageInfoTest, AliasesShareSameLexerClassName)
{
    auto getClassName = [](const QString& name) -> QString
    {
        const auto* lang = findLanguage(name);
        return lang ? lang->lexerClassName : QString();
    };

    EXPECT_EQ(getClassName("cpp"), getClassName("c"));
    EXPECT_EQ(getClassName("h"), getClassName("cpp"));
    EXPECT_EQ(getClassName("hpp"), getClassName("cpp"));
    EXPECT_EQ(getClassName("html"), getClassName("htm"));
    EXPECT_EQ(getClassName("markdown"), getClassName("md"));
    // TypeScript aliases (typescript/ts/tsx) are registered by the TypeScript
    // plugin, not by the built-in table.
}

TEST_F(LanguageInfoTest, KeywordsProviderReturnsNonEmpty)
{
    for (const auto& lang : languageTable())
    {
        const auto& kws = lang.keywords();
        EXPECT_FALSE(kws.isEmpty()) << "keywords empty for " << lang.name.toStdString();
    }
}

TEST_F(LanguageInfoTest, APIsConstructionDoesNotCrash)
{
    for (const auto& lang : languageTable())
    {
        QsciLexer* lexer = lang.factory(nullptr);
        ASSERT_NE(lexer, nullptr);
        QsciAPIs* apis = new QsciAPIs(lexer);
        const auto& kws = lang.keywords();
        for (const auto& kw : kws)
        {
            apis->add(kw);
        }
        delete apis;
        delete lexer;
    }
}
