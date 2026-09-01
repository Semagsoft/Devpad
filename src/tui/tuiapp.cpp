/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuiapp.h"

#include "core/fileservice.h"
#include "managers/settingsmanager.h"
#include "tui/tuibuffer.h"
#include "tui/tuicommand.h"
#include "tui/tuifiletree.h"
#include "tui/tuihighlighter.h"
#include "tui/tuiinput.h"
#include "tui/tuirenderer.h"
#include "tui/tuisession.h"
#include "tui/tuitabmodel.h"

#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTextStream>

#include <unistd.h>

#ifdef BUILD_TUI
#include <ncurses.h>
#endif

static void showHelpFallback(const QStringList& files)
{
    Q_UNUSED(files);
    QTextStream out(stdout);
    out << "Devpad TUI mode - terminal editor\n";
    out << "Usage: Devpad --tui [files...]\n";
}

int runTuiApp(QCoreApplication* app, const QCommandLineParser& parser, const QStringList& positional)
{
    return TuiApp::run(app, parser, positional);
}

int TuiApp::run(QCoreApplication* app, const QCommandLineParser& parser, const QStringList& positionalFiles)
{
    Q_UNUSED(app);
    Q_UNUSED(parser);

#ifndef BUILD_TUI
    QTextStream out(stdout);
    out << "TUI mode not built. Reconfigure with -DBUILD_TUI=ON\n";
    showHelpFallback(positionalFiles);
    return 1;
#else
    TuiTabModel tabs;
    TuiFileTree fileTree;

    for (const QString& arg : positionalFiles)
    {
        QString filePath = arg;
        if (!QFileInfo(filePath).isAbsolute())
            filePath = QDir::current().absoluteFilePath(filePath);
        QFileInfo fi(filePath);
        if (fi.isFile())
        {
            FileLoadResult res = FileService::load(filePath);
            if (res.ok)
            {
                TuiBuffer buf(filePath, res.text, res.encoding);
                tabs.addBuffer(buf);
            }
            else
            {
                TuiBuffer buf(filePath, QString(), QStringLiteral("UTF-8"));
                tabs.addBuffer(buf);
            }
        }
        else if (fi.isDir())
        {
            if (!fileTree.hasRoot())
                fileTree.setRootPath(filePath);
            QDir dir(filePath);
            QStringList entries = dir.entryList(QDir::Files, QDir::Name);
            for (const QString& e : entries)
            {
                QString fp = dir.absoluteFilePath(e);
                FileLoadResult res = FileService::load(fp);
                if (res.ok)
                {
                    TuiBuffer buf(fp, res.text, res.encoding);
                    tabs.addBuffer(buf);
                    if (tabs.count() >= 20)
                        break;
                }
            }
        }
        else
        {
            TuiBuffer buf(filePath, QString(), QStringLiteral("UTF-8"));
            tabs.addBuffer(buf);
        }
    }

    if (tabs.isEmpty())
        tabs.addBuffer(TuiBuffer(QString(), QString(), QStringLiteral("UTF-8")));

    TuiSession::tryRestore(parser, positionalFiles, tabs, fileTree);

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO))
    {
        QTextStream out(stdout);
        for (int i = 0; i < tabs.count(); ++i)
        {
            const TuiBuffer* b = tabs.bufferAt(i);
            if (!b)
                continue;
            out << "=== " << (b->filePath().isEmpty() ? QStringLiteral("(untitled)") : b->filePath()) << " ===\n";
            out << b->text() << "\n";
        }
        return 0;
    }

    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);

    bool hasColors = has_colors();
    if (hasColors)
    {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_BLACK, COLOR_CYAN);
        init_pair(2, COLOR_YELLOW, -1);
        init_pair(3, COLOR_CYAN, -1);
        init_pair(4, COLOR_BLUE, -1);
        init_pair(5, COLOR_RED, -1);
        init_pair(6, COLOR_GREEN, -1);
        init_pair(7, COLOR_MAGENTA, -1);
        init_pair(8, COLOR_CYAN, -1);
    }

    TuiInputState st;
    st.view.wordWrap = SettingsManager::instance().wordWrap();
    st.view.syntaxEnabled = true;
    TuiHighlighter::setEnabled(true);
    st.fileTreeVisible = fileTree.hasRoot();
    st.fileTreeFocused = false;

    for (int i = 0; i < tabs.count(); ++i)
    {
        const TuiBuffer* b = tabs.bufferAt(i);
        if (b && !b->filePath().isEmpty())
            st.fileMtimes[b->filePath()] = QFileInfo(b->filePath()).lastModified();
    }

    qint64 lastTreePollMs = 0;
    QDateTime lastTreeDirMTime;

    int ch = 0;
    while (true)
    {
        TuiBuffer* cur = tabs.currentBuffer();
        if (!cur)
        {
            tabs.addBuffer(TuiBuffer());
            cur = tabs.currentBuffer();
        }

        TuiSession::pollExternalChanges(tabs, cur, st.fileMtimes, st.statusMsg);
        TuiSession::pollFileTree(fileTree, lastTreePollMs, lastTreeDirMTime, st.statusMsg);

        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        const int tabBarH = 1;
        const int statusBarH = 2;
        int editorH = rows - tabBarH - statusBarH;
        if (editorH < 1)
            editorH = 1;
        const int treeWidth = 30;

        TuiRenderer::drawTabBar(tabs, cols);
        TuiRenderer::drawEditor(cur, tabs, fileTree, st.view, editorH, cols, tabBarH, treeWidth, st.fileTreeVisible, st.fileTreeFocused, hasColors,
                                st.lastSearch);
        TuiRenderer::drawStatusBar(cur, tabs, st.statusMsg, fileTree, st.fileTreeVisible, st.fileTreeFocused, st.findMode, st.findInput,
                                   st.replaceMode, st.replacePhase, st.replaceFindInput, st.replaceInput, st.fileTreeFilterMode,
                                   st.fileTreeFilterInput, st.saveAsMode, st.saveAsInput, st.gotoMode, st.gotoInput, st.commandMode, st.commandInput,
                                   cols, rows, hasColors);

        refresh();
        ch = getch();

        if (st.replaceMode || st.findMode || st.saveAsMode || st.gotoMode || st.commandMode || st.fileTreeFilterMode)
        {
            bool handled = TuiInput::handleInputModes(ch, tabs, cur, fileTree, st, st.fileMtimes);
            if (handled)
            {
                if (st.statusMsg == QStringLiteral("__QUIT__"))
                    break;
                continue;
            }
        }

        if (st.fileTreeVisible && st.fileTreeFocused)
        {
            // handleInputModes already handled filter mode; now handle tree navigation
            bool handled = TuiInput::handleFileTreeInput(ch, fileTree, st, tabs, st.fileMtimes);
            if (handled)
                continue;
        }

        bool shouldQuit = TuiInput::handleGlobalInput(ch, tabs, cur, fileTree, st, st.fileMtimes, editorH);
        if (shouldQuit)
            break;
        if (st.statusMsg == QStringLiteral("__QUIT__"))
            break;
    }

    TuiSession::save(parser, tabs, fileTree);

    endwin();
    return 0;
#endif
}
