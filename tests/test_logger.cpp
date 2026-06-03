#include <gtest/gtest.h>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include "logger.h"

class LoggerTest : public ::testing::Test {
protected:
    QTemporaryDir m_tempDir;

    void SetUp() override {
        ASSERT_TRUE(m_tempDir.isValid());
        QString logPath = m_tempDir.filePath("test.log");
        Logger::instance().setLogFile(logPath);
        Logger::instance().setLevel(Logger::Debug);
    }

    void TearDown() override {
        Logger::instance().setLogFile(
            QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + "/devpad/devpad.log"
        );
        Logger::instance().setLevel(Logger::Debug);
    }

    QString logFilePath() const {
        return m_tempDir.filePath("test.log");
    }

    QString readLog() const {
        QFile file(logFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};
        return QString::fromUtf8(file.readAll());
    }
};

TEST_F(LoggerTest, LogFileIsCreated) {
    Logger::instance().info("test message");
    QFileInfo fi(logFilePath());
    EXPECT_TRUE(fi.exists());
}

TEST_F(LoggerTest, InfoMessageAppearsInLog) {
    Logger::instance().info("hello world");
    QString content = readLog();
    EXPECT_TRUE(content.contains("hello world"));
    EXPECT_TRUE(content.contains("INFO"));
}

TEST_F(LoggerTest, DebugMessageAppearsAtDebugLevel) {
    Logger::instance().debug("debug test");
    QString content = readLog();
    EXPECT_TRUE(content.contains("debug test"));
    EXPECT_TRUE(content.contains("DEBUG"));
}

TEST_F(LoggerTest, WarningMessageAppears) {
    Logger::instance().warning("warning test");
    QString content = readLog();
    EXPECT_TRUE(content.contains("warning test"));
    EXPECT_TRUE(content.contains("WARN"));
}

TEST_F(LoggerTest, ErrorMessageAppears) {
    Logger::instance().error("error test");
    QString content = readLog();
    EXPECT_TRUE(content.contains("error test"));
    EXPECT_TRUE(content.contains("ERROR"));
}

TEST_F(LoggerTest, CriticalMessageAppears) {
    Logger::instance().critical("critical test");
    QString content = readLog();
    EXPECT_TRUE(content.contains("critical test"));
    EXPECT_TRUE(content.contains("CRITICAL"));
}

TEST_F(LoggerTest, LevelFiltering_InfoOnlyHidesDebug) {
    Logger::instance().setLevel(Logger::Info);
    Logger::instance().debug("should be hidden");
    Logger::instance().info("should appear");

    QString content = readLog();
    EXPECT_FALSE(content.contains("should be hidden"));
    EXPECT_TRUE(content.contains("should appear"));
}

TEST_F(LoggerTest, LevelFiltering_WarningOnly) {
    Logger::instance().setLevel(Logger::Warning);
    Logger::instance().info("hidden info");
    Logger::instance().warning("visible warning");
    Logger::instance().error("visible error");

    QString content = readLog();
    EXPECT_FALSE(content.contains("hidden info"));
    EXPECT_TRUE(content.contains("visible warning"));
    EXPECT_TRUE(content.contains("visible error"));
}

TEST_F(LoggerTest, LogContainsTimestamp) {
    Logger::instance().info("timestamp check");
    QString content = readLog();
    EXPECT_TRUE(content.contains(QDateTime::currentDateTime().toString("yyyy")));
}

TEST_F(LoggerTest, MultipleMessages) {
    Logger::instance().info("first");
    Logger::instance().warning("second");
    Logger::instance().error("third");

    QString content = readLog();
    EXPECT_TRUE(content.contains("first"));
    EXPECT_TRUE(content.contains("second"));
    EXPECT_TRUE(content.contains("third"));
}

TEST_F(LoggerTest, SetLogFileChangesDestination) {
    Logger::instance().info("original file");

    QString newPath = m_tempDir.filePath("newlog.log");
    Logger::instance().setLogFile(newPath);
    Logger::instance().info("new file");

    QString originalContent = [&]() {
        QFile f(logFilePath());
        return f.open(QIODevice::ReadOnly | QIODevice::Text) ? f.readAll() : QString();
    }();
    QString newContent = [&]() {
        QFile f(newPath);
        return f.open(QIODevice::ReadOnly | QIODevice::Text) ? f.readAll() : QString();
    }();

    EXPECT_TRUE(originalContent.contains("original file"));
    EXPECT_FALSE(originalContent.contains("new file"));
    EXPECT_TRUE(newContent.contains("new file"));
}
