#include "encodingutils.h"

#include <gtest/gtest.h>

class EncodingUtilsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
    }
};

TEST_F(EncodingUtilsTest, SupportedEncodingsNotEmpty)
{
    auto encodings = supportedEncodings();
    EXPECT_FALSE(encodings.isEmpty());
    EXPECT_EQ(encodings.size(), 9);
}

TEST_F(EncodingUtilsTest, SupportedEncodingsContainsUtf8)
{
    auto encodings = supportedEncodings();
    bool found = false;
    for (const auto& info : encodings)
    {
        if (info.displayName == "UTF-8" && info.encoding == QStringConverter::Utf8)
        {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(EncodingUtilsTest, SupportedEncodingsContainsAll)
{
    auto encodings = supportedEncodings();
    QStringList names;
    for (const auto& info : encodings)
    {
        names.append(info.displayName);
    }
    EXPECT_TRUE(names.contains("UTF-8"));
    EXPECT_TRUE(names.contains("UTF-16"));
    EXPECT_TRUE(names.contains("UTF-16LE"));
    EXPECT_TRUE(names.contains("UTF-16BE"));
    EXPECT_TRUE(names.contains("UTF-32"));
    EXPECT_TRUE(names.contains("UTF-32LE"));
    EXPECT_TRUE(names.contains("UTF-32BE"));
    EXPECT_TRUE(names.contains("ISO-8859-1"));
    EXPECT_TRUE(names.contains("System"));
}

TEST_F(EncodingUtilsTest, EncodingFromNameUtf8)
{
    EXPECT_EQ(encodingFromName("UTF-8"), QStringConverter::Utf8);
}

TEST_F(EncodingUtilsTest, EncodingFromNameUtf16)
{
    EXPECT_EQ(encodingFromName("UTF-16"), QStringConverter::Utf16);
}

TEST_F(EncodingUtilsTest, EncodingFromNameLatin1)
{
    EXPECT_EQ(encodingFromName("ISO-8859-1"), QStringConverter::Latin1);
}

TEST_F(EncodingUtilsTest, EncodingFromNameSystem)
{
    EXPECT_EQ(encodingFromName("System"), QStringConverter::System);
}

TEST_F(EncodingUtilsTest, EncodingFromNameUnknownReturnsUtf8)
{
    EXPECT_EQ(encodingFromName("Unknown-Encoding"), QStringConverter::Utf8);
}

TEST_F(EncodingUtilsTest, EncodingToDisplayNameUtf8)
{
    EXPECT_EQ(encodingToDisplayName(QStringConverter::Utf8), "UTF-8");
}

TEST_F(EncodingUtilsTest, EncodingToDisplayNameUtf16)
{
    EXPECT_EQ(encodingToDisplayName(QStringConverter::Utf16), "UTF-16");
}

TEST_F(EncodingUtilsTest, EncodingToDisplayNameLatin1)
{
    EXPECT_EQ(encodingToDisplayName(QStringConverter::Latin1), "ISO-8859-1");
}

TEST_F(EncodingUtilsTest, EncodingToDisplayNameSystem)
{
    EXPECT_EQ(encodingToDisplayName(QStringConverter::System), "System");
}

TEST_F(EncodingUtilsTest, EncodingToDisplayNameUnknownReturnsUtf8)
{
    EXPECT_EQ(encodingToDisplayName(static_cast<QStringConverter::Encoding>(99)), "UTF-8");
}

TEST_F(EncodingUtilsTest, RoundTrip)
{
    auto encodings = supportedEncodings();
    for (const auto& info : encodings)
    {
        EXPECT_EQ(encodingFromName(info.displayName), info.encoding);
        EXPECT_EQ(encodingToDisplayName(info.encoding), info.displayName);
    }
}
