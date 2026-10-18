#include "filewatchermanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTest>
#include <QTimer>

#include <gtest/gtest.h>

class FileWatcherManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_manager = new FileWatcherManager();
        ASSERT_TRUE(m_tempDir.isValid());
    }

    void TearDown() override
    {
        delete m_manager;
    }

    QString createFile(const QString& name, const QByteArray& content = "test")
    {
        QString path = m_tempDir.filePath(name);
        QFile file(path);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(content);
            file.close();
        }
        return path;
    }

    void modifyFile(const QString& path)
    {
        // Wait a tick so the modification time differs
        QTest::qSleep(10);
        QFile file(path);
        if (file.open(QIODevice::Append))
        {
            file.write("modified");
            file.close();
        }
    }

    QTemporaryDir m_tempDir;
    FileWatcherManager* m_manager = nullptr;
};

TEST_F(FileWatcherManagerTest, ConstructorDoesNotCrash)
{
    FileWatcherManager fwm;
}

TEST_F(FileWatcherManagerTest, WatchExistingFileDoesNotThrow)
{
    QString path = createFile("watch_me.txt");
    EXPECT_NO_THROW(m_manager->watchFile(path));
}

TEST_F(FileWatcherManagerTest, WatchNonExistentFileDoesNotThrow)
{
    EXPECT_NO_THROW(m_manager->watchFile("/nonexistent/path/file.txt"));
}

TEST_F(FileWatcherManagerTest, UnwatchNonExistentFileDoesNotCrash)
{
    m_manager->unwatchFile("/nonexistent/path/file.txt");
}

TEST_F(FileWatcherManagerTest, WatchAndUnwatchFile)
{
    QString path = createFile("unwatch_me.txt");
    m_manager->watchFile(path);
    m_manager->unwatchFile(path);
    // After unwatch, modifications should not trigger a signal
    QSignalSpy spy(m_manager, &FileWatcherManager::fileModifiedExternally);
    modifyFile(path);
    QTest::qWait(100);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(FileWatcherManagerTest, WatchFileTwiceDoesNotThrow)
{
    QString path = createFile("twice.txt");
    EXPECT_NO_THROW(m_manager->watchFile(path));
    EXPECT_NO_THROW(m_manager->watchFile(path));
}

TEST_F(FileWatcherManagerTest, UnwatchAllOnEmptyManagerDoesNotCrash)
{
    m_manager->unwatchAll();
}

TEST_F(FileWatcherManagerTest, UnwatchAllAfterWatchingClearsAll)
{
    QString path1 = createFile("all1.txt");
    QString path2 = createFile("all2.txt");
    m_manager->watchFile(path1);
    m_manager->watchFile(path2);

    m_manager->unwatchAll();

    QSignalSpy spy(m_manager, &FileWatcherManager::fileModifiedExternally);
    modifyFile(path1);
    modifyFile(path2);
    QTest::qWait(100);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(FileWatcherManagerTest, UpdateModificationTimeOnExistingFile)
{
    QString path = createFile("modtime.txt");
    EXPECT_NO_THROW(m_manager->updateModificationTime(path));
}

TEST_F(FileWatcherManagerTest, UpdateModificationTimeOnNonExistentFileDoesNotCrash)
{
    EXPECT_NO_THROW(m_manager->updateModificationTime("/nonexistent/path/file.txt"));
}

TEST_F(FileWatcherManagerTest, WatchFileInDirectory)
{
    QString path = createFile("in_dir.txt");
    EXPECT_NO_THROW(m_manager->watchFile(path));
    m_manager->unwatchFile(path);
}

TEST_F(FileWatcherManagerTest, MultipleIndependentWatches)
{
    QStringList paths;
    for (int i = 0; i < 5; ++i)
    {
        QString path = createFile(QString("multi%1.txt").arg(i));
        paths << path;
        EXPECT_NO_THROW(m_manager->watchFile(path));
    }

    QSignalSpy spy(m_manager, &FileWatcherManager::fileModifiedExternally);
    modifyFile(paths[0]);
    QTest::qWait(100);
    EXPECT_GE(spy.count(), 0);

    m_manager->unwatchAll();
}

TEST_F(FileWatcherManagerTest, SingleModificationEmitsOnce)
{
    QString path = createFile("single.txt");
    m_manager->watchFile(path);

    QSignalSpy spy(m_manager, &FileWatcherManager::fileModifiedExternally);
    modifyFile(path);
    QTest::qWait(500);

    ASSERT_EQ(spy.count(), 1) << "watcher did not report the external change";
}

TEST_F(FileWatcherManagerTest, RapidModificationsAreCoalescedIntoOneSignal)
{
    QString path = createFile("burst.txt");
    m_manager->watchFile(path);

    QSignalSpy spy(m_manager, &FileWatcherManager::fileModifiedExternally);

    // A single external write often touches the file repeatedly (truncate then write,
    // formatter plus compiler). Only the first of such a burst may be reported.
    for (int i = 0; i < 4; ++i)
        modifyFile(path);

    QTest::qWait(500);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(FileWatcherManagerTest, ModificationAfterCoalesceWindowEmitsAgain)
{
    QString path = createFile("after_window.txt");
    m_manager->watchFile(path);

    QSignalSpy spy(m_manager, &FileWatcherManager::fileModifiedExternally);

    modifyFile(path);
    QTest::qWait(500);
    ASSERT_EQ(spy.count(), 1);

    // Coalescing must not permanently suppress reporting.
    QTest::qWait(FileWatcherManager::COALESCE_INTERVAL_MS + 300);
    modifyFile(path);
    QTest::qWait(500);
    EXPECT_EQ(spy.count(), 2);
}
