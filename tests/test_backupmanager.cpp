#include "backupmanager.h"
#include "logger.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QUrl>
#include <QTextStream>

#include <gtest/gtest.h>

class BackupManagerTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;

    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());
        Logger::instance().setLogFile(m_tempDir.filePath("test.log"));
        Logger::instance().setLevel(Logger::Warning);

        QDir origDir(m_tempDir.filePath("originals"));
        ASSERT_TRUE(origDir.mkpath("."));
    }

    QString testFilePath(const QString& name = "test.txt") const
    {
        return m_tempDir.filePath("originals/" + name);
    }

    void createOriginalFile(const QString& path, const QString& content = "original")
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << content;
    }
};

TEST_F(BackupManagerTest, SaveBackupCreatesFile)
{
    QString original = testFilePath("save_test.txt");
    createOriginalFile(original, "hello");

    bool result = BackupManager::saveBackup(original, "backup content");
    EXPECT_TRUE(result);
    EXPECT_TRUE(BackupManager::hasBackup(original));
}

TEST_F(BackupManagerTest, GetBackupContent)
{
    QString original = testFilePath("content_test.txt");
    createOriginalFile(original, "original");

    BackupManager::saveBackup(original, "saved backup content");
    QString content = BackupManager::getBackupContent(original);
    EXPECT_EQ(content, "saved backup content");
}

TEST_F(BackupManagerTest, NoBackupForUntitled)
{
    EXPECT_FALSE(BackupManager::hasBackup("Untitled"));
    EXPECT_FALSE(BackupManager::saveBackup("Untitled", "test"));
    EXPECT_TRUE(BackupManager::getBackupContent("Untitled").isEmpty());
}

TEST_F(BackupManagerTest, NoBackupForEmptyPath)
{
    EXPECT_FALSE(BackupManager::hasBackup(""));
    EXPECT_FALSE(BackupManager::saveBackup("", "test"));
    EXPECT_TRUE(BackupManager::getBackupContent("").isEmpty());
}

TEST_F(BackupManagerTest, DeleteBackup)
{
    QString original = testFilePath("delete_test.txt");
    createOriginalFile(original, "to delete");

    BackupManager::saveBackup(original, "delete me");
    EXPECT_TRUE(BackupManager::hasBackup(original));

    bool deleted = BackupManager::deleteBackup(original);
    EXPECT_TRUE(deleted);
    EXPECT_FALSE(BackupManager::hasBackup(original));
}

TEST_F(BackupManagerTest, DeleteNonexistentBackupReturnsFalse)
{
    EXPECT_FALSE(BackupManager::deleteBackup(testFilePath("nonexistent.txt")));
}

TEST_F(BackupManagerTest, BackupDirectoryIsWritable)
{
    QString dir = BackupManager::backupDirectory();
    EXPECT_FALSE(dir.isEmpty());
    QDir qdir(dir);
    EXPECT_TRUE(qdir.exists());
    EXPECT_TRUE(QFileInfo(dir).isWritable());
}

TEST_F(BackupManagerTest, IsBackupNewerWhenOriginalMissing)
{
    QString original = testFilePath("missing.txt");
    BackupManager::saveBackup(original, "orphan backup");
    EXPECT_TRUE(BackupManager::isBackupNewer(original));
}

TEST_F(BackupManagerTest, IsBackupNewerFalseWhenNoBackup)
{
    QString original = testFilePath("no_backup.txt");
    createOriginalFile(original, "no backup");
    EXPECT_FALSE(BackupManager::isBackupNewer(original));
}

TEST_F(BackupManagerTest, BackupPathIsEncoded)
{
    QString original = testFilePath("path with spaces & chars.txt");
    createOriginalFile(original, "content");

    QString backupPath = BackupManager::backupPathForFile(original);
    EXPECT_FALSE(backupPath.isEmpty());
    EXPECT_TRUE(backupPath.contains('%'));
}

TEST_F(BackupManagerTest, UntitledSyntheticKeyRoundTrip)
{
    // Untitled documents are backed up under synthetic "untitled-…" keys.
    QString key = QStringLiteral("untitled-12345-7");
    EXPECT_TRUE(BackupManager::saveBackup(key, "unsaved text"));
    EXPECT_TRUE(BackupManager::hasBackup(key));
    EXPECT_EQ(BackupManager::getBackupContent(key), "unsaved text");

    QStringList keys = BackupManager::backupKeysWithPrefix(QStringLiteral("untitled-"));
    EXPECT_TRUE(keys.contains(key));

    EXPECT_TRUE(BackupManager::deleteBackup(key));
    EXPECT_FALSE(BackupManager::hasBackup(key));
    // Plain "Untitled" is intentionally not a valid backup key.
    EXPECT_FALSE(BackupManager::saveBackup(QStringLiteral("Untitled"), "x"));
}

TEST_F(BackupManagerTest, PurgeKeepsBackupWhenParentDirMissing)
{
    // Simulate a file on an unmounted/detached volume: the original path
    // prefix does not exist. Its backups must survive the purge.
    QTemporaryDir backupDir;
    ASSERT_TRUE(backupDir.isValid());

    QString ghostPath = m_tempDir.filePath(QStringLiteral("definitely_no_such_mount/very/deep/file.txt"));
    QString backupDirPath = backupDir.filePath(QStringLiteral("backups"));
    QDir().mkpath(backupDirPath);

    // Write the backup directly into the isolated directory.
    QString backupPath = QDir(backupDirPath).filePath(QUrl::toPercentEncoding(ghostPath));
    QFile backupFile(backupPath);
    ASSERT_TRUE(backupFile.open(QIODevice::WriteOnly | QIODevice::Text));
    backupFile.write("precious unsaved work");
    backupFile.close();

    BackupManager::purgeStaleBackups(30, backupDirPath);

    EXPECT_TRUE(QFile::exists(backupPath));
}

TEST_F(BackupManagerTest, PurgeRemovesBackupWhenParentDirExists)
{
    QTemporaryDir backupDir;
    ASSERT_TRUE(backupDir.isValid());
    QString backupDirPath = backupDir.filePath(QStringLiteral("backups"));
    QDir().mkpath(backupDirPath);

    // Parent dir "originals" exists; the original file is gone.
    QString missingOriginal = m_tempDir.filePath(QStringLiteral("originals/never_was.txt"));
    QString backupPath = QDir(backupDirPath).filePath(QUrl::toPercentEncoding(missingOriginal));
    QFile backupFile(backupPath);
    ASSERT_TRUE(backupFile.open(QIODevice::WriteOnly | QIODevice::Text));
    backupFile.write("stale");
    backupFile.close();

    BackupManager::purgeStaleBackups(30, backupDirPath);

    EXPECT_FALSE(QFile::exists(backupPath));
}

TEST_F(BackupManagerTest, MultipleIndependentBackups)
{
    QString file1 = testFilePath("multi1.txt");
    QString file2 = testFilePath("multi2.txt");
    createOriginalFile(file1, "one");
    createOriginalFile(file2, "two");

    BackupManager::saveBackup(file1, "backup one");
    BackupManager::saveBackup(file2, "backup two");

    EXPECT_EQ(BackupManager::getBackupContent(file1), "backup one");
    EXPECT_EQ(BackupManager::getBackupContent(file2), "backup two");

    BackupManager::deleteBackup(file1);
    EXPECT_FALSE(BackupManager::hasBackup(file1));
    EXPECT_TRUE(BackupManager::hasBackup(file2));
}
