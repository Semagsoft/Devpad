#include "editorcontroller.h"
#include "actionmanager.h"
#include "appstrings.h"
#include "backupmanager.h"
#include "codeeditor.h"
#include "filemanager.h"
#include "filewatchermanager.h"
#include "languageinfo.h"
#include "settingsmanager.h"
#include "tabmanager.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QApplication>
#include <QObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTabWidget>
#include <QTextStream>
#include <QFile>

class EditorControllerTest : public ::testing::Test
{
protected:
    QTemporaryDir m_tempDir;
    QTabWidget* tabWidget = nullptr;
    TabManager* tabManager = nullptr;
    FileManager* fileManager = nullptr;
    FileWatcherManager* fileWatcherManager = nullptr;
    ActionManager* actionManager = nullptr;
    EditorController* controller = nullptr;
    std::unique_ptr<SettingsManager> m_testSettings;

    void SetUp() override
    {
        ASSERT_TRUE(m_tempDir.isValid());

        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        SettingsManager::instance().setAutoSaveEnabled(false);

        tabWidget = new QTabWidget();
        tabManager = new TabManager(tabWidget);
        fileManager = new FileManager();
        fileWatcherManager = new FileWatcherManager();
        actionManager = new ActionManager();
        actionManager->createActions();

        controller = new EditorController(
            tabManager, fileManager, fileWatcherManager,
            actionManager, tabWidget
        );
    }

    void TearDown() override
    {
        delete controller;
        delete actionManager;
        delete fileWatcherManager;
        delete fileManager;
        delete tabManager;
        delete tabWidget;

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
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << content;
    }
};

TEST_F(EditorControllerTest, InitiallyNoEditor)
{
    EXPECT_EQ(controller->currentEditor(), nullptr);
    EXPECT_EQ(controller->findEditorByFileName("any.txt"), nullptr);
}

TEST_F(EditorControllerTest, NewFileCreatesEditor)
{
    controller->newFile();
    EXPECT_NE(tabManager->currentEditor(), nullptr);
    EXPECT_EQ(tabManager->count(), 1);
    EXPECT_EQ(tabManager->currentEditor()->fileName(), Strings::untitled());
}

TEST_F(EditorControllerTest, NewFileEmitsEditorConnected)
{
    QSignalSpy spy(controller, &EditorController::editorConnected);
    controller->newFile();
    EXPECT_EQ(spy.count(), 1);
    auto* editor = qvariant_cast<CodeEditor*>(spy.at(0).at(0));
    EXPECT_NE(editor, nullptr);
}

TEST_F(EditorControllerTest, CurrentEditorDelegatesToTabManager)
{
    controller->newFile();
    EXPECT_EQ(controller->currentEditor(), tabManager->currentEditor());
}

TEST_F(EditorControllerTest, FindEditorByFileNameDelegatesToTabManager)
{
    controller->newFile();
    auto* editor = tabManager->currentEditor();
    editor->setFileName("/path/to/test.cpp");

    EXPECT_EQ(controller->findEditorByFileName("/path/to/test.cpp"), editor);
    EXPECT_EQ(controller->findEditorByFileName("/nonexistent"), nullptr);
}

TEST_F(EditorControllerTest, NewFileAppliesDefaultFormat)
{
    SettingsManager::instance().setDefaultFormat(0);
    controller->newFile();
    auto* editor = tabManager->currentEditor();
    QStringList langs = {"cpp", "c", "csharp", "java", "python", "javascript",
                         "typescript", "html", "css", "xml", "sql", "rust",
                         "go", "markdown", "bash", "cmake"};
    ASSERT_GT(langs.size(), 0);
    EXPECT_FALSE(editor->syntax().isEmpty());
}

TEST_F(EditorControllerTest, SaveFileWithNoEditorReturnsFalse)
{
    EXPECT_FALSE(controller->saveFile());
}

TEST_F(EditorControllerTest, UndoRedoWithNoEditorNoCrash)
{
    controller->undo();
    controller->redo();
}

TEST_F(EditorControllerTest, CutCopyPasteWithNoEditorNoCrash)
{
    controller->cut();
    controller->copy();
    controller->paste();
}

TEST_F(EditorControllerTest, SelectAllDeleteWithNoEditorNoCrash)
{
    controller->selectAll();
    controller->deleteText();
}

TEST_F(EditorControllerTest, ZoomWithNoEditorNoCrash)
{
    controller->zoomIn();
    controller->zoomOut();
    controller->zoomReset();
}

TEST_F(EditorControllerTest, BookmarkOperationsWithNoEditorNoCrash)
{
    controller->toggleBookmark();
    controller->nextBookmark();
    controller->prevBookmark();
    controller->clearBookmarks();
}

TEST_F(EditorControllerTest, UpdateUndoRedoStateWithNoEditorDisablesActions)
{
    controller->updateUndoRedoState();
    EXPECT_FALSE(actionManager->undoAct()->isEnabled());
    EXPECT_FALSE(actionManager->redoAct()->isEnabled());
}

TEST_F(EditorControllerTest, UpdateReadOnlyActionStateWithNoEditorUnchecked)
{
    controller->updateReadOnlyActionState();
    EXPECT_FALSE(actionManager->readOnlyAct()->isChecked());
}

TEST_F(EditorControllerTest, UndoRedoStateUpdatesWithEditor)
{
    controller->newFile();
    auto* editor = tabManager->currentEditor();
    controller->updateUndoRedoState();

    EXPECT_FALSE(actionManager->undoAct()->isEnabled());
    EXPECT_FALSE(actionManager->redoAct()->isEnabled());

    editor->insert("hello");
    controller->updateUndoRedoState();
    EXPECT_TRUE(actionManager->undoAct()->isEnabled());
    EXPECT_FALSE(actionManager->redoAct()->isEnabled());
}

TEST_F(EditorControllerTest, CallbackOperationsOnCurrentEditor)
{
    controller->newFile();
    auto* editor = tabManager->currentEditor();

    editor->selectAll();
    editor->setModified(true);

    controller->undo();
    controller->redo();
    controller->cut();
    controller->copy();
    controller->paste();
    controller->zoomIn();
    controller->zoomOut();
    controller->zoomReset();

    SUCCEED();
}

TEST_F(EditorControllerTest, ToggleReadOnlyChangesEditorState)
{
    controller->newFile();
    auto* editor = tabManager->currentEditor();

    EXPECT_FALSE(editor->isReadOnlyMode());
    controller->toggleReadOnly();
    EXPECT_TRUE(editor->isReadOnlyMode());
    controller->toggleReadOnly();
    EXPECT_FALSE(editor->isReadOnlyMode());
}

TEST_F(EditorControllerTest, ToggleReadOnlyUpdatesAction)
{
    controller->newFile();
    controller->toggleReadOnly();
    EXPECT_TRUE(actionManager->readOnlyAct()->isChecked());
}

TEST_F(EditorControllerTest, ConnectEditorSignalsEmitsSignal)
{
    auto* editor = new CodeEditor();
    QSignalSpy spy(controller, &EditorController::editorConnected);
    controller->connectEditorSignals(editor);
    EXPECT_EQ(spy.count(), 1);
    delete editor;
}

TEST_F(EditorControllerTest, AutoSaveWithDisabledSettingDoesNothing)
{
    SettingsManager::instance().setAutoSaveEnabled(false);
    controller->newFile();
    auto* editor = tabManager->currentEditor();
    editor->setText("modified content");

    controller->autoSave();
}

TEST_F(EditorControllerTest, AutoSaveWithNamedFileAndOriginalMode)
{
    SettingsManager::instance().setAutoSaveEnabled(true);
    SettingsManager::instance().setAutoSaveToOriginalFile(true);

    QString filePath = testFilePath("autosave_test.txt");
    createFile(filePath, "original");

    controller->newFile();
    auto* editor = tabManager->currentEditor();
    editor->setFileName(filePath);
    editor->setText("new content");
    editor->setModified(true);

    QSignalSpy spy(controller, &EditorController::fileSaved);
    controller->autoSave();

    EXPECT_EQ(spy.count(), 1);
    QFile savedFile(filePath);
    ASSERT_TRUE(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    EXPECT_EQ(QString::fromUtf8(savedFile.readAll()), "new content");
}

TEST_F(EditorControllerTest, AutoSaveWithBackupMode)
{
    SettingsManager::instance().setAutoSaveEnabled(true);
    SettingsManager::instance().setAutoSaveToOriginalFile(false);

    QString filePath = testFilePath("backup_autosave.txt");
    createFile(filePath, "original");

    controller->newFile();
    auto* editor = tabManager->currentEditor();
    editor->setFileName(filePath);
    editor->setText("backup content");
    editor->setModified(true);

    controller->autoSave();

    EXPECT_TRUE(BackupManager::hasBackup(filePath));
    EXPECT_EQ(BackupManager::getBackupContent(filePath), "backup content");
    BackupManager::deleteBackup(filePath);
}

TEST_F(EditorControllerTest, MultipleNewEditorsGetDistinctInstances)
{
    controller->newFile();
    auto* first = tabManager->currentEditor();
    controller->newFile();
    auto* second = tabManager->currentEditor();

    EXPECT_NE(first, second);
    EXPECT_EQ(tabManager->count(), 2);
}

TEST_F(EditorControllerTest, UpdateReadOnlyActionStateWithEditor)
{
    controller->newFile();
    auto* editor = tabManager->currentEditor();

    controller->updateReadOnlyActionState();
    EXPECT_FALSE(actionManager->readOnlyAct()->isChecked());

    editor->setReadOnlyMode(true);
    controller->updateReadOnlyActionState();
    EXPECT_TRUE(actionManager->readOnlyAct()->isChecked());

    editor->setReadOnlyMode(false);
    controller->updateReadOnlyActionState();
    EXPECT_FALSE(actionManager->readOnlyAct()->isChecked());
}

TEST_F(EditorControllerTest, SaveModifiedNamedFile)
{
    QString path = testFilePath("save_named.txt");
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "original content";
    }

    CodeEditor* editor = controller->openFile(path);
    ASSERT_NE(editor, nullptr);

    editor->setText("modified content");
    editor->setModified(true);

    bool result = controller->saveFile();
    EXPECT_TRUE(result);

    QFile savedFile(path);
    ASSERT_TRUE(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    EXPECT_EQ(QString::fromUtf8(savedFile.readAll()), "modified content");
}

TEST_F(EditorControllerTest, SaveNamedFileMultipleTimes)
{
    QString path = testFilePath("multi_save.txt");
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "hello";
    }

    CodeEditor* editor = controller->openFile(path);
    ASSERT_NE(editor, nullptr);

    for (int i = 0; i < 10; i++) {
        editor->setText(QString("modified %1").arg(i));
        editor->setModified(true);
        EXPECT_TRUE(controller->saveFile());
    }

    QFile savedFile(path);
    ASSERT_TRUE(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
    EXPECT_EQ(QString::fromUtf8(savedFile.readAll()), "modified 9");
}

TEST_F(EditorControllerTest, SaveNamedFileWithEventProcessing)
{
    QString path = testFilePath("event_save.txt");
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "original";
    }

    CodeEditor* editor = controller->openFile(path);
    ASSERT_NE(editor, nullptr);
    editor->setText("modified");
    editor->setModified(true);
    EXPECT_TRUE(controller->saveFile());

    QApplication::processEvents();

    editor->setText("modified again");
    editor->setModified(true);
    EXPECT_TRUE(controller->saveFile());
}

TEST_F(EditorControllerTest, SaveWithFileSavedSpy)
{
    QString path = testFilePath("spy_save.txt");
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "original";
    }

    CodeEditor* editor = controller->openFile(path);
    ASSERT_NE(editor, nullptr);

    QSignalSpy spy(controller, &EditorController::fileSaved);

    editor->setText("modified");
    editor->setModified(true);
    EXPECT_TRUE(controller->saveFile());
    EXPECT_EQ(spy.count(), 1);

    // Editor should still be valid
    EXPECT_FALSE(editor->isModified());
    EXPECT_EQ(editor->fileName(), path);
}

// Simulate the EXACT crash scenario: file watcher fires after save
TEST_F(EditorControllerTest, FileWatcherFiresAfterSave)
{
    QString path = testFilePath("watcher_after_save.txt");
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "original";
    }

    CodeEditor* editor = controller->openFile(path);
    ASSERT_NE(editor, nullptr);

    editor->setText("modified");
    editor->setModified(true);

    // Connect fileModifiedExternally like MainWindow does
    QPointer<CodeEditor> editorPtr = editor;
    QObject::connect(fileWatcherManager, &FileWatcherManager::fileModifiedExternally,
                     fileWatcherManager, [editorPtr](const QString& filePath) {
        // This simulates the MainWindow::onFileModifiedExternally behavior
        if (!editorPtr) return;  // QPointer null-check after dialog
        // Access the editor - this would crash with raw pointer
        editorPtr->isModified();
        editorPtr->bookmarkLines();
        editorPtr->encoding();
    });

    EXPECT_TRUE(controller->saveFile());

    // Force processing of pending file watcher events
    QApplication::processEvents();
}

// Save with concurrent external modification notification
TEST_F(EditorControllerTest, SaveTriggersExternalModification)
{
    QString path = testFilePath("ext_mod_save.txt");
    {
        QFile file(path);
        ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream out(&file);
        out << "original";
    }

    CodeEditor* editor = controller->openFile(path);
    ASSERT_NE(editor, nullptr);

    editor->setText("modified");
    editor->setModified(true);

    // Simulate what happens when file watcher fires AFTER save
    // by manually calling onFileChanged with a stale timestamp
    QPointer<CodeEditor> editorPtr = editor;
    QObject::connect(fileWatcherManager, &FileWatcherManager::fileModifiedExternally,
                     fileWatcherManager, [editorPtr](const QString&) {
        if (editorPtr) {
            // Touch the editor - shouldn't crash with QPointer
            editorPtr->setModified(true);
        }
    });

    // Save once
    EXPECT_TRUE(controller->saveFile());
    QApplication::processEvents();

    // Modify external timestamps to simulate race condition
    // by touching the file externally
    {
        QFile file(path);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream out(&file);
        out << "externally modified";
    }

    // Process events to trigger file watcher
    QApplication::processEvents();

    // Editor should still be usable if QPointer guard works
    if (editorPtr) {
        EXPECT_FALSE(editorPtr->isModified());
    }
}
