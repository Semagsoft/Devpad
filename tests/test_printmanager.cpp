#include "codeeditor.h"
#include "printmanager.h"
#include "settingsmanager.h"

#include <QPrinter>
#include <QTextDocument>

#include <gtest/gtest.h>

class PrintManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        m_editor = new CodeEditor();
        m_manager = new PrintManager();
    }

    void TearDown() override
    {
        delete m_manager;
        delete m_editor;
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }

    CodeEditor* m_editor = nullptr;
    PrintManager* m_manager = nullptr;
    std::unique_ptr<SettingsManager> m_testSettings;
};

TEST_F(PrintManagerTest, ConstructorDoesNotCrash)
{
    PrintManager pm;
}

TEST_F(PrintManagerTest, PrintWithNoLexerUsesPlainText)
{
    m_editor->setText("plain text content");
    m_editor->setSyntax("");

    QPrinter printer(QPrinter::HighResolution);
    // Just verify no crash when printing plain text
    EXPECT_NO_THROW(m_manager->printEditorWithHighlighting(m_editor, &printer));
}

TEST_F(PrintManagerTest, PrintWithCppLexerDoesNotCrash)
{
    m_editor->setText("int main() { return 0; }");
    m_editor->setSyntax("cpp");
    ASSERT_NE(m_editor->lexer(), nullptr);

    QPrinter printer(QPrinter::HighResolution);
    EXPECT_NO_THROW(m_manager->printEditorWithHighlighting(m_editor, &printer));
}

TEST_F(PrintManagerTest, PrintWithPythonLexerDoesNotCrash)
{
    m_editor->setText("def foo():\n    pass");
    m_editor->setSyntax("python");
    ASSERT_NE(m_editor->lexer(), nullptr);

    QPrinter printer(QPrinter::HighResolution);
    EXPECT_NO_THROW(m_manager->printEditorWithHighlighting(m_editor, &printer));
}

TEST_F(PrintManagerTest, PrintWithMultiLineDoesNotCrash)
{
    m_editor->setText("line one\nline two\nline three\nline four\nline five\n");
    m_editor->setSyntax("cpp");

    QPrinter printer(QPrinter::HighResolution);
    EXPECT_NO_THROW(m_manager->printEditorWithHighlighting(m_editor, &printer));
}

TEST_F(PrintManagerTest, PrintEmptyEditorDoesNotCrash)
{
    m_editor->setText("");
    m_editor->setSyntax("cpp");

    QPrinter printer(QPrinter::HighResolution);
    EXPECT_NO_THROW(m_manager->printEditorWithHighlighting(m_editor, &printer));
}

TEST_F(PrintManagerTest, EditorWithoutSyntaxHasNoLexer)
{
    m_editor->setText("plain text");
    m_editor->setSyntax("");
    EXPECT_EQ(m_editor->lexer(), nullptr);
}

TEST_F(PrintManagerTest, PrintManagerIsQObject)
{
    EXPECT_NE(qobject_cast<QObject*>(m_manager), nullptr);
}
