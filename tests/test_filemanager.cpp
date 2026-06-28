#include "codeeditor.h"
#include "filemanager.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTextStream>

#include <gtest/gtest.h>

class FileManagerTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;

    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());
    }

    QString testFilePath(const QString& name = "test.txt") const
    {
        return m_tempDir.filePath(name);
    }

    void createFile(const QString& path, const QByteArray& content)
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(content);
        file.close();
    }
};

TEST_F(FileManagerTest, DetectEncodingUtf8Bom)
{
    QByteArray data = QByteArray("\xEF\xBB\xBFHello", 8);
    EXPECT_EQ(FileManager::detectEncoding(data), "UTF-8");
}

TEST_F(FileManagerTest, DetectEncodingUtf8NoBom)
{
    QByteArray data("Hello, World!");
    EXPECT_EQ(FileManager::detectEncoding(data), "UTF-8");
}

TEST_F(FileManagerTest, DetectEncodingUtf16BE)
{
    QByteArray data = QByteArray("\xFE\xFF\x00H\x00e\x00l\x00l\x00o", 10);
    EXPECT_EQ(FileManager::detectEncoding(data), "UTF-16BE");
}

TEST_F(FileManagerTest, DetectEncodingUtf16LE)
{
    QByteArray data = QByteArray("\xFF\xFEH\x00e\x00l\x00l\x00o\x00", 10);
    EXPECT_EQ(FileManager::detectEncoding(data), "UTF-16LE");
}

TEST_F(FileManagerTest, DetectEncodingUtf32BE)
{
    QByteArray data = QByteArray("\x00\x00\xFE\xFF\x00\x00\x00H\x00\x00\x00e", 12);
    EXPECT_EQ(FileManager::detectEncoding(data), "UTF-32BE");
}

TEST_F(FileManagerTest, DetectEncodingUtf32LE)
{
    QByteArray data = QByteArray("\xFF\xFE\x00\x00H\x00\x00\x00e\x00\x00\x00", 12);
    EXPECT_EQ(FileManager::detectEncoding(data), "UTF-32LE");
}

TEST_F(FileManagerTest, DetectEncodingEmptyReturnsUtf8)
{
    QByteArray data;
    EXPECT_EQ(FileManager::detectEncoding(data), "UTF-8");
}

TEST_F(FileManagerTest, DetectEncodingLatin1)
{
    QByteArray data;
    data.append(static_cast<char>(0xE9)); // é in Latin-1, invalid UTF-8 lead byte
    data.append(static_cast<char>(0xE0));
    EXPECT_EQ(FileManager::detectEncoding(data), "ISO-8859-1");
}

TEST_F(FileManagerTest, DetectEncodingUtf8Multibyte)
{
    QByteArray data;
    data.append("Hello ", 6);
    data.append(static_cast<char>(0xC3));
    data.append(static_cast<char>(0xA9));
    data.append(static_cast<char>(0xC3));
    data.append(static_cast<char>(0xA0));
    data.append(" World", 6);
    EXPECT_EQ(FileManager::detectEncoding(data), "UTF-8");
}

TEST_F(FileManagerTest, LoadFileNullEditorReturnsFalse)
{
    FileManager mgr;
    bool result = mgr.loadFile("/nonexistent/file.txt", nullptr);
    EXPECT_FALSE(result);
    EXPECT_FALSE(mgr.lastError().isEmpty());
}

TEST_F(FileManagerTest, SaveFileNullEditorReturnsFalse)
{
    FileManager mgr;
    bool result = mgr.saveFile("/tmp/test.txt", nullptr);
    EXPECT_FALSE(result);
    EXPECT_FALSE(mgr.lastError().isEmpty());
}

TEST_F(FileManagerTest, LoadFileWithNullEditorReturnsFalse)
{
    FileManager mgr;
    bool result = mgr.loadFile("/nonexistent/path/file.txt", nullptr);
    EXPECT_FALSE(result);
}

TEST_F(FileManagerTest, LastErrorIsEmptyByDefault)
{
    FileManager mgr;
    EXPECT_TRUE(mgr.lastError().isEmpty());
}

TEST_F(FileManagerTest, SaveAndLoadRoundTrip)
{
    CodeEditor editor;
    editor.setText("Hello, World!\nSecond line");
    editor.setFileName(testFilePath("roundtrip.txt"));

    FileManager mgr;
    EXPECT_TRUE(mgr.saveFile(testFilePath("roundtrip.txt"), &editor));

    CodeEditor editor2;
    EXPECT_TRUE(mgr.loadFile(testFilePath("roundtrip.txt"), &editor2));
    EXPECT_EQ(editor2.text(), "Hello, World!\nSecond line");
    EXPECT_EQ(editor2.fileName(), testFilePath("roundtrip.txt"));
    EXPECT_FALSE(editor2.isModified());
}

TEST_F(FileManagerTest, SaveFileWithEncoding)
{
    CodeEditor editor;
    editor.setText("Test content");
    editor.setEncoding("UTF-8");

    FileManager mgr;
    EXPECT_TRUE(mgr.saveFile(testFilePath("utf8.txt"), &editor, "UTF-8"));

    QFile file(testFilePath("utf8.txt"));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QByteArray content = file.readAll();
    file.close();
    // FileManager writes UTF-8 BOM prefix for UTF-8 encoding
    EXPECT_TRUE(content.contains(QByteArray("Test content")));
    EXPECT_TRUE(content.startsWith(QByteArray("\xEF\xBB\xBF", 3)));
    EXPECT_EQ(FileManager::detectEncoding(content), "UTF-8");
}

TEST_F(FileManagerTest, SaveWithBom)
{
    CodeEditor editor;
    editor.setText("BOM test");
    editor.setEncoding("UTF-8");

    FileManager mgr;
    EXPECT_TRUE(mgr.saveFile(testFilePath("bom_test.txt"), &editor, "UTF-8"));

    QFile file(testFilePath("bom_test.txt"));
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QByteArray content = file.readAll();
    file.close();

    // UTF-8 BOM is EF BB BF
    ASSERT_GE(content.size(), 3);
    EXPECT_EQ(content.mid(0, 3), QByteArray("\xEF\xBB\xBF", 3));
    EXPECT_EQ(content.mid(3), QByteArray("BOM test"));
}

TEST_F(FileManagerTest, LoadNonExistentFile)
{
    CodeEditor editor;
    FileManager mgr;
    EXPECT_FALSE(mgr.loadFile(testFilePath("nonexistent.txt"), &editor));
    EXPECT_FALSE(mgr.lastError().isEmpty());
}

TEST_F(FileManagerTest, SaveToNonWritableLocation)
{
    CodeEditor editor;
    editor.setText("test");
    editor.setFileName("/nonexistent_dir/file.txt");

    FileManager mgr;
    EXPECT_FALSE(mgr.saveFile("/nonexistent_dir/file.txt", &editor));
    EXPECT_FALSE(mgr.lastError().isEmpty());
}

TEST_F(FileManagerTest, SaveAndReloadPreservesUtf8Content)
{
    CodeEditor editor;
    QString unicodeText = QString::fromUtf8("Hello \u00E9\u00E0\u00FC World! \u2603"); // éàü ☃
    editor.setText(unicodeText);

    FileManager mgr;
    EXPECT_TRUE(mgr.saveFile(testFilePath("unicode.txt"), &editor, "UTF-8"));

    CodeEditor editor2;
    EXPECT_TRUE(mgr.loadFile(testFilePath("unicode.txt"), &editor2));
    EXPECT_EQ(editor2.text(), unicodeText);
}

TEST_F(FileManagerTest, SaveAndReloadRoundTripEncodingPreserved)
{
    CodeEditor editor;
    editor.setText("Encoding test");
    editor.setEncoding("UTF-16LE");

    FileManager mgr;
    EXPECT_TRUE(mgr.saveFile(testFilePath("utf16le.txt"), &editor, "UTF-16LE"));

    CodeEditor editor2;
    EXPECT_TRUE(mgr.loadFile(testFilePath("utf16le.txt"), &editor2, "UTF-16LE"));
    EXPECT_EQ(editor2.text(), "Encoding test");
}
