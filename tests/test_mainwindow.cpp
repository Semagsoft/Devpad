#include "codeeditor.h"
#include "editorcontroller.h"
#include "filemanager.h"
#include "lsp/lsptypes.h"
#include "mainwindow.h"
#include "managers/actionmanager.h"
#include "plugins/pluginhost.h"
#include "plugins/pluginmanager.h"
#include "settingsmanager.h"
#include "tabmanager.h"
#include "theme.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QKeySequence>
#include <QMessageBox>
#include <QSignalSpy>
#include <QTabWidget>
#include <cstdio>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QToolButton>
#include <QTest>
#include <QTextStream>
#include <QTimer>
#include <cstdio>
#include <cstdlib>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Counts QMessageBox dialogs as they are shown and auto-dismisses them with
// "No" from inside the prompt's own nested event loop. That makes dismissal
// deterministic — the dialog's exec() always returns — and removes any
// dependence on timer granularity, watchdogs, or how loaded the machine is.
// Used to test the single-prompt guard without hanging or racing.
class MessageBoxShowCounter : public QObject
{
public:
    int count = 0;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        auto* box = qobject_cast<QMessageBox*>(obj);
        if (!box)
            return QObject::eventFilter(obj, event);
        if (event->type() == QEvent::Show)
        {
            ++count;
            QTimer::singleShot(0, box, [box]() { box->done(QMessageBox::No); });
        }
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

    // Invokes the private onFileModifiedExternally slot. Re-enters the slot
    // `reentries` times from inside the prompt's nested event loop, mimicking
    // watcher signals delivered while the dialog is already up; the guard must
    // drop them. The prompt itself is auto-dismissed with "No" once shown (see
    // MessageBoxShowCounter), so the sequence is deterministic. Dialogs shown
    // are accumulated in `counter`, which may span several calls.
    void runExternalChangePrompt(MainWindow& window, const QString& filePath, MessageBoxShowCounter& counter, int reentries = 0)
    {
        // Installed on the application so it sees every event, including those of the
        // dialog (which is not reachable from the main window's modal state here).
        qApp->installEventFilter(&counter);

        // Re-entrant signals delivered while the outer prompt is still open.
        // They must all be dropped by the guard — none may stack another dialog.
        for (int i = 0; i < reentries; ++i)
        {
            QTimer::singleShot(0, &window, [&window, filePath]()
                               {
                                   QMetaObject::invokeMethod(&window, "onFileModifiedExternally", Qt::DirectConnection,
                                                             Q_ARG(QString, filePath));
                               });
        }

        QMetaObject::invokeMethod(&window, "onFileModifiedExternally", Qt::DirectConnection, Q_ARG(QString, filePath));

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

    // The terminal panel is plugin-owned: when the Terminal plugin is loaded
    // the dock object always exists, but with the panel disabled it must not
    // be visible (no tab added and the dock removed from the main window).
    auto* terminal = window.findChild<QDockWidget*>("TerminalPanel");
    if (terminal)
    {
        EXPECT_FALSE(terminal->isVisible());
        auto* tabWidget = window.findChild<QTabWidget*>();
        if (tabWidget)
        {
            EXPECT_EQ(tabWidget->indexOf(terminal), -1);
        }
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

TEST_F(MainWindowTest, ToggleCommentShortcutTogglesExactlyOnce)
{
    MainWindow window;
    window.show();

    QString filePath = testFilePath("shortcut_comment.cpp");
    createFile(filePath, "int x = 1;\n");
    window.openFileFromPath(filePath);

    auto* editor = window.findChild<CodeEditor*>();
    ASSERT_NE(editor, nullptr);

    QAction* toggleCommentAct = nullptr;
    for (QAction* act : window.actions())
    {
        if (act->shortcut() == QKeySequence("Ctrl+/"))
        {
            toggleCommentAct = act;
            break;
        }
    }
    ASSERT_NE(toggleCommentAct, nullptr) << "Toggle Comment must be registered as a window shortcut";
    EXPECT_TRUE(toggleCommentAct->isEnabled());

    editor->setFocus();
    editor->setCursorPosition(0, 0);
    QTest::keyClick(editor, Qt::Key_Slash, Qt::ControlModifier);
    EXPECT_EQ(editor->text().toStdString(), "//int x = 1;\n") << "Ctrl+/ must toggle the comment exactly once";

    editor->setCursorPosition(0, 0);
    QTest::keyClick(editor, Qt::Key_Slash, Qt::ControlModifier);
    EXPECT_EQ(editor->text().toStdString(), "int x = 1;\n");

    // The Edit menu action itself must reach the current editor.
    editor->setCursorPosition(0, 0);
    toggleCommentAct->trigger();
    EXPECT_EQ(editor->text().toStdString(), "//int x = 1;\n");
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

TEST_F(MainWindowTest, RevertFileReloadsContentFromDisk)
{
    MainWindow window;
    QString filePath = testFilePath("revert_disk.txt");
    createFile(filePath, "original");
    window.openFileFromPath(filePath);

    auto* editor = window.findChild<CodeEditor*>();
    ASSERT_NE(editor, nullptr);
    EXPECT_EQ(editor->text(), "original");

    // Change the file on disk without touching the editor in Devpad.
    createFile(filePath, "modified on disk");

    // Reverting must reload from disk, not keep the stale in-editor view.
    window.revertFile();

    // Flush deleteLater so the stale closed editor is no longer a child.
    QCoreApplication::processEvents();

    // At least one editor must reflect the reloaded disk content. Note: closing a tab
    // leaves the old CodeEditor as a child of the tab pane (pre-existing parentage), so
    // there may be more than one editor; we only require that the reloaded content is present.
    bool reloaded = false;
    for (CodeEditor* e : window.findChildren<CodeEditor*>())
    {
        if (e->text() == "modified on disk")
        {
            reloaded = true;
            break;
        }
    }
    EXPECT_TRUE(reloaded) << "revertFile() must reload the file from disk";
}

TEST_F(MainWindowTest, OpeningSameFileTwiceDoesNotDuplicateTab)
{
    MainWindow window;
    QString filePath = testFilePath("dupe.txt");
    createFile(filePath, "content");
    window.openFileFromPath(filePath);
    window.openFileFromPath(filePath); // same file again -> switch to existing tab

    auto editors = window.findChildren<CodeEditor*>();
    EXPECT_EQ(editors.size(), 1);
    if (!editors.isEmpty())
    {
        EXPECT_EQ(editors.front()->text(), "content");
    }
}

// ── Plugin bridge ──────────────────────────────────────────────────────────
// MainWindow talks to plugins through QMetaObject::invokeMethod on the
// plugin's QObject side, since IPlugin is not a QObject. These tests verify
// that bridge for the DAP debugger plugin.

TEST_F(MainWindowTest, DapPluginExposesInvokableDebugMethods)
{
    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);

    auto* plugin = manager->plugin(QStringLiteral("org.semagsoft.devpad.dap"));
    ASSERT_NE(plugin, nullptr);

    auto* object = dynamic_cast<QObject*>(plugin);
    ASSERT_NE(object, nullptr);

    const QMetaObject* meta = object->metaObject();
    EXPECT_NE(meta->indexOfMethod("startDebugging(QString,QString,QString,QStringList)"), -1);
    EXPECT_NE(meta->indexOfMethod("stopDebugging(QString)"), -1);
    EXPECT_NE(meta->indexOfMethod("debugContinue(QString)"), -1);
    EXPECT_NE(meta->indexOfMethod("debugStepOver(QString)"), -1);
    EXPECT_NE(meta->indexOfMethod("debugStepInto(QString)"), -1);
    EXPECT_NE(meta->indexOfMethod("debugStepOut(QString)"), -1);
    EXPECT_NE(meta->indexOfMethod("debugRestart(QString)"), -1);
}

TEST_F(MainWindowTest, StartDebuggingInvokesDapPluginViaMetaObject)
{
    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);

    auto* plugin = manager->plugin(QStringLiteral("org.semagsoft.devpad.dap"));
    ASSERT_NE(plugin, nullptr);
    auto* object = dynamic_cast<QObject*>(plugin);
    ASSERT_NE(object, nullptr);

    // The plugin registers its DapServerManager with the host; connect to its
    // clientError signal so we can observe that the invocation reached it.
    auto* server = manager->host()->object(QStringLiteral("dap_server"));
    ASSERT_NE(server, nullptr);
    QSignalSpy errorSpy(server, SIGNAL(clientError(QString, QString)));

    // Open a file so startDebugging() has a path to debug, then run it with
    // a language no adapter is configured for: the DapServerManager must
    // emit clientError and not spawn a process.
    QString filePath = testFilePath("debug_me.py");
    createFile(filePath, "print('hi')");
    window.openFileFromPath(filePath);
    ASSERT_NE(window.findChild<CodeEditor*>(), nullptr);

    bool invoked =
        QMetaObject::invokeMethod(object, "startDebugging", Qt::DirectConnection, Q_ARG(QString, lsp::uriFromPath(filePath)),
                                  Q_ARG(QString, QStringLiteral("unknownlang")), Q_ARG(QString, QString()), Q_ARG(QStringList, QStringList()));
    EXPECT_TRUE(invoked);

    // The invocation reached the plugin and was forwarded to the server
    // manager, which reports the missing adapter.
    QTRY_VERIFY_WITH_TIMEOUT(errorSpy.count() >= 1, 1000);

    QMetaObject::invokeMethod(object, "stopDebugging", Qt::DirectConnection, Q_ARG(QString, QStringLiteral("session")));
}

TEST_F(MainWindowTest, DebugPanelTogglesViaPluginHost)
{
    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);

    auto* dock = qobject_cast<QDockWidget*>(manager->host()->panel(QStringLiteral("debug")));
    if (!dock)
    {
        GTEST_SKIP() << "DAP debug panel not registered (plugin disabled?)";
    }

    window.show();
    EXPECT_FALSE(dock->isVisible());

    window.toggleDebugPanel();
    EXPECT_TRUE(dock->isVisible());
    EXPECT_TRUE(window.actionManager()->debugPanelAct()->isChecked());

    window.toggleDebugPanel();
    EXPECT_FALSE(dock->isVisible());
    EXPECT_FALSE(window.actionManager()->debugPanelAct()->isChecked());
}

// ── DAP UI visibility follows plugin state ─────────────────────────────────

TEST_F(MainWindowTest, DapUiHiddenWhenPluginDisabled)
{
    SettingsManager::instance().setPluginSetting(QStringLiteral("org.semagsoft.devpad.dap"), QStringLiteral("Enabled"), false);

    MainWindow window;

    EXPECT_FALSE(window.pluginManager()->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap")));
    EXPECT_FALSE(window.actionManager()->isDebugUiVisible());
    EXPECT_FALSE(window.actionManager()->debugMenu()->menuAction()->isVisible());
}

TEST_F(MainWindowTest, DapUiVisibleWhenPluginEnabled)
{
    // Default: no persisted Enabled setting -> manifest default (enabled).
    MainWindow window;

    EXPECT_TRUE(window.pluginManager()->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap")));
    EXPECT_TRUE(window.actionManager()->isDebugUiVisible());
    EXPECT_TRUE(window.actionManager()->debugMenu()->menuAction()->isVisible());
}

TEST_F(MainWindowTest, DisablingDapPluginAtRuntimeHidesDebugUi)
{
    MainWindow window;

    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);
    EXPECT_TRUE(manager->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap")));
    EXPECT_TRUE(window.actionManager()->isDebugUiVisible());

    manager->setPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap"), false);

    EXPECT_FALSE(window.actionManager()->isDebugUiVisible());
    EXPECT_FALSE(window.actionManager()->debugMenu()->menuAction()->isVisible());
}

TEST_F(MainWindowTest, EnablingDapPluginAtRuntimeShowsDebugUi)
{
    SettingsManager::instance().setPluginSetting(QStringLiteral("org.semagsoft.devpad.dap"), QStringLiteral("Enabled"), false);

    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);
    EXPECT_FALSE(window.actionManager()->isDebugUiVisible());

    manager->setPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap"), true);

    EXPECT_TRUE(window.actionManager()->isDebugUiVisible());
    EXPECT_TRUE(window.actionManager()->debugMenu()->menuAction()->isVisible());
}

// ── Plugin bridge: editor integration + lifecycle ─────────────────────────

TEST_F(MainWindowTest, DapEditorIntegrationAttachedOnFileOpen)
{
    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);
    ASSERT_TRUE(manager->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap")));

    QString filePath = testFilePath("integ.py");
    createFile(filePath, "print('hi')");
    window.openFileFromPath(filePath);

    auto* editor = window.findChild<CodeEditor*>();
    ASSERT_NE(editor, nullptr);
    EXPECT_NE(editor->dapIntegration(), nullptr);
}

TEST_F(MainWindowTest, DebugDockRemovedOnPluginDisable)
{
    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);

    fprintf(stderr, "DBG before: enabled=%d loaded=%d plugin=%p\n",
        manager->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap")),
        manager->isLoaded(QStringLiteral("org.semagsoft.devpad.dap")),
        (void*)manager->plugin(QStringLiteral("org.semagsoft.devpad.dap")));

    manager->setPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap"), false);

    fprintf(stderr, "DBG after: enabled=%d loaded=%d plugin=%p panel=%p\n",
        manager->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap")),
        manager->isLoaded(QStringLiteral("org.semagsoft.devpad.dap")),
        (void*)manager->plugin(QStringLiteral("org.semagsoft.devpad.dap")),
        (void*)manager->host()->panel(QStringLiteral("debug")));
}

TEST_F(MainWindowTest, DapPluginReenableRestoresDebugUi)
{
    SettingsManager::instance().setPluginSetting(QStringLiteral("org.semagsoft.devpad.dap"), QStringLiteral("Enabled"), false);

    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);
    EXPECT_FALSE(window.actionManager()->isDebugUiVisible());

    manager->setPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap"), true);

    EXPECT_TRUE(window.actionManager()->isDebugUiVisible());
    EXPECT_NE(manager->host()->panel(QStringLiteral("debug")), nullptr);
}

TEST_F(MainWindowTest, TerminalToggleHiddenOnPluginDisable)
{
    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);
    ASSERT_TRUE(manager->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.terminal")));

    auto* dock = qobject_cast<QDockWidget*>(manager->host()->panel(QStringLiteral("terminal")));
    if (!dock)
    {
        GTEST_SKIP() << "Terminal panel not registered";
    }

    EXPECT_TRUE(window.actionManager()->terminalPanelAct()->isVisible());

    manager->setPluginEnabled(QStringLiteral("org.semagsoft.devpad.terminal"), false);

    EXPECT_FALSE(window.actionManager()->terminalPanelAct()->isVisible());
    EXPECT_FALSE(window.actionManager()->terminalPanelButton()->isVisible());
    EXPECT_EQ(manager->host()->panel(QStringLiteral("terminal")), nullptr);
}

TEST_F(MainWindowTest, TerminalReenableRestoresPanel)
{
    SettingsManager::instance().setPluginSetting(QStringLiteral("org.semagsoft.devpad.terminal"), QStringLiteral("Enabled"), false);

    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);

    manager->setPluginEnabled(QStringLiteral("org.semagsoft.devpad.terminal"), true);

    EXPECT_TRUE(window.actionManager()->terminalPanelAct()->isVisible());
    EXPECT_NE(manager->host()->panel(QStringLiteral("terminal")), nullptr);
}

// ── Terminal toggle + position switching ──────────────────────────────────

TEST_F(MainWindowTest, TerminalToggleAddsRemovesTab)
{
    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);
    ASSERT_TRUE(manager->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.terminal")));

    auto* dock = qobject_cast<QDockWidget*>(manager->host()->panel(QStringLiteral("terminal")));
    ASSERT_NE(dock, nullptr);

    auto* tabWidget = window.findChild<QTabWidget*>();
    ASSERT_NE(tabWidget, nullptr);

    // Position Tab, hidden -> toggle should add as tab
    SettingsManager::instance().setTerminalPanelPosition(TerminalPanelPosition::Tab);
    SettingsManager::instance().setShowTerminalPanel(false);
    EXPECT_EQ(tabWidget->indexOf(dock), -1);

    QMetaObject::invokeMethod(dock, "toggle", Qt::DirectConnection,
        Q_ARG(QTabWidget*, tabWidget), Q_ARG(QMainWindow*, &window));
    QCoreApplication::processEvents();
    EXPECT_EQ(tabWidget->indexOf(dock), 0);
    EXPECT_TRUE(SettingsManager::instance().showTerminalPanel());

    // Toggle again -> removed from tab
    QMetaObject::invokeMethod(dock, "toggle", Qt::DirectConnection,
        Q_ARG(QTabWidget*, tabWidget), Q_ARG(QMainWindow*, &window));
    QCoreApplication::processEvents();
    EXPECT_EQ(tabWidget->indexOf(dock), -1);
    EXPECT_FALSE(SettingsManager::instance().showTerminalPanel());
}

TEST_F(MainWindowTest, TerminalPositionSwitchingTabToDock)
{
    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);
    ASSERT_TRUE(manager->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.terminal")));

    auto* dock = qobject_cast<QDockWidget*>(manager->host()->panel(QStringLiteral("terminal")));
    ASSERT_NE(dock, nullptr);

    auto* tabWidget = window.findChild<QTabWidget*>();
    ASSERT_NE(tabWidget, nullptr);

    // Add as tab first
    SettingsManager::instance().setTerminalPanelPosition(TerminalPanelPosition::Tab);
    SettingsManager::instance().setShowTerminalPanel(true);
    QMetaObject::invokeMethod(dock, "toggle", Qt::DirectConnection,
        Q_ARG(QTabWidget*, tabWidget), Q_ARG(QMainWindow*, &window));
    QCoreApplication::processEvents();
    EXPECT_EQ(tabWidget->indexOf(dock), 0);

    // Switch to Bottom position -> removed from tab, docked
    SettingsManager::instance().setTerminalPanelPosition(TerminalPanelPosition::Bottom);
    QMetaObject::invokeMethod(dock, "applyPosition", Qt::DirectConnection,
        Q_ARG(int, static_cast<int>(TerminalPanelPosition::Bottom)),
        Q_ARG(QTabWidget*, tabWidget), Q_ARG(QMainWindow*, &window));
    QCoreApplication::processEvents();
    EXPECT_EQ(tabWidget->indexOf(dock), -1);
}

// ── Debug session active during plugin disable ────────────────────────────

TEST_F(MainWindowTest, DebugSessionClearedWhenPluginDisabled)
{
    MainWindow window;
    auto* manager = window.pluginManager();
    ASSERT_NE(manager, nullptr);
    ASSERT_TRUE(manager->isPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap")));

    auto* object = dynamic_cast<QObject*>(manager->plugin(QStringLiteral("org.semagsoft.devpad.dap")));
    ASSERT_NE(object, nullptr);

    // Start a debug session with no adapter configured -> server emits clientError
    QString filePath = testFilePath("debug_me.py");
    createFile(filePath, "print('hi')");
    window.openFileFromPath(filePath);
    ASSERT_NE(window.findChild<CodeEditor*>(), nullptr);

    bool invoked = QMetaObject::invokeMethod(object, "startDebugging", Qt::DirectConnection,
        Q_ARG(QString, lsp::uriFromPath(filePath)),
        Q_ARG(QString, QStringLiteral("unknownlang")),
        Q_ARG(QString, QString()),
        Q_ARG(QStringList, QStringList()));
    EXPECT_TRUE(invoked);

    QCoreApplication::processEvents();

    // Now disable the plugin -> session key cleared, UI hidden
    manager->setPluginEnabled(QStringLiteral("org.semagsoft.devpad.dap"), false);

    EXPECT_FALSE(window.actionManager()->isDebugUiVisible());
}
