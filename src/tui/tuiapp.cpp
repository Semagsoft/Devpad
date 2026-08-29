/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuiapp.h"

#include "core/fileservice.h"
#include "tui/tuibuffer.h"
#include "tui/tuisearchengine.h"
#include "tui/tuitabmodel.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
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

    // Load positional files via headless FileService
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
                // Open empty buffer with error noted in status
                TuiBuffer buf(filePath, QString(), QStringLiteral("UTF-8"));
                tabs.addBuffer(buf);
            }
        }
        else if (fi.isDir())
        {
            // For TUI, opening a folder just notes it; actual file tree is out of scope for minimal build
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
            // New untitled buffer for non-existent path interpreted as new file
            TuiBuffer buf(filePath, QString(), QStringLiteral("UTF-8"));
            tabs.addBuffer(buf);
        }
    }

    if (tabs.isEmpty())
    {
        tabs.addBuffer(TuiBuffer(QString(), QString(), QStringLiteral("UTF-8")));
    }

    // Ensure we have a tty; otherwise just cat files and exit (useful for CI)
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

    // ncurses init
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
        init_pair(1, COLOR_BLACK, COLOR_CYAN); // status bar
        init_pair(2, COLOR_YELLOW, -1); // line numbers
        init_pair(3, COLOR_CYAN, -1); // bookmark
    }

    int scrollTop = 0;
    QString statusMsg = QStringLiteral("Ctrl+Q quit  Ctrl+S save  Ctrl+W close  Ctrl+N new  Ctrl+F find  F2 bookmark");
    QString findQuery;
    SearchOptions findOpts;
    SearchResult lastSearch;
    bool findMode = false;
    QString findInput;

    auto saveCurrent = [&](TuiBuffer* buf) -> bool {
        if (!buf)
            return false;
        QString path = buf->filePath();
        if (path.isEmpty() || path == QStringLiteral("Untitled"))
        {
            statusMsg = QStringLiteral("No file name - cannot save untitled in TUI (use :w <path> pattern in future)");
            return false;
        }
        QString err;
        bool ok = FileService::save(path, buf->text(), buf->encoding(), &err);
        if (ok)
        {
            buf->setModified(false);
            statusMsg = QStringLiteral("Saved %1").arg(path);
        }
        else
        {
            statusMsg = err;
        }
        return ok;
    };

    int ch = 0;
    while (true)
    {
        TuiBuffer* cur = tabs.currentBuffer();
        if (!cur)
        {
            tabs.addBuffer(TuiBuffer());
            cur = tabs.currentBuffer();
        }

        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        int tabBarH = 1;
        int statusBarH = 2;
        int editorH = rows - tabBarH - statusBarH;
        if (editorH < 1)
            editorH = 1;

        // Keep cursor in view
        if (cur->cursorLine() < scrollTop)
            scrollTop = cur->cursorLine();
        if (cur->cursorLine() >= scrollTop + editorH)
            scrollTop = cur->cursorLine() - editorH + 1;

        // Draw tab bar
        attron(A_REVERSE);
        for (int c = 0; c < cols; ++c)
            mvaddch(0, c, ' ');
        int x = 0;
        for (int i = 0; i < tabs.count(); ++i)
        {
            const TuiBuffer* b = tabs.bufferAt(i);
            QString label = b->displayName();
            if (b->isModified())
                label += QStringLiteral("*");
            if (tabs.isPinned(i))
                label.prepend(QString::fromUtf8("\xF0\x9F\x93\x8C "));
            label = QStringLiteral(" %1 ").arg(label);
            if (x + label.size() + 1 >= cols)
                break;
            if (i == tabs.currentIndex())
                attron(A_BOLD);
            mvaddnstr(0, x, label.toUtf8().constData(), label.toUtf8().size());
            if (i == tabs.currentIndex())
                attroff(A_BOLD);
            x += label.size();
            mvaddch(0, x++, '|');
        }
        attroff(A_REVERSE);

        // Draw editor
        for (int r = 0; r < editorH; ++r)
        {
            int lineIdx = scrollTop + r;
            int y = tabBarH + r;
            for (int c = 0; c < cols; ++c)
                mvaddch(y, c, ' ');

            if (lineIdx >= cur->lineCount())
                continue;

            QString line = cur->lines().at(lineIdx);
            bool isBm = cur->hasBookmark(lineIdx);
            QString gutter;
            if (hasColors && isBm)
                gutter = QString::fromUtf8("\xE2\x98\x85 ");
            else
                gutter = QStringLiteral("  ");

            // Line number
            QString lnStr = QStringLiteral("%1 ").arg(lineIdx + 1, 4);
            if (hasColors)
                attron(COLOR_PAIR(2));
            mvaddnstr(y, 0, lnStr.toUtf8().constData(), qMin(lnStr.toUtf8().size(), cols));
            if (hasColors)
                attroff(COLOR_PAIR(2));

            int gutterX = 5;
            if (hasColors && isBm)
                attron(COLOR_PAIR(3));
            mvaddnstr(y, gutterX, gutter.toUtf8().constData(), gutter.size());
            if (hasColors && isBm)
                attroff(COLOR_PAIR(3));

            int textX = gutterX + 2;
            int avail = cols - textX;
            if (avail <= 0)
                continue;

            // Simple horizontal scroll based on cursor col
            int hScroll = 0;
            if (cur->cursorLine() == lineIdx && cur->cursorCol() >= avail)
                hScroll = cur->cursorCol() - avail + 1;

            QString visible = line.mid(hScroll, avail);
            // Highlight search match if on this line
            if (lastSearch.found && lastSearch.line == lineIdx)
            {
                int start = lastSearch.column - hScroll;
                int len = lastSearch.length;
                if (start < 0)
                {
                    len += start;
                    start = 0;
                }
                if (start < avail && len > 0)
                {
                    mvaddnstr(y, textX, visible.left(start).toUtf8().constData(), start);
                    attron(A_REVERSE);
                    mvaddnstr(y, textX + start, visible.mid(start, len).toUtf8().constData(), len);
                    attroff(A_REVERSE);
                    if (start + len < visible.size())
                        mvaddnstr(y, textX + start + len, visible.mid(start + len).toUtf8().constData(), visible.size() - start - len);
                }
                else
                {
                    mvaddnstr(y, textX, visible.toUtf8().constData(), visible.toUtf8().size());
                }
            }
            else
            {
                mvaddnstr(y, textX, visible.toUtf8().constData(), visible.toUtf8().size());
            }

            if (lineIdx == cur->cursorLine())
            {
                int curX = textX + (cur->cursorCol() - hScroll);
                if (curX >= textX && curX < cols)
                    move(y, curX);
            }
        }

        // Status bar
        int statusY = rows - 2;
        int msgY = rows - 1;
        if (hasColors)
            attron(COLOR_PAIR(1));
        else
            attron(A_REVERSE);
        for (int c = 0; c < cols; ++c)
        {
            mvaddch(statusY, c, ' ');
            mvaddch(msgY, c, ' ');
        }
        QString statusLeft = QStringLiteral(" %1  %2:%3  %4  %5 ")
                                 .arg(cur->filePath().isEmpty() ? QStringLiteral("(untitled)") : cur->filePath())
                                 .arg(cur->cursorLine() + 1)
                                 .arg(cur->cursorCol() + 1)
                                 .arg(cur->encoding())
                                 .arg(cur->isModified() ? QStringLiteral("[+]") : QStringLiteral(""));
        QString statusRight = QStringLiteral(" Tab %1/%2 ").arg(tabs.currentIndex() + 1).arg(tabs.count());
        mvaddnstr(statusY, 0, statusLeft.toUtf8().constData(), qMin(statusLeft.toUtf8().size(), cols - statusRight.toUtf8().size() - 1));
        mvaddnstr(statusY, cols - statusRight.toUtf8().size(), statusRight.toUtf8().constData(), statusRight.toUtf8().size());
        QString msg = findMode ? QStringLiteral("Find: ") + findInput : statusMsg;
        mvaddnstr(msgY, 0, msg.toUtf8().constData(), qMin(msg.toUtf8().size(), cols));
        if (hasColors)
            attroff(COLOR_PAIR(1));
        else
            attroff(A_REVERSE);

        refresh();

        ch = getch();

        if (findMode)
        {
            if (ch == 27) // ESC
            {
                findMode = false;
                findInput.clear();
            }
            else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
            {
                findQuery = findInput;
                findMode = false;
                if (!findQuery.isEmpty())
                {
                    SearchResult r = TuiSearchEngine::findNext(cur->lines(), findQuery, findOpts, cur->cursorLine(), cur->cursorCol() + 1, true, true);
                    if (r.found)
                    {
                        cur->setCursor(r.line, r.column);
                        lastSearch = r;
                        statusMsg = QStringLiteral("Found at %1:%2").arg(r.line + 1).arg(r.column + 1);
                    }
                    else
                    {
                        statusMsg = QStringLiteral("Not found: %1").arg(findQuery);
                        lastSearch = {};
                    }
                }
            }
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            {
                if (!findInput.isEmpty())
                    findInput.chop(1);
            }
            else if (ch >= 32 && ch < 127)
            {
                findInput.append(QChar(ch));
            }
            continue;
        }

        // Global shortcuts
        if (ch == 17) // Ctrl+Q
        {
            bool anyModified = false;
            for (int i = 0; i < tabs.count(); ++i)
            {
                if (tabs.bufferAt(i)->isModified())
                    anyModified = true;
            }
            if (anyModified)
            {
                statusMsg = QStringLiteral("Modified buffers exist. Press Ctrl+Q again to quit without saving, or save with Ctrl+S");
                // Require second Ctrl+Q
                int ch2 = getch();
                if (ch2 != 17)
                {
                    statusMsg = QStringLiteral("Quit cancelled");
                    // push back handling for ch2? simplified: handle ch2 as next loop's ch
                    if (ch2 == 19) // Ctrl+S
                        saveCurrent(cur);
                    continue;
                }
            }
            break;
        }
        else if (ch == 19) // Ctrl+S
        {
            saveCurrent(cur);
        }
        else if (ch == 23) // Ctrl+W close tab
        {
            if (cur->isModified())
            {
                statusMsg = QStringLiteral("Buffer modified. Save? (y/n/c)");
                refresh();
                int a = getch();
                if (a == 'y' || a == 'Y')
                    saveCurrent(cur);
                else if (a == 'c' || a == 'C' || a == 27)
                {
                    statusMsg = QStringLiteral("Close cancelled");
                    continue;
                }
            }
            tabs.closeCurrent();
            if (tabs.isEmpty())
                tabs.addBuffer(TuiBuffer());
            statusMsg = QStringLiteral("Tab closed");
        }
        else if (ch == 14) // Ctrl+N
        {
            tabs.addBuffer(TuiBuffer());
            statusMsg = QStringLiteral("New buffer");
            scrollTop = 0;
        }
        else if (ch == 6) // Ctrl+F
        {
            findMode = true;
            findInput = findQuery;
            statusMsg = QStringLiteral("Find mode");
        }
        else if (ch == KEY_F(2))
        {
            cur->toggleBookmark(cur->cursorLine());
            statusMsg = cur->hasBookmark(cur->cursorLine()) ? QStringLiteral("Bookmark set") : QStringLiteral("Bookmark cleared");
        }
        else if (ch == KEY_NPAGE) // PageDown
        {
            cur->moveCursor(editorH - 2, 0);
        }
        else if (ch == KEY_PPAGE) // PageUp
        {
            cur->moveCursor(-editorH + 2, 0);
        }
        else if (ch == KEY_F(3)) // Find next
        {
            if (!findQuery.isEmpty())
            {
                SearchResult r = TuiSearchEngine::findNext(cur->lines(), findQuery, findOpts, cur->cursorLine(), cur->cursorCol() + 1, true, true);
                if (r.found)
                {
                    cur->setCursor(r.line, r.column);
                    lastSearch = r;
                }
                else
                    statusMsg = QStringLiteral("Not found");
            }
        }
        else if (ch == 9) // Tab -> next tab (Ctrl+Tab is often 9)
        {
            // Use Ctrl+Tab simulation: Tab cycles
            int next = (tabs.currentIndex() + 1) % tabs.count();
            tabs.setCurrentIndex(next);
            scrollTop = 0;
        }
        else if (ch == KEY_LEFT)
        {
            if (cur->cursorCol() > 0)
                cur->setCursor(cur->cursorLine(), cur->cursorCol() - 1);
            else if (cur->cursorLine() > 0)
                cur->setCursor(cur->cursorLine() - 1, cur->lines().at(cur->cursorLine() - 1).size());
        }
        else if (ch == KEY_RIGHT)
        {
            int lineLen = cur->lines().at(cur->cursorLine()).size();
            if (cur->cursorCol() < lineLen)
                cur->setCursor(cur->cursorLine(), cur->cursorCol() + 1);
            else if (cur->cursorLine() + 1 < cur->lineCount())
                cur->setCursor(cur->cursorLine() + 1, 0);
        }
        else if (ch == KEY_UP)
        {
            cur->moveCursor(-1, 0);
        }
        else if (ch == KEY_DOWN)
        {
            cur->moveCursor(1, 0);
        }
        else if (ch == KEY_HOME || ch == 1) // Ctrl+A
        {
            cur->setCursor(cur->cursorLine(), 0);
        }
        else if (ch == KEY_END || ch == 5) // Ctrl+E
        {
            cur->setCursor(cur->cursorLine(), cur->lines().at(cur->cursorLine()).size());
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
        {
            cur->backspace();
        }
        else if (ch == KEY_DC) // Delete
        {
            cur->deleteChar();
        }
        else if (ch == KEY_ENTER || ch == 10 || ch == 13)
        {
            cur->newLine();
        }
        else if (ch >= 32 && ch < 127)
        {
            cur->insertChar(QChar(ch));
        }
        else if (ch == 2) // Ctrl+B - toggle find whole words placeholder
        {
            statusMsg = QStringLiteral("TUI: %1 buffers").arg(tabs.count());
        }
        else if (ch == KEY_F(1))
        {
            statusMsg = QStringLiteral("Help: Ctrl+Q quit  Ctrl+S save  Ctrl+W close  Ctrl+N new  Ctrl+F find  F2 bookmark  F3 find next  Arrows move  Tab switch tab");
        }
    }

    endwin();
    return 0;
#endif
}
