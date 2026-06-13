#include <gtest/gtest.h>
#include "appstrings.h"

class AppStringsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(AppStringsTest, AppNameIsDevpad) {
    EXPECT_EQ(Strings::AppName(), QStringLiteral("Devpad"));
}

TEST_F(AppStringsTest, UntitledIsUntitled) {
    EXPECT_EQ(Strings::untitled(), QObject::tr("Untitled"));
}

TEST_F(AppStringsTest, AppNameNotEmpty) {
    EXPECT_FALSE(Strings::AppName().isEmpty());
}

TEST_F(AppStringsTest, UntitledNotEmpty) {
    EXPECT_FALSE(Strings::untitled().isEmpty());
}
