#include "codeeditor.h"
#include "editorcontroller.h"
#include "filemanager.h"
#include "mainwindow.h"
#include "settingsmanager.h"
#include "tabmanager.h"
#include "terminalpanel.h"
#include "theme.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <cstdio>
#include <cstdlib>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Counts how many QMessageBox dialogs are shown anywhere in the application.
class MessageBoxShowCounter : public QObject
{
public:
    int count = 0;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::Show && qobject_cast<QMessageBox*>(obj))
            ++count;

        return QObject::eventFilter(obj, event);
    }
};

class MainWindowTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    std::unique_ptr<SettingsManager> m_testSettings;

    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());

        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        SettingsManager::instance().setAutoSaveEnabled(false);
        SettingsManager::instance().setStartupMode(StartupMode::Empty);
    }

    void TearDown() override
    {
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }

    QString testFilePath(const QString& name) const
    {
        return m_tempDir.filePath(name);
    }

    void createFile(const QString& path, const QString& content = "test content")
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        QTextStream out(&file);
        out << content;
    }

    // Opens a file so the external-change prompt has an editor to act on.
    void openWatchedFile(MainWindow& window, const QString& name, const QString& content = "original")
    {
        QString filePath = testFilePath(name);
        createFile(filePath, content);
        window.openFileFromPath(filePath);
    }

    // Invokes the private onFileModifiedExternally slot, dismissing any prompt it raises
    // with No. Re-enters the slot `reentries` times from inside that prompt's nested event
    // loop, mimicking watcher signals delivered while the dialog is already up.
    // Dialogs shown are accumulated in `counter`, which may span several calls.
    void runExternalChangePrompt(MainWindow& window, const QString& filePath, MessageBoxShowCounter& counter, int reentries = 0)
    {
        // Installed on the application so it sees every event, including those of the
        // dialog (which is not reachable from the main window's modal state here).
        qApp->installEventFilter(&counter);

        QTimer ticker;
        int tick = 0;
        QObject::connect(&ticker, &QTimer::timeout, &window,
                         [&]()
                         {
                             if (tick++ < reentries)
                             {
                                 QMetaObject::invokeMethod(&window, "onFileModifiedExternally", Qt::DirectConnection, Q_ARG(QString, filePath));
                                 return;
                             }

                             // Dismiss every open prompt so its exec() returns. With the guard
                             // in place there is at most one; a regression that stacks dialogs
                             // still unwinds instead of hanging.
                             const auto prompts = window.findChildren<QMessageBox*>();
                             for (auto* prompt : prompts)
                                 prompt->close();
                         });
        ticker.start(1);

        // Stacked modal loops cannot always be unwound from here, so give up loudly instead
        // of hanging the suite when the guard is missing.
        QTimer watchdog;
        int budget = 0;
        QObject::connect(&watchdog, &QTimer::timeout, &window,
                         [&]()
                         {
                             if (++budget <= 40)
                                 return;

                             fprintf(stderr, "'File Modified' prompts never dismissed: the one-dialog guard is missing\n");
                             fflush(stderr);
                             std::_Exit(1);
                         });
        watchdog.start(50);

        QMetaObject::invokeMethod(&window, "onFileModifiedExternally", Qt::DirectConnection, Q_ARG(QString, filePath));

        ticker.stop();
        watchdog.stop();
        qApp->removeEventFilter(&counter);
    }
};

TEST_F(MainWindowTest, ConstructorDoesNotCrash)
{
    MainWindow* window = new MainWindow();
    EXPECT_NE(window, nullptr);
    delete window;
}

TEST_F(MainWindowTest, TerminalPanelHiddenInTabModeWhenDisabled)
{
    SettingsManager::instance().setTerminalPanelPosition(TerminalPanelPosition::Tab);
    SettingsManager::instance().setShowTerminalPanel(false);

    MainWindow window;
    window.show();

    auto* terminal = window.findChild<TerminalPanel*>();
    ASSERT_NE(terminal, nullptr);
    EXPECT_FALSE(terminal->isVisible());

    for (QTabWidget* tw : window.findChildren<QTabWidget*>())
    {
        EXPECT_EQ(tw->indexOf(terminal), -1) << "Disabled terminal panel must not be added as a tab";
    }
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

TEST_F(MainWindowTest, ExternalChangePromptShownOnlyOnceWhileOpen)
{
    MainWindow window;
    openWatchedFile(window, "external_burst.txt");

    QString filePath = testFilePath("external_burst.txt");
    ASSERT_NE(window.findChild<CodeEditor*>(), nullptr);

    // Five re-entrant signals arrive while the first prompt is still showing. Exactly one
    // dialog must be raised; the rest are dropped rather than stacked on top of it.
    MessageBoxShowCounter counter;
    runExternalChangePrompt(window, filePath, counter, 5);
    EXPECT_EQ(counter.count, 1);
}

TEST_F(MainWindowTest, ExternalChangePromptShownAgainAfterDismissal)
{
    MainWindow window;
    openWatchedFile(window, "external_again.txt");

    QString filePath = testFilePath("external_again.txt");
    ASSERT_NE(window.findChild<CodeEditor*>(), nullptr);

    // The guard must not latch: a later, genuinely new change prompts again.
    MessageBoxShowCounter counter;
    runExternalChangePrompt(window, filePath, counter);
    EXPECT_EQ(counter.count, 1);
    runExternalChangePrompt(window, filePath, counter);
    EXPECT_EQ(counter.count, 2);
}

TEST_F(MainWindowTest, ExternalChangePromptSkippedForFileNotOpen)
{
    MainWindow window;
    openWatchedFile(window, "external_open.txt");

    // No editor matches this path, so no prompt should be raised at all.
    MessageBoxShowCounter counter;
    runExternalChangePrompt(window, testFilePath("never_opened.txt"), counter);
    EXPECT_EQ(counter.count, 0);
}
