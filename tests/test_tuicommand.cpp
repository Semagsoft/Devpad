/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tui/tuicommand.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <gtest/gtest.h>

// Helper to run a command through TuiCommand::dispatch and capture results.
static TuiCommand::Result runCmd(const QString& cmd, TuiTabModel& tabs, TuiBuffer* cur, TuiFileTree& fileTree, TuiViewState& view, QString& statusMsg,
                                 SearchResult& lastSearch, SearchOptions& findOpts, QHash<QString, QDateTime>& fileMtimes, bool& saveAsReq)
{
    return TuiCommand::dispatch(cmd, tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
}

// ── Quit commands ──────────────────────────────────────────────────────

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

    auto res = runCmd(QStringLiteral("q"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
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

    auto res = runCmd(QStringLiteral("q"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
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

    auto res = runCmd(QStringLiteral("q!"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Quit);
}

// ── Save commands ──────────────────────────────────────────────────────

TEST(TuiCommand, SaveWithPath)
{
    QTemporaryDir tmpDir;
    QString filePath = tmpDir.path() + QStringLiteral("/test.txt");

    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(filePath, QStringLiteral("test content")));
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    auto res = runCmd(QStringLiteral("w"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_FALSE(cur->isModified());
    EXPECT_TRUE(QFile::exists(filePath));
    QFile::remove(filePath);
}

TEST(TuiCommand, SaveAsNewPath)
{
    QTemporaryDir tmpDir;
    QString filePath = tmpDir.path() + QStringLiteral("/newfile.txt");

    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QStringLiteral("/tmp/untitled"), QStringLiteral("new content")));
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    auto res = runCmd(QStringLiteral("w ") + filePath, tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_EQ(cur->filePath(), filePath);
    EXPECT_FALSE(cur->isModified());
    EXPECT_TRUE(QFile::exists(filePath));
    QFile::remove(filePath);
}

// ── Open commands ──────────────────────────────────────────────────────

TEST(TuiCommand, OpenExistingFile)
{
    QTemporaryDir tmpDir;
    QString filePath = tmpDir.path() + QStringLiteral("/existing.txt");
    QFile file(filePath);
    file.open(QIODevice::WriteOnly);
    file.write("existing content");
    file.close();

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

    auto res = runCmd(QStringLiteral("e ") + filePath, tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_EQ(tabs.count(), 2);
    EXPECT_EQ(statusMsg, QStringLiteral("Opened ") + filePath);
    QFile::remove(filePath);
}

// ── Quit and save ──────────────────────────────────────────────────────

TEST(TuiCommand, WqSavesAndQuits)
{
    QTemporaryDir tmpDir;
    QString filePath = tmpDir.path() + QStringLiteral("/wqtest.txt");

    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(filePath, QStringLiteral("wq content")));
    tabs.currentBuffer()->setModified(true);
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    auto res = runCmd(QStringLiteral("wq"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Quit);
    EXPECT_FALSE(cur->isModified());
    EXPECT_TRUE(QFile::exists(filePath));
    QFile::remove(filePath);
}

// ── Wrap commands ──────────────────────────────────────────────────────

TEST(TuiCommand, SetWrapOn)
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

    EXPECT_FALSE(view.wordWrap);

    auto res = runCmd(QStringLiteral("set wrap on"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_TRUE(view.wordWrap);
    EXPECT_EQ(statusMsg, QStringLiteral("Wrap on"));
}

TEST(TuiCommand, SetWrapOff)
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
    view.wordWrap = true;

    auto res = runCmd(QStringLiteral("set wrap off"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_FALSE(view.wordWrap);
    EXPECT_EQ(statusMsg, QStringLiteral("Wrap off"));
}

// ── Line number commands ───────────────────────────────────────────────

TEST(TuiCommand, SetNumberOn)
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

    // showLineNumbers defaults to true; turn it off first
    view.showLineNumbers = false;

    auto res = runCmd(QStringLiteral("set number"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_TRUE(view.showLineNumbers);
    EXPECT_EQ(statusMsg, QStringLiteral("Line numbers on"));
}

TEST(TuiCommand, SetNonumber)
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
    view.showLineNumbers = true;

    auto res = runCmd(QStringLiteral("set nonumber"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_FALSE(view.showLineNumbers);
    EXPECT_EQ(statusMsg, QStringLiteral("Line numbers off"));
}

// ── Syntax commands ────────────────────────────────────────────────────

TEST(TuiCommand, SyntaxOn)
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

    auto res = runCmd(QStringLiteral("syntax on"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_TRUE(view.syntaxEnabled);
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

    auto res = runCmd(QStringLiteral("set syntax python"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_EQ(statusMsg, QStringLiteral("Syntax language set to python"));
}

// ── Replace commands ───────────────────────────────────────────────────

TEST(TuiCommand, ReplaceSingle)
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

    auto res = runCmd(QStringLiteral("s/foo/baz/"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_EQ(statusMsg, QStringLiteral("Replaced at 1:1"));
    EXPECT_EQ(cur->text(), QStringLiteral("baz bar foo"));
}

TEST(TuiCommand, ReplaceAll)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QString(), QStringLiteral("foo bar foo foo")));
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    auto res = runCmd(QStringLiteral("%s/foo/qux/g"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_EQ(statusMsg, QStringLiteral("Replaced 3 occurrences"));
    EXPECT_EQ(cur->text(), QStringLiteral("qux bar qux qux"));
}

// ── Bookmark commands ──────────────────────────────────────────────────

TEST(TuiCommand, BookmarkNextPrev)
{
    TuiTabModel tabs;
    TuiFileTree fileTree;
    TuiViewState view;
    QString statusMsg;
    SearchResult lastSearch;
    SearchOptions findOpts;
    QHash<QString, QDateTime> fileMtimes;
    bool saveAsReq = false;

    tabs.addBuffer(TuiBuffer(QString(), QStringLiteral("line1\nline2\nline3\nline4\nline5")));
    tabs.setCurrentIndex(0);
    TuiBuffer* cur = tabs.currentBuffer();

    cur->toggleBookmark(1);
    cur->toggleBookmark(3);

    // Start at line 3, go to prev bookmark (line 1)
    cur->setCursor(3, 0);
    auto res = runCmd(QStringLiteral("bp"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(statusMsg, QStringLiteral("Prev bookmark 2"));
    EXPECT_EQ(cur->cursorLine(), 1);

    // From line 1, go to next bookmark (line 3)
    res = runCmd(QStringLiteral("bn"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(statusMsg, QStringLiteral("Next bookmark 4"));
    EXPECT_EQ(cur->cursorLine(), 3);

    runCmd(QStringLiteral("bc"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(statusMsg, QStringLiteral("Bookmarks cleared"));
    EXPECT_FALSE(cur->hasBookmark(1));
    EXPECT_FALSE(cur->hasBookmark(3));
}

// ── Tab navigation commands ────────────────────────────────────────────

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

    runCmd(QStringLiteral("tabnext"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 1);

    runCmd(QStringLiteral("tabnext"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 2);

    // Wrap around
    runCmd(QStringLiteral("tabnext"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 0);

    runCmd(QStringLiteral("tabprev"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 2);

    runCmd(QStringLiteral("tabfirst"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 0);

    runCmd(QStringLiteral("tablast"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(tabs.currentIndex(), 2);
}

// ── Unknown command ────────────────────────────────────────────────────

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

    auto res = runCmd(QStringLiteral("frobnicate"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_EQ(res, TuiCommand::Result::Continue);
    EXPECT_EQ(statusMsg, QStringLiteral("Unknown command: frobnicate"));
}

// ── Help and version ───────────────────────────────────────────────────

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

    runCmd(QStringLiteral("help"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_TRUE(statusMsg.contains(QStringLiteral(":w")));
    EXPECT_TRUE(statusMsg.contains(QStringLiteral(":q")));

    runCmd(QStringLiteral("version"), tabs, cur, fileTree, view, statusMsg, lastSearch, findOpts, fileMtimes, saveAsReq);
    EXPECT_TRUE(statusMsg.contains(QStringLiteral("Devpad")));
}

// ── Tab completion ─────────────────────────────────────────────────────

TEST(TuiCommand, CompleteSingleMatch)
{
    QString input = QStringLiteral("wqa");
    QString statusMsg;
    TuiCommand::complete(input, statusMsg);
    EXPECT_EQ(input, QStringLiteral("wqa "));
    EXPECT_TRUE(statusMsg.isEmpty());
}

TEST(TuiCommand, CompleteMultipleMatches)
{
    QString input = QStringLiteral("set s");
    QString statusMsg;
    TuiCommand::complete(input, statusMsg);
    EXPECT_TRUE(input.startsWith(QStringLiteral("set ")));
    EXPECT_TRUE(statusMsg.startsWith(QStringLiteral("Candidates:")));
}
