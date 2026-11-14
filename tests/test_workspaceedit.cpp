#include "lsp/lspworkspaceedit.h"

#include <QFile>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTextStream>

#include <gtest/gtest.h>

class LspWorkspaceEditTest : public ::testing::Test
{
protected:
    static QJsonObject makeEdit(int startLine, int startCol, int endLine, int endCol, const QString& newText)
    {
        QJsonObject range;
        range["start"] = QJsonObject{{"line", startLine}, {"character", startCol}};
        range["end"] = QJsonObject{{"line", endLine}, {"character", endCol}};
        QJsonObject edit{{"range", range}, {"newText", newText}};
        return edit;
    }
};

TEST_F(LspWorkspaceEditTest, SingleLineReplace)
{
    QString text = QStringLiteral("hello world\n");
    QList<QJsonObject> edits{makeEdit(0, 6, 0, 11, QStringLiteral("there"))};
    bool ok = false;
    const QString result = lsp::applyTextEditsToText(text, edits, &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(result, "hello there\n");
}

TEST_F(LspWorkspaceEditTest, MultipleEditsApplyBottomMostFirst)
{
    // Ranges refer to the original document; applying the later one first
    // keeps both ranges valid.
    QString text = QStringLiteral("aaa bbb ccc\n");
    QList<QJsonObject> edits{makeEdit(0, 0, 0, 3, QStringLiteral("AAA")), makeEdit(0, 8, 0, 11, QStringLiteral("CCC"))};
    bool ok = false;
    const QString result = lsp::applyTextEditsToText(text, edits, &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(result, "AAA bbb CCC\n");
}

TEST_F(LspWorkspaceEditTest, InsertAtBothEndsWithNewlines)
{
    QString text = QStringLiteral("one\ntwo\nthree\n");
    QList<QJsonObject> edits{makeEdit(0, 0, 0, 0, QStringLiteral("START\n")), makeEdit(2, 5, 2, 5, QStringLiteral("\nEND"))};
    bool ok = false;
    const QString result = lsp::applyTextEditsToText(text, edits, &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(result, "START\none\ntwo\nthree\nEND\n");
}

TEST_F(LspWorkspaceEditTest, NonAsciiContentBeforeEdit)
{
    // "naïve": n a ï v e — ï is 2 UTF-8 bytes, 1 UTF-16 unit.
    QString text = QStringLiteral("naïve x\n");
    // Replace the trailing "x" at UTF-16 col 6.
    QList<QJsonObject> edits{makeEdit(0, 6, 0, 7, QStringLiteral("y"))};
    bool ok = false;
    const QString result = lsp::applyTextEditsToText(text, edits, &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(result, QStringLiteral("naïve y\n"));
}

TEST_F(LspWorkspaceEditTest, AstralCharacterCountsAsTwoColumns)
{
    // "😀x": 😀 is 4 UTF-8 bytes, 2 UTF-16 units; x is col 2.
    QString text = QStringLiteral("😀x\n");
    QList<QJsonObject> edits{makeEdit(0, 2, 0, 3, QStringLiteral("y"))};
    bool ok = false;
    const QString result = lsp::applyTextEditsToText(text, edits, &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(result, QStringLiteral("😀y\n"));
}

TEST_F(LspWorkspaceEditTest, OutOfDocumentEditFails)
{
    QString text = QStringLiteral("short\n");
    QList<QJsonObject> edits{makeEdit(5, 0, 5, 1, QStringLiteral("x"))};
    bool ok = false;
    QString error;
    const QString result = lsp::applyTextEditsToText(text, edits, &ok, &error);
    EXPECT_FALSE(ok);
    EXPECT_EQ(result, text);
    EXPECT_FALSE(error.isEmpty());
}

TEST_F(LspWorkspaceEditTest, ReplacesWholeDocument)
{
    QString text = QStringLiteral("everything goes\n");
    QList<QJsonObject> edits{makeEdit(0, 0, 0, 15, QStringLiteral("new"))};
    bool ok = false;
    const QString result = lsp::applyTextEditsToText(text, edits, &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(result, "new\n");
}

TEST_F(LspWorkspaceEditTest, ApplyTextEditsToFileWritesAtomically)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString filePath = dir.filePath("edit_me.txt");
    {
        QFile f(filePath);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly | QIODevice::Text));
        f.write("foo bar\n");
    }

    QString error;
    ASSERT_TRUE(lsp::applyTextEditsToFile(filePath, {makeEdit(0, 4, 0, 7, QStringLiteral("baz"))}, &error)) << error.toStdString();

    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::ReadOnly | QIODevice::Text));
    QTextStream in(&f);
    EXPECT_EQ(in.readAll(), QStringLiteral("foo baz\n"));
}
