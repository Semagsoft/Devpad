/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tui/tuicommand.h"

#include <gtest/gtest.h>

#include "tui/tuihighlighter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

// Minimal helper to run a command and capture the status message.
static TuiCommand::Result runCmd(const QString& cmd, TuiTabModel& tabs, TuiBuffer* cur,
                                 TuiFileTree& fileTree, TuiViewState& view, QString& statusMsg,
                                 SearchResult& lastSearch, SearchOptions& findOpts,
                                 QHash<QString, QDateTime>& fileMtimes, bool& saveAsReq)
{
    return TuiCommand::dispatch(cmd, tabs, cur, fileTree, view, statusMsg, lastSearch,
                                findOpts, fileMtimes, saveAsReq);
}

TEST(TuiCommand, QuitNoModified)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/a.txt"), QStringLiteral("hello")));
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    auto res = runCmd(QStringLiteral("q"), tabs, cur, fileTree, view, statusMsg, lastSearch,
                      findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Quit);
}

TEST(TuiCommand, QuitBlockedByModified)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/a.txt"), QStringLiteral("hello")));
    tabs.currentBuffer()->setModified(true);
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    auto res = runCmd(QStringLiteral("q"), tabs, cur, fileTree, view, statusMsg, lastSearch,
                      findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_EQ(statusMsg, QStringLiteral("Modified buffers - use :wq or :q! to force"));
}

TEST(TuiCommand, QuitForce)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/a.txt"), QStringLiteral("hello")));
    tabs.currentBuffer()->setModified(true);
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    auto res = runCmd(QStringLiteral("q!"), tabs, cur, fileTree, view, statusMsg, lastSearch,
                      findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Quit);
}

TEST(TuiCommand, SetWrap)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer());
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    // Turn on
    auto res = runCmd(QStringLiteral("set wrap on"), tabs, cur, fileTree, view, statusMsg, lastSearch,
                      findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_TRUE(view.wordWrap);
    EXPECT_EQ(statusMsg, QStringLiteral("Wrap on"));

    // Turn off
    res = runCmd(QStringLiteral("set wrap off"), tabs, cur, fileTree, view, statusMsg, lastSearch,
                 findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_FALSE(view.wordWrap);
    EXPECT_EQ(statusMsg, QStringLiteral("Wrap off"));

    // Query
    res = runCmd(QStringLiteral("set wrap"), tabs, cur, fileTree, view, statusMsg, lastSearch,
                 findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_EQ(statusMsg, QStringLiteral("Wrap is off"));
}

TEST(TuiCommand, SetNumber)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer());
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    EXPECT_TRUE(view.showLineNumbers);

    runCmd(QStringLiteral("set nonumber"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_FALSE(view.showLineNumbers);
    EXPECT_EQ(statusMsg, QStringLiteral("Line numbers off"));

    runCmd(QStringLiteral("set number"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_TRUE(view.showLineNumbers);
    EXPECT_EQ(statusMsg, QStringLiteral("Line numbers on"));
}

TEST(TuiCommand, SyntaxOnOff)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer());
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    runCmd(QStringLiteral("syntax off"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_FALSE(view.syntaxEnabled);
    EXPECT_FALSE(TuiHighlighter::isSyntaxEnabled());
    EXPECT_EQ(statusMsg, QStringLiteral("Syntax off"));

    runCmd(QStringLiteral("syntax on"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_TRUE(view.syntaxEnabled);
    EXPECT_TRUE(TuiHighlighter::isSyntaxEnabled());
    EXPECT_EQ(statusMsg, QStringLiteral("Syntax on"));
}

TEST(TuiCommand, SetSyntaxLanguage)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer());
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    // Reset any previous language override
    TuiHighlighter::setLanguageOverride(QString());

    runCmd(QStringLiteral("set syntax cpp"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(TuiHighlighter::currentLanguage(QString()), QStringLiteral("cpp"));
    EXPECT_EQ(statusMsg, QStringLiteral("Syntax language set to cpp"));

    runCmd(QStringLiteral("set ft python"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(TuiHighlighter::currentLanguage(QString()), QStringLiteral("python"));

    runCmd(QStringLiteral("syntax off"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(TuiHighlighter::currentLanguage(QString()), QString());
    EXPECT_EQ(statusMsg, QStringLiteral("Syntax off"));
}

TEST(TuiCommand, TabsList)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/a.txt"), QStringLiteral("hello")));
    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/b.txt"), QStringLiteral("world")));
    tabs.currentBuffer()->setModified(true);
    tabs.setCurrentIndex(1);
    TuiBuffer* cur = tabs.currentBuffer();

    auto res = runCmd(QStringLiteral("tabs"), tabs, cur, fileTree, view, statusMsg, lastSearch,
                      findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_TRUE(statusMsg.contains(QStringLiteral("a.txt")));
    EXPECT_TRUE(statusMsg.contains(QStringLiteral("b.txt")));
    EXPECT_TRUE(statusMsg.contains(QStringLiteral(">")));   // current tab marker
    EXPECT_TRUE(statusMsg.contains(QStringLiteral("+")));   // modified marker
}

TEST(TuiCommand, TabNavigation)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/a.txt"), QStringLiteral("a")));
    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/b.txt"), QStringLiteral("b")));
    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/c.txt"), QStringLiteral("c")));
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    runCmd(QStringLiteral("tabnext"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 1);

    runCmd(QStringLiteral("tabnext"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 2);

    // Wraps around
    runCmd(QStringLiteral("tabnext"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 0);

    runCmd(QStringLiteral("tabprev"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 2);

    runCmd(QStringLiteral("tabfirst"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 0);

    runCmd(QStringLiteral("tablast"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 2);
}

TEST(TuiCommand, TabClose)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/a.txt"), QStringLiteral("a")));
    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/b.txt"), QStringLiteral("b")));
    TuiBuffer* cur = tabs.currentBuffer();

    EXPECT_EQ(tabs.count(), 2);

    runCmd(QStringLiteral("tabclose"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.count(), 1);
    EXPECT_EQ(statusMsg, QStringLiteral("Tab closed"));

    // Closing last tab creates an empty buffer
    runCmd(QStringLiteral("tabclose"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.count(), 1);
}

TEST(TuiCommand, Bookmarks)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QStringLiteral("line1\nline2\nline3")));
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    cur->setCursor(1, 0);
    runCmd(QStringLiteral("bn"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(statusMsg, QStringLiteral("No bookmarks"));

    cur->toggleBookmark(1);
    runCmd(QStringLiteral("bn"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(statusMsg, QStringLiteral("Next bookmark 2"));

    runCmd(QStringLiteral("bp"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(statusMsg, QStringLiteral("Prev bookmark 2"));

    runCmd(QStringLiteral("bc"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(statusMsg, QStringLiteral("Bookmarks cleared"));
    EXPECT_FALSE(cur->hasBookmark(1));
}

TEST(TuiCommand, Replace)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QString(), QStringLiteral("foo bar foo")));
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    runCmd(QStringLiteral("s/foo/baz/"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(statusMsg, QStringLiteral("Replaced at 1:1"));
    EXPECT_EQ(cur->text(), QStringLiteral("baz bar foo"));

    runCmd(QStringLiteral("%s/foo/qux/g"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(statusMsg, QStringLiteral("Replaced 1 occurrences"));
    EXPECT_EQ(cur->text(), QStringLiteral("baz bar qux"));
}

TEST(TuiCommand, HelpAndVersion)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer());
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    runCmd(QStringLiteral("help"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_TRUE(statusMsg.contains(QStringLiteral(":w")));
    EXPECT_TRUE(statusMsg.contains(QStringLiteral(":q")));

    runCmd(QStringLiteral("version"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_FALSE(statusMsg.isEmpty());
}

TEST(TuiCommand, UnknownCommand)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer());
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    runCmd(QStringLiteral("frobnicate"), tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(statusMsg, QStringLiteral("Unknown command: frobnicate"));
}

TEST(TuiCommand, CdCommand)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer());
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    QTemporaryDir tmpDir;
    QString dirPath = tmpDir.path();

    runCmd(QStringLiteral("cd ") + dirPath, tabs, cur, fileTree, view, statusMsg, lastSearch,
           findOpts, fileMtimes, saveAsReq);
    EXPECT_TRUE(statusMsg.contains(QStringLiteral("Project:")));
    EXPECT_TRUE(fileTree.hasRoot());
}
