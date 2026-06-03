#include "remotefileservice.h"

#include <QSignalSpy>
#include <QTemporaryDir>

#include <gtest/gtest.h>

class RemoteFileServiceTest : public ::testing::Test
{
protected:
    RemoteFileService m_service;
};

TEST_F(RemoteFileServiceTest, InvalidUrlFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemote("::invalid::url");

    ASSERT_EQ(failSpy.count(), 1);
    EXPECT_EQ(failSpy.at(0).at(0).toString(), "::invalid::url");
}

TEST_F(RemoteFileServiceTest, SshUrlMissingHostFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemote("ssh://user@/path/to/file");

    ASSERT_EQ(failSpy.count(), 1);
}

TEST_F(RemoteFileServiceTest, SshUrlMissingPathFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemote("ssh://user@host");

    ASSERT_EQ(failSpy.count(), 1);
}

TEST_F(RemoteFileServiceTest, SshUrlHostStartsWithDashFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemote("ssh://user@-oProxyCommand=evil/path");

    ASSERT_EQ(failSpy.count(), 1);
}

TEST_F(RemoteFileServiceTest, SshUrlPathStartsWithDashFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemote("ssh://user@host/-oProxyCommand=evil");

    ASSERT_EQ(failSpy.count(), 1);
    EXPECT_TRUE(failSpy.at(0).at(1).toString().contains("starts with '-'"));
}

TEST_F(RemoteFileServiceTest, SshUrlUserStartsWithDashFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemote("ssh://-evil@host/path");

    ASSERT_EQ(failSpy.count(), 1);
    EXPECT_TRUE(failSpy.at(0).at(1).toString().contains("starts with '-'"));
}

TEST_F(RemoteFileServiceTest, SshUrlValidPassesValidation)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    QSignalSpy statusSpy(&m_service, &RemoteFileService::statusMessage);
    ASSERT_TRUE(failSpy.isValid());
    ASSERT_TRUE(statusSpy.isValid());

    m_service.openRemote("ssh://user@example.com/home/user/file.txt");

    EXPECT_EQ(failSpy.count(), 0);
    ASSERT_EQ(statusSpy.count(), 1);
    EXPECT_TRUE(statusSpy.at(0).at(0).toString().contains("SSH"));
}

TEST_F(RemoteFileServiceTest, SshUrlDefaultUserPassesValidation)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    QSignalSpy statusSpy(&m_service, &RemoteFileService::statusMessage);
    ASSERT_TRUE(failSpy.isValid());
    ASSERT_TRUE(statusSpy.isValid());

    m_service.openRemote("ssh://example.com/home/user/file.txt");

    EXPECT_EQ(failSpy.count(), 0);
    ASSERT_EQ(statusSpy.count(), 1);
}

TEST_F(RemoteFileServiceTest, SshUrlWithPortPassesValidation)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    QSignalSpy statusSpy(&m_service, &RemoteFileService::statusMessage);
    ASSERT_TRUE(failSpy.isValid());
    ASSERT_TRUE(statusSpy.isValid());

    m_service.openRemote("ssh://user@host:2222/path/to/file");

    EXPECT_EQ(failSpy.count(), 0);
    ASSERT_EQ(statusSpy.count(), 1);
}

TEST_F(RemoteFileServiceTest, HttpUrlTriggersDownload)
{
    QSignalSpy statusSpy(&m_service, &RemoteFileService::statusMessage);
    ASSERT_TRUE(statusSpy.isValid());

    m_service.openRemote("http://example.com/file.txt");

    ASSERT_EQ(statusSpy.count(), 1);
    EXPECT_TRUE(statusSpy.at(0).at(0).toString().contains("Downloading"));
}
