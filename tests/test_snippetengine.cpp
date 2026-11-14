#include "codeeditor.h"
#include "settingsmanager.h"
#include "snippet.h"
#include "snippetengine.h"

#include <QApplication>
#include <QCoreApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>

namespace
{

Snippet makeSnippet(const QString& prefix, const QStringList& body)
{
    Snippet snippet;
    snippet.name = prefix;
    snippet.prefix = prefix;
    snippet.body = body;
    snippet.scope = {"cpp"};
    return snippet;
}

} // namespace

class SnippetEngineTest : public ::testing::Test
{
protected:
    std::unique_ptr<SettingsManager> m_testSettings;
    CodeEditor* editor = nullptr;
    SnippetEngine* engine = nullptr;

    void SetUp() override
    {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
        editor = new CodeEditor();
        engine = new SnippetEngine(editor);
    }
    void TearDown() override
    {
        delete editor;
        editor = nullptr;
        engine = nullptr;
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }

    static void setCursor(CodeEditor* ed, int line, int col)
    {
        ed->setCursorPosition(line, col);
    }

    // The mirror text sync is deferred out of the SCN_MODIFIED notification
    // (Scintilla drops nested edits from inside it), so editor mutations are
    // only reconciled with the mirror stops once the event loop runs.
    static void pumpEvents()
    {
        for (int i = 0; i < 4; ++i)
        {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
            QCoreApplication::processEvents();
        }
    }
};

TEST_F(SnippetEngineTest, NotActiveInitially)
{
    EXPECT_FALSE(engine->isActive());
}

TEST_F(SnippetEngineTest, SnippetWithoutTabStopsDoesNotEnterSnippetMode)
{
    // A body with no tab stops has nowhere to navigate, so the engine
    // performs the insertion and leaves snippet mode immediately.
    Snippet snippet = makeSnippet("hdr", {"// header", "// body"});
    engine->insertSnippet(snippet);

    EXPECT_FALSE(engine->isActive());
    EXPECT_TRUE(editor->text().contains("// header\n// body"));
}

TEST_F(SnippetEngineTest, InsertOnReadOnlyEditorDoesNothing)
{
    editor->setReadOnlyMode(true);
    Snippet snippet = makeSnippet("hdr", {"// header"});
    engine->insertSnippet(snippet);

    EXPECT_FALSE(engine->isActive());
    EXPECT_FALSE(editor->text().contains("// header"));
}

TEST_F(SnippetEngineTest, EscapeExitsSnippetMode)
{
    Snippet snippet = makeSnippet("hdr", {"a${1:one}b"});
    engine->insertSnippet(snippet);
    EXPECT_TRUE(engine->isActive());

    engine->handleKeyPress(Qt::Key_Escape, Qt::NoModifier);
    EXPECT_FALSE(engine->isActive());
}

TEST_F(SnippetEngineTest, ExitSnippetModeWhenNotActiveIsNoop)
{
    engine->exitSnippetMode();
    EXPECT_FALSE(engine->isActive());
}

TEST_F(SnippetEngineTest, AdvanceTabStopMovesToNextStopAndEventuallyExits)
{
    Snippet snippet = makeSnippet("two", {"${1:first} x ${2:second}"});
    engine->insertSnippet(snippet);
    EXPECT_TRUE(engine->isActive());

    engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier);
    EXPECT_TRUE(engine->isActive());

    // Past the final tab stop the snippet mode exits.
    engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier);
    EXPECT_FALSE(engine->isActive());
}

TEST_F(SnippetEngineTest, MirrorTabStopsAreNavigatedTogether)
{
    // "${1:name}" appears twice; advance from stop 1 goes to stop 2 (if present) or exits.
    Snippet snippet = makeSnippet("mirror", {"${1:name}${2:more}${1:name}"});
    engine->insertSnippet(snippet);
    EXPECT_TRUE(engine->isActive());

    int selLineFrom, selColFrom, selLineTo, selColTo;
    editor->getSelection(&selLineFrom, &selColFrom, &selLineTo, &selColTo);
    EXPECT_EQ(selLineFrom, 0);
    EXPECT_EQ(selColFrom, 0);

    // Advance to the mirror occurrence of tab stop 1 (over tab stop 2).
    engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier);
    EXPECT_TRUE(engine->isActive());
}

TEST_F(SnippetEngineTest, HandleKeyPassesThroughWhenInactive)
{
    EXPECT_FALSE(engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier));
    EXPECT_FALSE(engine->handleKeyPress(Qt::Key_Escape, Qt::NoModifier));
    EXPECT_FALSE(engine->handleKeyPress(Qt::Key_A, Qt::NoModifier));
}

TEST_F(SnippetEngineTest, ShiftTabOnFirstStopStaysActive)
{
    Snippet snippet = makeSnippet("two", {"${1:first} ${2:second}"});
    engine->insertSnippet(snippet);

    // Retreat from the first stop should not leave snippet mode nor crash.
    engine->handleKeyPress(Qt::Key_Tab, Qt::ShiftModifier);
    EXPECT_TRUE(engine->isActive());
}

TEST_F(SnippetEngineTest, SnippetModeChangedSignalEmitted)
{
    QSignalSpy spy(engine, &SnippetEngine::snippetModeChanged);
    Snippet snippet = makeSnippet("hdr", {"a${1:one}b"});
    engine->insertSnippet(snippet);
    engine->handleKeyPress(Qt::Key_Escape, Qt::NoModifier);

    ASSERT_EQ(spy.size(), 2);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
    EXPECT_FALSE(spy.at(1).at(0).toBool());
}

TEST_F(SnippetEngineTest, IndicatorConstantMatchesDocumentation)
{
    EXPECT_EQ(SnippetEngine::SNIPPET_INDICATOR, 21);
}

TEST_F(SnippetEngineTest, MirrorTextSyncsOnEdit)
{
    Snippet snippet = makeSnippet("mirror", {"${1:name}${1}${2:more}"});
    engine->insertSnippet(snippet);
    ASSERT_TRUE(engine->isActive());

    // The first mirror is selected; replace its content.
    editor->replaceSelectedText(QStringLiteral("foo"));
    pumpEvents();
    EXPECT_EQ(editor->text(), QStringLiteral("foofoomore")); // both ${1} mirrors now "foo"
    ASSERT_TRUE(engine->isActive());
}

TEST_F(SnippetEngineTest, MirrorTextSyncsOnDeleteToEmpty)
{
    Snippet snippet = makeSnippet("mirror", {"${1:abc}${1}"});
    engine->insertSnippet(snippet);
    ASSERT_TRUE(engine->isActive());

    editor->replaceSelectedText(QString()); // delete the content of stop 1
    pumpEvents();
    EXPECT_EQ(editor->text(), QString());
}

TEST_F(SnippetEngineTest, MirrorSyncKeepsOtherStopPositionsCorrect)
{
    // After a mirror sync shrinks stop 1, the non-mirrored stop 2 must be
    // tracked to its new location for navigation purposes.
    Snippet snippet = makeSnippet("mirror", {"${1:name}${1}${2:more}"});
    engine->insertSnippet(snippet);

    editor->replaceSelectedText(QStringLiteral("foo")); // "foofoomore"
    pumpEvents();
    ASSERT_EQ(editor->text(), QStringLiteral("foofoomore"));

    engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier); // -> stop 2
    int l, c, l2, c2;
    editor->getSelection(&l, &c, &l2, &c2);
    EXPECT_EQ(c, 6);   // "more" now starts at col 6 in "foofoomore"
    EXPECT_EQ(c2, 10);
    int start = editor->positionFromLineIndex(l, c);
    int end = editor->positionFromLineIndex(l2, c2);
    EXPECT_EQ(editor->text(start, end), QStringLiteral("more"));
}

TEST_F(SnippetEngineTest, MirrorEditsAreNotPropagatedFromOtherStops)
{
    Snippet snippet = makeSnippet("mirror", {"${1:one} ${2:two}"});
    engine->insertSnippet(snippet);

    // Advance to stop 2 and rewrite it; stop 1 must remain unchanged.
    engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier);
    editor->replaceSelectedText(QStringLiteral("TWO"));
    pumpEvents();
    EXPECT_EQ(editor->text(), QStringLiteral("one TWO"));
}

TEST_F(SnippetEngineTest, NumberedStopsVisitInAscendingOrderNotPosition)
{
    // ${2} appears before ${1} in the text; visit ${1} first anyway.
    Snippet snippet = makeSnippet("order", {"${2:two} ${1:one}"});
    engine->insertSnippet(snippet);

    int l, c, l2, c2;
    editor->getSelection(&l, &c, &l2, &c2);
    EXPECT_EQ(l, 0);
    EXPECT_EQ(c, 4);   // ${1} "one" spans columns 4..7
    EXPECT_EQ(c2, 7);

    engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier); // -> ${2}
    editor->getSelection(&l, &c, &l2, &c2);
    EXPECT_EQ(c, 0);   // ${2} "two" spans columns 0..3
    EXPECT_EQ(c2, 3);

    engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier); // -> exit (past ${2})
    EXPECT_FALSE(engine->isActive());
}

TEST_F(SnippetEngineTest, DollarZeroVisitedLastAndExitsAfter)
{
    Snippet snippet = makeSnippet("order", {"a${1:one}b${0:end}c"});
    engine->insertSnippet(snippet);

    int l, c, l2, c2;
    editor->getSelection(&l, &c, &l2, &c2);
    EXPECT_EQ(c, 1); // inside "one"
    EXPECT_EQ(c2, 4);

    engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier); // -> ${0}
    editor->getSelection(&l, &c, &l2, &c2);
    EXPECT_EQ(c, 5); // start of "end" (a one b -> "end" at col 5)
    EXPECT_EQ(c2, 8);

    engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier); // -> exit
    EXPECT_FALSE(engine->isActive());
}

TEST_F(SnippetEngineTest, TabNavigatesPastMirrorGroupAsSingleField)
{
    // ${1}${1} ${2:two}: two mirrors of 1, then 2.
    Snippet snippet = makeSnippet("group", {"${1:a}${1} ${2:two}"});
    engine->insertSnippet(snippet);
    ASSERT_TRUE(engine->isActive());

    engine->handleKeyPress(Qt::Key_Tab, Qt::NoModifier); // -> stop 2
    int l, c, l2, c2;
    editor->getSelection(&l, &c, &l2, &c2);
    EXPECT_EQ(c, 3); // start of "two"
}
