#include "plugins/terminalplugin/terminalshortcuts.h"

#include "settingsmanager.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <gtest/gtest.h>

class TerminalShortcutsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_testSettings = SettingsManager::createForTesting();
        SettingsManager::setTestingInstance(m_testSettings.get());
    }

    void TearDown() override
    {
        SettingsManager::setTestingInstance(nullptr);
        m_testSettings.reset();
    }

    static QKeyEvent* keyEvent(int key, Qt::KeyboardModifiers modifiers)
    {
        return new QKeyEvent(QEvent::KeyPress, key, modifiers);
    }

    std::unique_ptr<SettingsManager> m_testSettings;
};

TEST_F(TerminalShortcutsTest, EffectiveShortcutFallsBackToDefault)
{
    const QKeySequence seq = TerminalShortcuts::effectiveShortcut(QStringLiteral("Terminal.Copy"), QKeySequence(QStringLiteral("Ctrl+Shift+C")));
    EXPECT_EQ(seq, QKeySequence(QStringLiteral("Ctrl+Shift+C")));
}

TEST_F(TerminalShortcutsTest, EffectiveShortcutPrefersUserSetting)
{
    SettingsManager::instance().setShortcutForAction(QStringLiteral("Terminal.Copy"), QStringLiteral("Ctrl+Alt+Y"));
    const QKeySequence seq = TerminalShortcuts::effectiveShortcut(QStringLiteral("Terminal.Copy"), QKeySequence(QStringLiteral("Ctrl+Shift+C")));
    EXPECT_EQ(seq, QKeySequence(QStringLiteral("Ctrl+Alt+Y")));
}

TEST_F(TerminalShortcutsTest, MatchesShortcutExactCombination)
{
    const QKeySequence seq(QStringLiteral("Ctrl+Shift+C"));
    const std::unique_ptr<QKeyEvent> match(keyEvent(Qt::Key_C, Qt::ControlModifier | Qt::ShiftModifier));
    const std::unique_ptr<QKeyEvent> wrongMods(keyEvent(Qt::Key_C, Qt::ControlModifier));
    const std::unique_ptr<QKeyEvent> wrongKey(keyEvent(Qt::Key_V, Qt::ControlModifier | Qt::ShiftModifier));
    EXPECT_TRUE(TerminalShortcuts::matchesShortcut(*match, seq));
    EXPECT_FALSE(TerminalShortcuts::matchesShortcut(*wrongMods, seq));
    EXPECT_FALSE(TerminalShortcuts::matchesShortcut(*wrongKey, seq));
}

TEST_F(TerminalShortcutsTest, MatchesShortcutRejectsEmptyAndMultiChord)
{
    const std::unique_ptr<QKeyEvent> event(keyEvent(Qt::Key_C, Qt::ControlModifier | Qt::ShiftModifier));
    EXPECT_FALSE(TerminalShortcuts::matchesShortcut(*event, QKeySequence()));
    EXPECT_FALSE(TerminalShortcuts::matchesShortcut(*event, QKeySequence(QStringLiteral("Ctrl+X, Ctrl+Shift+C"))));
}

TEST_F(TerminalShortcutsTest, AllowsGlobalShortcutDefaults)
{
    const std::unique_ptr<QKeyEvent> toggle(keyEvent(Qt::Key_QuoteLeft, Qt::ControlModifier));
    const std::unique_ptr<QKeyEvent> copy(keyEvent(Qt::Key_C, Qt::ControlModifier | Qt::ShiftModifier));
    const std::unique_ptr<QKeyEvent> paste(keyEvent(Qt::Key_V, Qt::ControlModifier | Qt::ShiftModifier));
    EXPECT_TRUE(TerminalShortcuts::allowsGlobalShortcut(*toggle));
    EXPECT_TRUE(TerminalShortcuts::allowsGlobalShortcut(*copy));
    EXPECT_TRUE(TerminalShortcuts::allowsGlobalShortcut(*paste));
}

TEST_F(TerminalShortcutsTest, AllowsGlobalShortcutRejectsEditorShortcuts)
{
    // Editor-only shortcuts must be claimed by the terminal, not fired app-wide.
    const std::unique_ptr<QKeyEvent> ctrlW(keyEvent(Qt::Key_W, Qt::ControlModifier));
    const std::unique_ptr<QKeyEvent> f2(keyEvent(Qt::Key_F2, Qt::NoModifier));
    const std::unique_ptr<QKeyEvent> ctrlS(keyEvent(Qt::Key_S, Qt::ControlModifier));
    EXPECT_FALSE(TerminalShortcuts::allowsGlobalShortcut(*ctrlW));
    EXPECT_FALSE(TerminalShortcuts::allowsGlobalShortcut(*f2));
    EXPECT_FALSE(TerminalShortcuts::allowsGlobalShortcut(*ctrlS));
}

TEST_F(TerminalShortcutsTest, AllowsGlobalShortcutRespectsRemappedShortcuts)
{
    // When the user remaps terminal copy/paste, the new combos stay global.
    SettingsManager::instance().setShortcutForAction(QStringLiteral("Terminal.Copy"), QStringLiteral("Ctrl+Alt+Y"));
    const std::unique_ptr<QKeyEvent> remapped(keyEvent(Qt::Key_Y, Qt::ControlModifier | Qt::AltModifier));
    const std::unique_ptr<QKeyEvent> oldDefault(keyEvent(Qt::Key_C, Qt::ControlModifier | Qt::ShiftModifier));
    EXPECT_TRUE(TerminalShortcuts::allowsGlobalShortcut(*remapped));
    EXPECT_FALSE(TerminalShortcuts::allowsGlobalShortcut(*oldDefault));
}
