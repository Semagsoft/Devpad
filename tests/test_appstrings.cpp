#include "appstrings.h"

#include <gtest/gtest.h>

TEST(AppStringsTest, AppNameIsNonEmptyAndCorrect)
{
    EXPECT_EQ(Strings::AppName(), QStringLiteral("Devpad"));
    EXPECT_FALSE(Strings::AppName().isEmpty());
}

TEST(AppStringsTest, UntitledIsNonEmpty)
{
    EXPECT_FALSE(Strings::untitled().isEmpty());
    EXPECT_EQ(Strings::untitled(), QObject::tr("Untitled"));
}
