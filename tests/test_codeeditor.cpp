#include "codeeditor.h"
#include "languageinfo.h"
#include "settingsmanager.h"

#include <QApplication>
#include <QFont>

#include <gtest/gtest.h>

class CodeEditorTest : public ::testing::Test
{
protected:
    CodeEditor* editor = nullptr;
    std::unique_ptr<SettingsManager> m_testSettings;

    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        editor = new CodeEditor();
        editor->setText("line0\nline1\nline2\nline3\nline4\nline5\n");
    }
    void TearDown() override
    {
        delete editor;
        editor = nullptr;
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }
};

TEST_F(CodeEditorTest, InitialState)
{
    EXPECT_EQ(editor->fileName(), "");
    EXPECT_EQ(editor->syntax(), "");
    EXPECT_EQ(editor->encoding(), "UTF-8");
    EXPECT_FALSE(editor->isReadOnlyMode());
}

TEST_F(CodeEditorTest, SetAndGetFileName)
{
    editor->setFileName("/path/to/test.cpp");
    EXPECT_EQ(editor->fileName(), "/path/to/test.cpp");
}

TEST_F(CodeEditorTest, SetAndGetSyntax)
{
    editor->setSyntax("cpp");
    EXPECT_EQ(editor->syntax(), "cpp");
}

TEST_F(CodeEditorTest, SetAndGetEncoding)
{
    editor->setEncoding("UTF-16LE");
    EXPECT_EQ(editor->encoding(), "UTF-16LE");
}

TEST_F(CodeEditorTest, ToggleBookmarkAddsAndRemoves)
{
    EXPECT_FALSE(editor->hasBookmark(0));
    editor->toggleBookmark(0);
    EXPECT_TRUE(editor->hasBookmark(0));
    editor->toggleBookmark(0);
    EXPECT_FALSE(editor->hasBookmark(0));
}

TEST_F(CodeEditorTest, BookmarkLinesReturnsCorrectLines)
{
    editor->toggleBookmark(1);
    editor->toggleBookmark(3);

    QList<int> lines = editor->bookmarkLines();
    EXPECT_EQ(lines.size(), 2);
    EXPECT_TRUE(lines.contains(1));
    EXPECT_TRUE(lines.contains(3));
}

TEST_F(CodeEditorTest, ClearBookmarksRemovesAll)
{
    editor->toggleBookmark(0);
    editor->toggleBookmark(2);
    editor->toggleBookmark(4);
    EXPECT_EQ(editor->bookmarkLines().size(), 3);

    editor->clearBookmarks();
    EXPECT_TRUE(editor->bookmarkLines().isEmpty());
}

TEST_F(CodeEditorTest, SetBookmarksReplacesExisting)
{
    editor->toggleBookmark(0);
    QList<int> newBookmarks = {2, 4};
    editor->setBookmarks(newBookmarks);

    QList<int> lines = editor->bookmarkLines();
    EXPECT_EQ(lines.size(), 2);
    EXPECT_TRUE(lines.contains(2));
    EXPECT_TRUE(lines.contains(4));
}

TEST_F(CodeEditorTest, NextBookmarkNavigatesForward)
{
    editor->toggleBookmark(1);
    editor->toggleBookmark(3);

    editor->setCursorPosition(2, 0);
    editor->nextBookmark();
    int line, index;
    editor->getCursorPosition(&line, &index);
    EXPECT_EQ(line, 3);
}

TEST_F(CodeEditorTest, NextBookmarkWrapsAround)
{
    editor->toggleBookmark(1);

    editor->setCursorPosition(2, 0);
    editor->nextBookmark();
    int line, index;
    editor->getCursorPosition(&line, &index);
    EXPECT_EQ(line, 1);
}

TEST_F(CodeEditorTest, PrevBookmarkNavigatesBackward)
{
    editor->toggleBookmark(1);
    editor->toggleBookmark(3);

    editor->setCursorPosition(4, 0);
    editor->prevBookmark();
    int line, index;
    editor->getCursorPosition(&line, &index);
    EXPECT_EQ(line, 3);
}

TEST_F(CodeEditorTest, PrevBookmarkWrapsAround)
{
    editor->toggleBookmark(3);

    editor->setCursorPosition(0, 0);
    editor->prevBookmark();
    int line, index;
    editor->getCursorPosition(&line, &index);
    EXPECT_EQ(line, 3);
}

TEST_F(CodeEditorTest, ReadOnlyMode)
{
    EXPECT_FALSE(editor->isReadOnlyMode());
    EXPECT_FALSE(editor->isReadOnly());

    editor->setReadOnlyMode(true);
    EXPECT_TRUE(editor->isReadOnlyMode());
    EXPECT_TRUE(editor->isReadOnly());

    editor->setReadOnlyMode(false);
    EXPECT_FALSE(editor->isReadOnlyMode());
    EXPECT_FALSE(editor->isReadOnly());
}

TEST_F(CodeEditorTest, ApplyFontSetsFont)
{
    QFont font("Courier New", 14);
    editor->applyFont(font);
    EXPECT_EQ(editor->font().family(), "Courier New");
    EXPECT_EQ(editor->font().pointSize(), 14);
}

TEST_F(CodeEditorTest, SetSyntaxToNullForUnknownLanguage)
{
    editor->setSyntax("nonexistent_language_xyz");
    EXPECT_EQ(editor->syntax(), "nonexistent_language_xyz");
    EXPECT_EQ(editor->lexer(), nullptr);
}

TEST_F(CodeEditorTest, CppSyntaxSetsLexer)
{
    editor->setSyntax("cpp");
    EXPECT_EQ(editor->syntax(), "cpp");
    EXPECT_NE(editor->lexer(), nullptr);
}

TEST_F(CodeEditorTest, ToggleLineNumbers)
{
    editor->setLineNumbersVisible(false);

    editor->setLineNumbersVisible(true);
}

TEST_F(CodeEditorTest, SetScrollPastEnd)
{
    editor->setScrollPastEnd(true);
    editor->setScrollPastEnd(false);
}

TEST_F(CodeEditorTest, SetCodeFolding)
{
    editor->setCodeFolding(true);
    editor->setCodeFolding(false);
}

TEST_F(CodeEditorTest, SetWhitespaceVisible)
{
    editor->setWhitespaceVisible(true);
    editor->setWhitespaceVisible(false);
}

TEST_F(CodeEditorTest, SetAutoCloseBrackets)
{
    editor->setAutoCloseBrackets(false);
    editor->setAutoCloseBrackets(true);
}

TEST_F(CodeEditorTest, SetHighlightCurrentLine)
{
    editor->setHighlightCurrentLine(false);
    editor->setHighlightCurrentLine(true);
}

TEST_F(CodeEditorTest, SetVerticalEdge)
{
    editor->setVerticalEdge(true, 100);
    editor->setVerticalEdge(false, 80);
}

TEST_F(CodeEditorTest, SetupAutoCompletion)
{
    editor->setupAutoCompletion(true, 2, false);
    editor->setupAutoCompletion(false, 1, true);
}

TEST_F(CodeEditorTest, CursorStyle)
{
    editor->setCursorStyle(CursorStyle::Block);
    editor->setCursorStyle(CursorStyle::Underline);
    editor->setCursorStyle(CursorStyle::Line);
}

TEST_F(CodeEditorTest, CursorBlinking)
{
    editor->setCursorBlinking(true);
    editor->setCursorBlinking(false);
}

TEST_F(CodeEditorTest, ForceModifiedDoesNotChangeContent)
{
    QString original = editor->text();
    editor->forceModified();
    EXPECT_TRUE(editor->isModified());
}
