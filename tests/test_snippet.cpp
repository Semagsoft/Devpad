#include "snippet.h"

#include <gtest/gtest.h>

TEST(SnippetTest, ParseBodySimpleTabStop)
{
    QStringList body = {"hello $1 world"};
    Snippet::ExpandedSnippet result = Snippet::parseBody(body);

    EXPECT_EQ(result.text, "hello  world");
    ASSERT_EQ(result.tabStops.size(), 1);
    EXPECT_EQ(result.tabStops[0].number, 1);
    EXPECT_EQ(result.tabStops[0].positions.size(), 1);
    EXPECT_EQ(result.tabStops[0].defaultValue, "");
    EXPECT_EQ(result.tabStops[0].length, 0);
}

TEST(SnippetTest, ParseBodyPlaceholderWithDefault)
{
    QStringList body = {"func(${1:default})"};
    Snippet::ExpandedSnippet result = Snippet::parseBody(body);

    EXPECT_EQ(result.text, "func(default)");
    ASSERT_EQ(result.tabStops.size(), 1);
    EXPECT_EQ(result.tabStops[0].number, 1);
    EXPECT_EQ(result.tabStops[0].defaultValue, "default");
    EXPECT_EQ(result.tabStops[0].length, 7);
}

TEST(SnippetTest, ParseBodyMultipleTabStops)
{
    QStringList body = {"${1:first} and ${2:second}"};
    Snippet::ExpandedSnippet result = Snippet::parseBody(body);

    EXPECT_EQ(result.text, "first and second");
    ASSERT_EQ(result.tabStops.size(), 2);
    EXPECT_EQ(result.tabStops[0].number, 1);
    EXPECT_EQ(result.tabStops[1].number, 2);
}

TEST(SnippetTest, ParseBodySameTabStopMultipleTimes)
{
    QStringList body = {"${1:val} + ${1:val} = ${2:result}"};
    Snippet::ExpandedSnippet result = Snippet::parseBody(body);

    EXPECT_EQ(result.text, "val + val = result");
    ASSERT_EQ(result.tabStops.size(), 2);
    EXPECT_EQ(result.tabStops[0].number, 1);
    EXPECT_EQ(result.tabStops[1].number, 2);
    // Tab stop 1 should have two positions
    EXPECT_EQ(result.tabStops[0].positions.size(), 2);
}

TEST(SnippetTest, ParseBodySimpleDollarStop)
{
    QStringList body = {"for ($1; $2; $3)"};
    Snippet::ExpandedSnippet result = Snippet::parseBody(body);

    EXPECT_EQ(result.text, "for (; ; )");
    ASSERT_EQ(result.tabStops.size(), 3);
    EXPECT_EQ(result.tabStops[0].number, 1);
    EXPECT_EQ(result.tabStops[1].number, 2);
    EXPECT_EQ(result.tabStops[2].number, 3);
}

TEST(SnippetTest, ParseBodyMixedStops)
{
    QStringList body = {"if (${1:condition}) {", "    $2", "}"};
    Snippet::ExpandedSnippet result = Snippet::parseBody(body);

    EXPECT_EQ(result.text, "if (condition) {\n    \n}");
    ASSERT_EQ(result.tabStops.size(), 2);
    EXPECT_EQ(result.tabStops[0].number, 1);
    EXPECT_EQ(result.tabStops[1].number, 2);
    EXPECT_EQ(result.tabStops[0].defaultValue, "condition");
}

TEST(SnippetTest, ParseBodyEscapeSequences)
{
    QStringList body = {R"(\n\t\\\$\})"};
    Snippet::ExpandedSnippet result = Snippet::parseBody(body);

    EXPECT_EQ(result.text, "\n\t\\$}");
}

TEST(SnippetTest, ParseBodyEmptyBody)
{
    QStringList body;
    Snippet::ExpandedSnippet result = Snippet::parseBody(body);

    EXPECT_EQ(result.text, "");
    EXPECT_TRUE(result.tabStops.isEmpty());
}

TEST(SnippetTest, ParseBodyMultipleLines)
{
    QStringList body = {"line1", "line2", "line3"};
    Snippet::ExpandedSnippet result = Snippet::parseBody(body);

    EXPECT_EQ(result.text, "line1\nline2\nline3");
}

TEST(SnippetTest, ExpandReturnsParseBodyResult)
{
    Snippet snip;
    snip.body = {"hello ${1:world}"};

    Snippet::ExpandedSnippet result = snip.expand();

    EXPECT_EQ(result.text, "hello world");
    ASSERT_EQ(result.tabStops.size(), 1);
    EXPECT_EQ(result.tabStops[0].number, 1);
    EXPECT_EQ(result.tabStops[0].defaultValue, "world");
}
