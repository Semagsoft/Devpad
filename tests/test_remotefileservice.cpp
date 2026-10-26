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

TEST_F(RemoteFileServiceTest, HttpUrlEmitsStatusSyncBeforeNetwork)
{
    // The status message is emitted synchronously before any async network I/O.
    // This test validates URL parsing and the synchronous preamble, not the actual download.
    QSignalSpy statusSpy(&m_service, &RemoteFileService::statusMessage);
    ASSERT_TRUE(statusSpy.isValid());

    m_service.openRemote("http://example.com/file.txt");

    ASSERT_EQ(statusSpy.count(), 1);
    EXPECT_TRUE(statusSpy.at(0).at(0).toString().contains("Downloading"));
    // No assertion on downloadFailed/fileDownloaded — those depend on network availability.
}

TEST_F(RemoteFileServiceTest, HttpUrlInvalidFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemote("");

    ASSERT_EQ(failSpy.count(), 1);
}

TEST_F(RemoteFileServiceTest, HttpsUrlEmitsStatusSyncBeforeNetwork)
{
    QSignalSpy statusSpy(&m_service, &RemoteFileService::statusMessage);
    ASSERT_TRUE(statusSpy.isValid());

    m_service.openRemote("https://example.com/file.txt");

    ASSERT_EQ(statusSpy.count(), 1);
    EXPECT_TRUE(statusSpy.at(0).at(0).toString().contains("Downloading"));
}

TEST_F(RemoteFileServiceTest, RemoteFolderInvalidUrlFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemoteFolder("::invalid::url");

    ASSERT_EQ(failSpy.count(), 1);
    EXPECT_EQ(failSpy.at(0).at(0).toString(), "::invalid::url");
}

TEST_F(RemoteFileServiceTest, RemoteFolderNonSshUrlFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemoteFolder("https://example.com/some/folder");

    ASSERT_EQ(failSpy.count(), 1);
    EXPECT_TRUE(failSpy.at(0).at(1).toString().contains("SSH"));
}

TEST_F(RemoteFileServiceTest, RemoteFolderSshMissingHostFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemoteFolder("ssh://user@/path/to/folder");

    ASSERT_EQ(failSpy.count(), 1);
}

TEST_F(RemoteFileServiceTest, RemoteFolderSshMissingPathFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    ASSERT_TRUE(failSpy.isValid());

    m_service.openRemoteFolder("ssh://user@host");

    ASSERT_EQ(failSpy.count(), 1);
}

TEST_F(RemoteFileServiceTest, RemoteFolderSshValidPassesValidation)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    QSignalSpy statusSpy(&m_service, &RemoteFileService::statusMessage);
    ASSERT_TRUE(failSpy.isValid());
    ASSERT_TRUE(statusSpy.isValid());

    m_service.openRemoteFolder("ssh://user@example.com/home/user/folder");

    EXPECT_EQ(failSpy.count(), 0);
    ASSERT_EQ(statusSpy.count(), 1);
    EXPECT_TRUE(statusSpy.at(0).at(0).toString().contains("SSH"));
}

TEST_F(RemoteFileServiceTest, RemoteFolderSshDefaultUserPassesValidation)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    QSignalSpy statusSpy(&m_service, &RemoteFileService::statusMessage);
    ASSERT_TRUE(failSpy.isValid());
    ASSERT_TRUE(statusSpy.isValid());

    m_service.openRemoteFolder("ssh://example.com/home/user/folder");

    EXPECT_EQ(failSpy.count(), 0);
    ASSERT_EQ(statusSpy.count(), 1);
    EXPECT_TRUE(statusSpy.at(0).at(0).toString().contains("SSH"));
}

TEST_F(RemoteFileServiceTest, RemoteFolderSshWithPortPassesValidation)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    QSignalSpy statusSpy(&m_service, &RemoteFileService::statusMessage);
    ASSERT_TRUE(failSpy.isValid());
    ASSERT_TRUE(statusSpy.isValid());

    m_service.openRemoteFolder("ssh://user@host:2222/path/to/folder");

    EXPECT_EQ(failSpy.count(), 0);
    ASSERT_EQ(statusSpy.count(), 1);
    EXPECT_TRUE(statusSpy.at(0).at(0).toString().contains("SSH"));
}

TEST_F(RemoteFileServiceTest, ListRemoteDirInvalidPathFails)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    QSignalSpy listingFailSpy(&m_service, &RemoteFileService::remoteListingFailed);
    ASSERT_TRUE(failSpy.isValid());
    ASSERT_TRUE(listingFailSpy.isValid());

    m_service.listRemoteDir("ssh://user@host", "");         // empty path
    m_service.listRemoteDir("ssh://user@host", "relative"); // not absolute
    m_service.listRemoteDir("", "/etc");                    // empty url

    ASSERT_EQ(failSpy.count(), 3);
    ASSERT_EQ(listingFailSpy.count(), 3);
}

TEST_F(RemoteFileServiceTest, ListRemoteDirValidEmitsStatus)
{
    QSignalSpy failSpy(&m_service, &RemoteFileService::downloadFailed);
    QSignalSpy statusSpy(&m_service, &RemoteFileService::statusMessage);
    ASSERT_TRUE(failSpy.isValid());
    ASSERT_TRUE(statusSpy.isValid());

    m_service.listRemoteDir("ssh://user@example.com", "/home/user/folder");

    EXPECT_EQ(failSpy.count(), 0);
    ASSERT_EQ(statusSpy.count(), 1);
    EXPECT_TRUE(statusSpy.at(0).at(0).toString().contains("SSH"));
}

TEST_F(RemoteFileServiceTest, ParseListingEmptyOutput)
{
    QList<RemoteFsEntry> entries = RemoteFileService::parseListing(QString(), "/root");
    EXPECT_TRUE(entries.isEmpty());
}

TEST_F(RemoteFileServiceTest, ParseListingDirsAndFiles)
{
    const QString output = QString("./subdir1\n./subdir2\n__DEVPAD_FILES__\n./a.txt\n./b.cpp\n");
    QList<RemoteFsEntry> entries = RemoteFileService::parseListing(output, "/root");

    ASSERT_EQ(entries.size(), 4);
    EXPECT_EQ(entries.at(0).remotePath, "/root/subdir1");
    EXPECT_TRUE(entries.at(0).isDir);
    EXPECT_EQ(entries.at(1).remotePath, "/root/subdir2");
    EXPECT_TRUE(entries.at(1).isDir);
    EXPECT_EQ(entries.at(2).remotePath, "/root/a.txt");
    EXPECT_FALSE(entries.at(2).isDir);
    EXPECT_EQ(entries.at(3).remotePath, "/root/b.cpp");
    EXPECT_FALSE(entries.at(3).isDir);
}

TEST_F(RemoteFileServiceTest, ParseListingTrimsTrailingSlash)
{
    QList<RemoteFsEntry> entries = RemoteFileService::parseListing("./x\n", "/root/parent/");
    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries.at(0).remotePath, "/root/parent/x");
}

TEST_F(RemoteFileServiceTest, ParseIgnoresStrayMarkerLines)
{
    // Marker-like lines that aren't the exact marker, and dot entries, are skipped.
    QList<RemoteFsEntry> entries = RemoteFileService::parseListing("__DEVPAD_FILES__\n__DEVPAD_FILES__X\n./.\n./..\n./ok\n", "/p");
    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries.at(0).remotePath, "/p/ok");
    EXPECT_FALSE(entries.at(0).isDir);
}
