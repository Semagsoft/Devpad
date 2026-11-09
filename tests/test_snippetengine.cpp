#include "codeeditor.h"
#include "settingsmanager.h"
#include "snippet.h"
#include "snippetengine.h"

#include <QApplication>
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
