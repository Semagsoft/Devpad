#include "keywords.h"

#include <QSet>
#include <array>

#include <gtest/gtest.h>

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
        EXPECT_TRUE(keywords.contains(kw)) << param.name.toStdString() << " should contain '" << kw.toStdString() << "'";
    }
}

TEST_P(KeywordsParamTest, NoDuplicates)
{
    const auto& param = GetParam();
    QStringList keywords = param.getKeywords();
    QSet<QString> unique(keywords.begin(), keywords.end());
    EXPECT_EQ(keywords.size(), unique.size()) << param.name.toStdString() << " has duplicate keywords";
}

static const std::array kAllLanguages = {
    LanguageKeywords{"C++", cppKeywords, {"int", "class", "constexpr", "nullptr", "auto"}},
    LanguageKeywords{"C#", csharpKeywords, {"class", "async", "await"}},
    LanguageKeywords{"Java", javaKeywords, {"class", "interface", "extends"}},
    LanguageKeywords{"Python", pythonKeywords, {"def", "class", "async", "await"}},
    LanguageKeywords{"JavaScript", javascriptKeywords, {"function", "const", "let", "async"}},
    LanguageKeywords{"TypeScript", typescriptKeywords, {"interface", "type", "any"}},
    LanguageKeywords{"Rust", rustKeywords, {"fn", "let", "match", "impl"}},
    LanguageKeywords{"Go", goKeywords, {"func", "defer", "go", "range"}},
    LanguageKeywords{"HTML", htmlKeywords, {"div", "html", "body"}},
    LanguageKeywords{"CSS", cssKeywords, {"color", "display", "flex"}},
    LanguageKeywords{"XML", xmlKeywords, {"xml", "version"}},
    LanguageKeywords{"SQL", sqlKeywords, {"SELECT", "FROM", "WHERE"}},
    LanguageKeywords{"Bash", bashKeywords, {"if", "then", "fi", "echo"}},
    LanguageKeywords{"CMake", cmakeKeywords, {"add_executable", "target_link_libraries", "cmake_minimum_required", "CMAKE_CURRENT_SOURCE_DIR"}},
    LanguageKeywords{"Markdown", markdownKeywords, {}},
};

INSTANTIATE_TEST_SUITE_P(AllLanguages, KeywordsParamTest, ::testing::ValuesIn(kAllLanguages),
                         [](const ::testing::TestParamInfo<LanguageKeywords>& info)
                         {
                             QString s = info.param.name;
                             for (auto& c : s)
                             {
                                 if (!c.isLetterOrNumber() && c != '_')
                                     c = QLatin1Char('_');
                             }
                             return s.toStdString();
                         });
