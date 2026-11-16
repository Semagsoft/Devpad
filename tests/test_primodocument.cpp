#include "nextgen/primodocument.h"

#include <gtest/gtest.h>

// Exercises PrimoDocument's incremental undo model. Regression tests for the
// double-apply bug: edits used to be applied once directly and a second time
// by QUndoStack::push() -> redo(), doubling inserted text on every operation.

TEST(PrimoDocumentTest, InsertTextTwice)
{
    PrimoDocument doc;
    doc.setText(QString());

    doc.insertText(0, QStringLiteral("a"));
    doc.insertText(1, QStringLiteral("b"));

    EXPECT_EQ(doc.text(), QStringLiteral("ab"));
}

TEST(PrimoDocumentTest, InsertTextUndoRedoRoundTrip)
{
    PrimoDocument doc;
    doc.insertText(0, QStringLiteral("a"));
    doc.insertText(1, QStringLiteral("b"));
    EXPECT_EQ(doc.text(), QStringLiteral("ab"));

    doc.undo();
    EXPECT_EQ(doc.text(), QStringLiteral("a"));

    doc.undo();
    EXPECT_EQ(doc.text(), QStringLiteral(""));

    doc.redo();
    EXPECT_EQ(doc.text(), QStringLiteral("a"));

    doc.redo();
    EXPECT_EQ(doc.text(), QStringLiteral("ab"));
}

TEST(PrimoDocumentTest, InsertMultiCharUndoRemove)
{
    PrimoDocument doc;
    doc.insertText(0, QStringLiteral("hello"));
    EXPECT_EQ(doc.text(), QStringLiteral("hello"));

    doc.undo();
    EXPECT_EQ(doc.text(), QStringLiteral(""));

    doc.redo();
    EXPECT_EQ(doc.text(), QStringLiteral("hello"));
}

TEST(PrimoDocumentTest, RemoveTextUndoRedoRoundTrip)
{
    PrimoDocument doc;
    doc.setText(QStringLiteral("hello world"));
    doc.clearUndoStack(); // ignore any undo entries from setText history

    doc.removeText(5, 6); // remove " world"
    EXPECT_EQ(doc.text(), QStringLiteral("hello"));

    doc.undo();
    EXPECT_EQ(doc.text(), QStringLiteral("hello world"));

    doc.redo();
    EXPECT_EQ(doc.text(), QStringLiteral("hello"));
}

TEST(PrimoDocumentTest, SetTextDiffUndoRedoRoundTrip)
{
    PrimoDocument doc;
    doc.setText(QStringLiteral("hello"));
    doc.clearUndoStack();

    doc.setText(QStringLiteral("heXXo world"));
    EXPECT_EQ(doc.text(), QStringLiteral("heXXo world"));

    doc.undo();
    EXPECT_EQ(doc.text(), QStringLiteral("hello"));

    doc.redo();
    EXPECT_EQ(doc.text(), QStringLiteral("heXXo world"));
}

TEST(PrimoDocumentTest, SetLineUndoRedoRoundTrip)
{
    PrimoDocument doc;
    doc.setText(QStringLiteral("first\nsecond\nthird"));
    doc.clearUndoStack();

    doc.setLine(1, QStringLiteral("SECOND"));
    EXPECT_EQ(doc.text(), QStringLiteral("first\nSECOND\nthird"));

    doc.undo();
    EXPECT_EQ(doc.text(), QStringLiteral("first\nsecond\nthird"));

    doc.redo();
    EXPECT_EQ(doc.text(), QStringLiteral("first\nSECOND\nthird"));
}

TEST(PrimoDocumentTest, UndoThenNewEditClearsRedo)
{
    PrimoDocument doc;
    doc.insertText(0, QStringLiteral("a"));
    doc.insertText(1, QStringLiteral("b"));

    doc.undo(); // "a"
    doc.insertText(1, QStringLiteral("X")); // "aX"

    EXPECT_EQ(doc.text(), QStringLiteral("aX"));
    EXPECT_FALSE(doc.canRedo());
}

TEST(PrimoDocumentTest, SingleNewlineInsertAppliesOnce)
{
    // Regression: the double-apply turned one typed newline into two.
    PrimoDocument doc;
    doc.insertText(0, QStringLiteral("line1"));
    doc.clearUndoStack();
    doc.insertText(5, QStringLiteral("\n"));

    EXPECT_EQ(doc.text(), QStringLiteral("line1\n"));
    EXPECT_EQ(doc.lineCount(), 2);
}

TEST(PrimoDocumentTest, TextChangedEmittedExactlyOncePerEdit)
{
    PrimoDocument doc;
    doc.setText(QStringLiteral("abc"));

    int emitted = 0;
    QObject::connect(&doc, &PrimoDocument::textChanged, [&emitted]() { ++emitted; });

    doc.insertText(3, QStringLiteral("d"));
    EXPECT_EQ(emitted, 1);

    doc.removeText(0, 2);
    EXPECT_EQ(emitted, 2);

    doc.setLine(0, QStringLiteral("XX"));
    EXPECT_EQ(emitted, 3);
}

TEST(PrimoDocumentTest, UndoEmitsTextChangedExactlyOnce)
{
    PrimoDocument doc;
    doc.setText(QStringLiteral("abc"));
    doc.clearUndoStack();
    doc.insertText(3, QStringLiteral("d"));

    int emitted = 0;
    QObject::connect(&doc, &PrimoDocument::textChanged, [&emitted]() { ++emitted; });

    doc.undo();
    EXPECT_EQ(emitted, 1);
    EXPECT_EQ(doc.text(), QStringLiteral("abc"));
}

TEST(PrimoDocumentTest, UndoDisabledEditsStillApply)
{
    PrimoDocument doc;
    doc.setUndoDisabled(true);
    doc.insertText(0, QStringLiteral("abc"));
    EXPECT_EQ(doc.text(), QStringLiteral("abc"));
    EXPECT_FALSE(doc.canUndo());
    doc.setUndoDisabled(false);
}

TEST(PrimoDocumentTest, InterleavedUndoKeepsConsistentText)
{
    PrimoDocument doc;
    doc.insertText(0, QStringLiteral("ab"));
    doc.insertText(2, QStringLiteral("c"));
    doc.undo();
    doc.insertText(2, QStringLiteral("C"));
    EXPECT_EQ(doc.text(), QStringLiteral("abC"));
    doc.undo();
    EXPECT_EQ(doc.text(), QStringLiteral("ab"));
}

TEST(PrimoDocumentTest, LineAtAfterEditReflectsNewText)
{
    PrimoDocument doc;
    doc.insertText(0, QStringLiteral("alpha"));
    EXPECT_EQ(doc.lineAt(0), QStringLiteral("alpha"));
    doc.insertText(5, QStringLiteral("\nbeta"));
    EXPECT_EQ(doc.lineCount(), 2);
    EXPECT_EQ(doc.lineAt(1), QStringLiteral("beta"));
    doc.undo();
    EXPECT_EQ(doc.lineCount(), 1);
    EXPECT_EQ(doc.lineAt(0), QStringLiteral("alpha"));
}
