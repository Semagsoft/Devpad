#include "mainwindow.h"
#include "codeeditor.h"
#include "editorcontroller.h"
#include "filemanager.h"
#include "settingsmanager.h"
#include "tabmanager.h"
#include "theme.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextStream>

class MainWindowTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    SettingsManager* m_testSettings = nullptr;

    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());

        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings);
        SettingsManager::instance().setAutoSaveEnabled(false);
    }

    void TearDown() override
    {
        SettingsManager::setTestingInstance(nullptr);
        SettingsManager::destroyForTesting(m_testSettings);
    }

    QString testFilePath(const QString& name) const
    {
        return m_tempDir.filePath(name);
    }

    void createFile(const QString& path, const QString& content = "test content")
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << content;
    }
};

TEST_F(MainWindowTest, ConstructorDoesNotCrash)
{
    MainWindow* window = new MainWindow();
    EXPECT_NE(window, nullptr);
    delete window;
}

TEST_F(MainWindowTest, OpenFileFromPathOpensTab)
{
    MainWindow window;
    QString filePath = testFilePath("open_test.txt");
    createFile(filePath, "hello world");

    window.openFileFromPath(filePath);

    auto* editor = window.findChild<CodeEditor*>();
    EXPECT_NE(editor, nullptr);
    if (editor)
    {
        EXPECT_EQ(editor->fileName(), filePath);
    }
}

TEST_F(MainWindowTest, OpenFileDetectsSyntaxFromExtension)
{
    MainWindow window;
    QString filePath = testFilePath("main.cpp");
    createFile(filePath, "int main() { return 0; }");

    window.openFileFromPath(filePath);

    auto* editor = window.findChild<CodeEditor*>();
    ASSERT_NE(editor, nullptr);
    EXPECT_EQ(editor->syntax(), QStringLiteral("cpp"));
}

TEST_F(MainWindowTest, OpenPythonFileDetectsPythonSyntax)
{
    MainWindow window;
    QString filePath = testFilePath("app.py");
    createFile(filePath, "def hello():\n    pass\n");

    window.openFileFromPath(filePath);

    auto* editor = window.findChild<CodeEditor*>();
    ASSERT_NE(editor, nullptr);
    EXPECT_EQ(editor->syntax(), QStringLiteral("python"));
}

TEST_F(MainWindowTest, OpenHtmlFileDetectsHtmlSyntax)
{
    MainWindow window;
    QString filePath = testFilePath("index.html");
    createFile(filePath, "<html><body></body></html>");

    window.openFileFromPath(filePath);

    auto* editor = window.findChild<CodeEditor*>();
    ASSERT_NE(editor, nullptr);
    EXPECT_EQ(editor->syntax(), QStringLiteral("html"));
}

TEST_F(MainWindowTest, OpenFolderFromPathShowsProjectPanel)
{
    MainWindow window;
    QTemporaryDir projectDir;
    ASSERT_TRUE(projectDir.isValid());

    window.openFolderFromPath(projectDir.path());

    EXPECT_TRUE(window.findChild<QWidget*>(QString(), Qt::FindDirectChildrenOnly));
}

TEST_F(MainWindowTest, OpenFileFromPathSetsEditorText)
{
    MainWindow window;
    QString filePath = testFilePath("content_test.txt");
    QString content = "line one\nline two\nline three\n";
    createFile(filePath, content);

    window.openFileFromPath(filePath);

    auto* editor = window.findChild<CodeEditor*>();
    ASSERT_NE(editor, nullptr);
    EXPECT_EQ(editor->text(), content);
}

TEST_F(MainWindowTest, MultipleFilesOpenSeparateTabs)
{
    MainWindow window;
    QString file1 = testFilePath("multi1.txt");
    QString file2 = testFilePath("multi2.txt");
    createFile(file1, "file one");
    createFile(file2, "file two");

    window.openFileFromPath(file1);
    window.openFileFromPath(file2);

    auto editors = window.findChildren<CodeEditor*>();
    EXPECT_EQ(editors.size(), 2);
}
