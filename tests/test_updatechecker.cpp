#include "updatechecker.h"

#include <gtest/gtest.h>

TEST(UpdateChecker, CompareVersionsEqual)
{
    EXPECT_EQ(UpdateChecker::compareVersions("1.0", "1.0"), 0);
    EXPECT_EQ(UpdateChecker::compareVersions("1.01", "1.01"), 0);
    EXPECT_EQ(UpdateChecker::compareVersions("1.0.0", "1.0"), 0);
}

TEST(UpdateChecker, CompareVersionsNewer)
{
    EXPECT_EQ(UpdateChecker::compareVersions("1.01", "1.0"), 1);
    EXPECT_EQ(UpdateChecker::compareVersions("1.2", "1.1"), 1);
    EXPECT_EQ(UpdateChecker::compareVersions("1.10", "1.9"), 1);
    EXPECT_EQ(UpdateChecker::compareVersions("2.0", "1.99"), 1);
    EXPECT_EQ(UpdateChecker::compareVersions("1.0.1", "1.0"), 1);
}

TEST(UpdateChecker, CompareVersionsOlder)
{
    EXPECT_EQ(UpdateChecker::compareVersions("1.0", "1.01"), -1);
    EXPECT_EQ(UpdateChecker::compareVersions("0.9", "1.0"), -1);
    EXPECT_EQ(UpdateChecker::compareVersions("1.9", "1.10"), -1);
}

// Release tags may carry pre-release suffixes ("v1.06-beta", "1.2-rc1").
// Only the leading numeric run of each component participates in the
// comparison; QString::toInt() alone would parse "06-beta" as 0 and
// misclassify the release as current.
TEST(UpdateChecker, CompareVersionsIgnoresNonNumericSuffixes)
{
    EXPECT_EQ(UpdateChecker::compareVersions("1.06-beta", "1.05"), 1);
    EXPECT_EQ(UpdateChecker::compareVersions("1.10-b", "1.9"), 1);
    EXPECT_EQ(UpdateChecker::compareVersions("1.2-rc1", "1.19"), -1);
    EXPECT_EQ(UpdateChecker::compareVersions("2.0-x", "1.99"), 1);
    EXPECT_EQ(UpdateChecker::compareVersions("1.05", "1.05.1-beta"), -1);
    EXPECT_EQ(UpdateChecker::compareVersions("1.05beta", "1.05"), 0);
}

TEST(UpdateChecker, NormalizeVersion)
{
    EXPECT_EQ(UpdateChecker::normalizeVersion("v1.01"), "1.01");
    EXPECT_EQ(UpdateChecker::normalizeVersion("V1.0"), "1.0");
    EXPECT_EQ(UpdateChecker::normalizeVersion("1.01"), "1.01");
    EXPECT_EQ(UpdateChecker::normalizeVersion(" 1.01 "), "1.01");
    EXPECT_EQ(UpdateChecker::normalizeVersion("v"), "v");
}
