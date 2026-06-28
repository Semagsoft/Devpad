#include "languageinfo.h"
#include "theme.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <Qsci/qscilexer.h>
#include <Qsci/qsciapis.h>

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
        EXPECT_EQ(actual, lang.lexerClassName)
            << lang.name.toStdString() << " expected " << lang.lexerClassName.toStdString()
            << " but got " << actual.toStdString();
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
        EXPECT_NO_THROW(lang.themeApplicator(lexer, colors))
            << "themeApplicator crashed for " << lang.name.toStdString();
        delete lexer;
    }
}

TEST_F(LanguageInfoTest, AliasesShareSameLexerClassName)
{
    auto getClassName = [](const QString& name) -> QString {
        const auto* lang = findLanguage(name);
        return lang ? lang->lexerClassName : QString();
    };

    EXPECT_EQ(getClassName("cpp"), getClassName("c"));
    EXPECT_EQ(getClassName("h"), getClassName("cpp"));
    EXPECT_EQ(getClassName("hpp"), getClassName("cpp"));
    EXPECT_EQ(getClassName("html"), getClassName("htm"));
    EXPECT_EQ(getClassName("typescript"), getClassName("javascript"));
    EXPECT_EQ(getClassName("ts"), getClassName("javascript"));
    EXPECT_EQ(getClassName("markdown"), getClassName("md"));
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
        delete lexer;
    }
}
