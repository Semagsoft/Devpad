#include <gtest/gtest.h>
#include <QSet>
#include "keywords.h"

struct LanguageKeywords
{
    QString name;
    const QStringList& (*getKeywords)();
    QStringList sampleKeywords;
};

class KeywordsParamTest : public ::testing::TestWithParam<LanguageKeywords>
{
};

TEST_P(KeywordsParamTest, KeywordsAreNonEmpty)
{
    const auto& param = GetParam();
    QStringList keywords = param.getKeywords();
    EXPECT_FALSE(keywords.isEmpty()) << param.name.toStdString() << " keywords should not be empty";
}

TEST_P(KeywordsParamTest, ContainsSampleKeywords)
{
    const auto& param = GetParam();
    QStringList keywords = param.getKeywords();
    for (const auto& kw : param.sampleKeywords)
    {
        EXPECT_TRUE(keywords.contains(kw))
            << param.name.toStdString() << " should contain '" << kw.toStdString() << "'";
    }
}

TEST_P(KeywordsParamTest, NoDuplicates)
{
    const auto& param = GetParam();
    QStringList keywords = param.getKeywords();
    QSet<QString> unique(keywords.begin(), keywords.end());
    EXPECT_EQ(keywords.size(), unique.size()) << param.name.toStdString() << " has duplicate keywords";
}

static const LanguageKeywords kAllLanguages[] = {
    {"C++",     cppKeywords,         {"int", "class", "constexpr", "nullptr", "auto"}},
    {"C#",      csharpKeywords,      {"class", "async", "await"}},
    {"Java",    javaKeywords,        {"class", "interface", "extends"}},
    {"Python",  pythonKeywords,      {"def", "class", "async", "await"}},
    {"JavaScript", javascriptKeywords, {"function", "const", "let", "async"}},
    {"TypeScript", typescriptKeywords, {"interface", "type", "any"}},
    {"Rust",    rustKeywords,        {"fn", "let", "match", "impl"}},
    {"Go",      goKeywords,          {"func", "defer", "go", "range"}},
    {"HTML",    htmlKeywords,        {"div", "html", "body"}},
    {"CSS",     cssKeywords,         {"color", "display", "flex"}},
    {"XML",     xmlKeywords,         {"xml", "version"}},
    {"SQL",     sqlKeywords,         {"SELECT", "FROM", "WHERE"}},
    {"Bash",    bashKeywords,        {"if", "then", "fi", "echo"}},
    {"CMake",   cmakeKeywords,       {"add_executable", "target_link_libraries", "cmake_minimum_required", "CMAKE_CURRENT_SOURCE_DIR"}},
    {"Markdown", markdownKeywords,   {}},
};

INSTANTIATE_TEST_SUITE_P(
    AllLanguages,
    KeywordsParamTest,
    ::testing::ValuesIn(kAllLanguages),
    [](const ::testing::TestParamInfo<LanguageKeywords>& info) {
        QString s = info.param.name;
        for (auto& c : s) { if (!c.isLetterOrNumber() && c != '_') c = QLatin1Char('_'); }
        return s.toStdString();
    }
);
