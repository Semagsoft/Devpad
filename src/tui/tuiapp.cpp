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
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QTextStream>

#include <algorithm>
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
    bool replaceMode = false;
    int replacePhase = 0; // 0=find, 1=replace
    QString replaceFindInput;
    QString replaceInput;
    bool saveAsMode = false;
    QString saveAsInput;
    bool gotoMode = false;
    QString gotoInput;
    bool commandMode = false;
    QString commandInput;
    QString clipboard;
    QHash<QString, QDateTime> fileMtimes;

    auto saveCurrent = [&](TuiBuffer* buf) -> bool {
        if (!buf)
            return false;
        QString path = buf->filePath();
        if (path.isEmpty() || path == QStringLiteral("Untitled"))
        {
            statusMsg = QStringLiteral("No file name - use Ctrl+O or :w <path> to save");
            return false;
        }
        QString err;
        bool ok = FileService::save(path, buf->text(), buf->encoding(), &err);
        if (ok)
        {
            buf->setModified(false);
            fileMtimes[path] = QFileInfo(path).lastModified();
            statusMsg = QStringLiteral("Saved %1").arg(path);
        }
        else
        {
            statusMsg = err;
        }
        return ok;
    };

    auto saveAs = [&](TuiBuffer* buf, const QString& newPath) -> bool {
        if (!buf || newPath.isEmpty())
            return false;
        QString err;
        bool ok = FileService::save(newPath, buf->text(), buf->encoding(), &err);
        if (ok)
        {
            buf->setFilePath(newPath);
            buf->setModified(false);
            fileMtimes[newPath] = QFileInfo(newPath).lastModified();
            statusMsg = QStringLiteral("Saved as %1").arg(newPath);
        }
        else
        {
            statusMsg = err;
        }
        return ok;
    };

    auto saveAll = [&]() {
        int saved = 0;
        for (int i = 0; i < tabs.count(); ++i)
        {
            TuiBuffer* b = tabs.bufferAt(i);
            if (b && b->isModified() && !b->filePath().isEmpty() && b->filePath() != QStringLiteral("Untitled"))
            {
                QString err;
                if (FileService::save(b->filePath(), b->text(), b->encoding(), &err))
                {
                    b->setModified(false);
                    fileMtimes[b->filePath()] = QFileInfo(b->filePath()).lastModified();
                    saved++;
                }
            }
        }
        statusMsg = QStringLiteral("Saved %1 buffers").arg(saved);
    };

    // Initialize file mtimes for watcher
    for (int i = 0; i < tabs.count(); ++i)
    {
        const TuiBuffer* b = tabs.bufferAt(i);
        if (b && !b->filePath().isEmpty())
            fileMtimes[b->filePath()] = QFileInfo(b->filePath()).lastModified();
    }

    int ch = 0;
    while (true)
    {
        TuiBuffer* cur = tabs.currentBuffer();
        if (!cur)
        {
            tabs.addBuffer(TuiBuffer());
            cur = tabs.currentBuffer();
        }

        // Poll for external file changes
        if (!cur->filePath().isEmpty() && fileMtimes.contains(cur->filePath()))
        {
            QFileInfo fi(cur->filePath());
            if (fi.exists())
            {
                QDateTime curMod = fi.lastModified();
                QDateTime lastMod = fileMtimes.value(cur->filePath());
                if (curMod.isValid() && lastMod.isValid() && curMod != lastMod)
                {
                    if (!cur->isModified())
                    {
                        FileLoadResult res = FileService::load(cur->filePath());
                        if (res.ok)
                        {
                            cur->setText(res.text);
                            cur->setEncoding(res.encoding);
                            cur->setModified(false);
                            fileMtimes[cur->filePath()] = curMod;
                            statusMsg = QStringLiteral("File reloaded (external change): %1").arg(cur->filePath());
                        }
                        else
                        {
                            fileMtimes[cur->filePath()] = curMod;
                            statusMsg = QStringLiteral("External change detected but reload failed");
                        }
                    }
                    else
                    {
                        // Modified buffer + external change -> warn, avoid spamming by updating mtime to current
                        // User can :e! to force reload or :w to overwrite
                        statusMsg = QStringLiteral("WARNING: File changed on disk — :e! to reload, :w to overwrite");
                        fileMtimes[cur->filePath()] = curMod;
                    }
                }
            }
        }
        else if (!cur->filePath().isEmpty() && !fileMtimes.contains(cur->filePath()) && QFile::exists(cur->filePath()))
        {
            fileMtimes[cur->filePath()] = QFileInfo(cur->filePath()).lastModified();
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

            // Compute selection highlight for this line
            int selStart = -1;
            int selEnd = -1;
            if (cur->hasSelection())
            {
                int aLine = cur->selectionAnchorLine();
                int aCol = cur->selectionAnchorCol();
                int cLine = cur->cursorLine();
                int cCol = cur->cursorCol();
                if (aLine > cLine || (aLine == cLine && aCol > cCol))
                {
                    std::swap(aLine, cLine);
                    std::swap(aCol, cCol);
                }
                if (lineIdx >= aLine && lineIdx <= cLine)
                {
                    if (aLine == cLine)
                    {
                        selStart = aCol - hScroll;
                        selEnd = cCol - hScroll;
                    }
                    else if (lineIdx == aLine)
                    {
                        selStart = aCol - hScroll;
                        selEnd = avail;
                    }
                    else if (lineIdx == cLine)
                    {
                        selStart = 0 - hScroll;
                        if (selStart < 0)
                            selStart = 0;
                        selEnd = cCol - hScroll;
                    }
                    else
                    {
                        selStart = 0;
                        selEnd = avail;
                    }
                    selStart = qBound(0, selStart, avail);
                    selEnd = qBound(0, selEnd, avail);
                    if (selStart == selEnd)
                    {
                        selStart = -1;
                        selEnd = -1;
                    }
                }
            }

            // Highlight search match if on this line (and selection not covering)
            if (lastSearch.found && lastSearch.line == lineIdx && selStart == -1)
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
            else if (selStart != -1)
            {
                mvaddnstr(y, textX, visible.left(selStart).toUtf8().constData(), selStart);
                attron(A_REVERSE);
                mvaddnstr(y, textX + selStart, visible.mid(selStart, selEnd - selStart).toUtf8().constData(), selEnd - selStart);
                attroff(A_REVERSE);
                if (selEnd < visible.size())
                    mvaddnstr(y, textX + selEnd, visible.mid(selEnd).toUtf8().constData(), visible.size() - selEnd);
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
        QString msg;
        if (findMode)
            msg = QStringLiteral("Find: ") + findInput;
        else if (replaceMode)
            msg = (replacePhase == 0 ? QStringLiteral("Find: ") + replaceFindInput : QStringLiteral("Replace: ") + replaceInput);
        else if (saveAsMode)
            msg = QStringLiteral("Save As: ") + saveAsInput;
        else if (gotoMode)
            msg = QStringLiteral("Go to line: ") + gotoInput;
        else if (commandMode)
            msg = QStringLiteral(":") + commandInput;
        else
            msg = statusMsg;
        mvaddnstr(msgY, 0, msg.toUtf8().constData(), qMin(msg.toUtf8().size(), cols));
        // Position cursor at end of input when in input modes
        if (findMode)
            move(msgY, 6 + findInput.size());
        else if (replaceMode)
        {
            if (replacePhase == 0)
                move(msgY, 6 + replaceFindInput.size());
            else
                move(msgY, 9 + replaceInput.size());
        }
        else if (saveAsMode)
            move(msgY, 9 + saveAsInput.size());
        else if (gotoMode)
            move(msgY, 12 + gotoInput.size());
        else if (commandMode)
            move(msgY, 1 + commandInput.size());
        if (hasColors)
            attroff(COLOR_PAIR(1));
        else
            attroff(A_REVERSE);

        refresh();

        ch = getch();

        if (replaceMode)
        {
            if (ch == 27)
            {
                replaceMode = false;
                replacePhase = 0;
                replaceFindInput.clear();
                replaceInput.clear();
                statusMsg = QStringLiteral("Replace cancelled");
            }
            else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
            {
                if (replacePhase == 0)
                {
                    // Move to replace input
                    replacePhase = 1;
                }
                else
                {
                    // Execute replace
                    QString find = replaceFindInput;
                    QString repl = replaceInput;
                    replaceMode = false;
                    replacePhase = 0;
                    if (find.isEmpty())
                    {
                        statusMsg = QStringLiteral("Find string empty");
                    }
                    else
                    {
                        // Single replace at cursor then prompt for next? For now replace next and show status
                        // If Shift held? For now single.
                        auto rr = cur->replaceNext(find, repl, findOpts, true);
                        if (rr.found)
                        {
                            lastSearch = {true, rr.line, rr.column, rr.length};
                            statusMsg = QStringLiteral("Replaced 1 at %1:%2").arg(rr.line + 1).arg(rr.column + 1);
                        }
                        else
                        {
                            statusMsg = QStringLiteral("Not found: %1").arg(find);
                            lastSearch = {};
                        }
                    }
                    replaceFindInput.clear();
                    replaceInput.clear();
                }
            }
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            {
                if (replacePhase == 0)
                {
                    if (!replaceFindInput.isEmpty())
                        replaceFindInput.chop(1);
                }
                else
                {
                    if (!replaceInput.isEmpty())
                        replaceInput.chop(1);
                }
            }
            else if (ch >= 32 && ch < 127)
            {
                if (replacePhase == 0)
                    replaceFindInput.append(QChar(ch));
                else
                    replaceInput.append(QChar(ch));
            }
            continue;
        }

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

        if (saveAsMode)
        {
            if (ch == 27)
            {
                saveAsMode = false;
                saveAsInput.clear();
                statusMsg = QStringLiteral("Save cancelled");
            }
            else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
            {
                QString path = saveAsInput.trimmed();
                if (!path.isEmpty())
                {
                    if (!QFileInfo(path).isAbsolute())
                        path = QDir::current().absoluteFilePath(path);
                    saveAs(cur, path);
                }
                saveAsMode = false;
                saveAsInput.clear();
            }
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            {
                if (!saveAsInput.isEmpty())
                    saveAsInput.chop(1);
            }
            else if (ch >= 32 && ch < 127)
            {
                saveAsInput.append(QChar(ch));
            }
            continue;
        }

        if (gotoMode)
        {
            if (ch == 27)
            {
                gotoMode = false;
                gotoInput.clear();
            }
            else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
            {
                bool ok = false;
                int line = gotoInput.toInt(&ok);
                if (ok && line >= 1 && line <= cur->lineCount())
                {
                    cur->setCursor(line - 1, 0);
                    statusMsg = QStringLiteral("Go to line %1").arg(line);
                }
                else
                {
                    statusMsg = QStringLiteral("Invalid line: %1").arg(gotoInput);
                }
                gotoMode = false;
                gotoInput.clear();
            }
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            {
                if (!gotoInput.isEmpty())
                    gotoInput.chop(1);
            }
            else if (ch >= 48 && ch <= 57)
            {
                gotoInput.append(QChar(ch));
            }
            continue;
        }

        if (commandMode)
        {
            if (ch == 27)
            {
                commandMode = false;
                commandInput.clear();
            }
            else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
            {
                QString cmd = commandInput.trimmed();
                commandMode = false;
                commandInput.clear();
                if (cmd == QStringLiteral("wa"))
                {
                    saveAll();
                }
                else if (cmd == QStringLiteral("wqa") || cmd == QStringLiteral("waq"))
                {
                    saveAll();
                    bool anyModified = false;
                    for (int i = 0; i < tabs.count(); ++i)
                        if (tabs.bufferAt(i)->isModified() && tabs.bufferAt(i)->filePath().isEmpty())
                            anyModified = true;
                    if (!anyModified)
                        break;
                }
                else if (cmd == QStringLiteral("w") || cmd.startsWith(QStringLiteral("w ")))
                {
                    QString arg = cmd.mid(1).trimmed();
                    if (arg.isEmpty())
                    {
                        if (!saveCurrent(cur))
                        {
                            saveAsMode = true;
                            saveAsInput.clear();
                        }
                    }
                    else
                    {
                        if (!QFileInfo(arg).isAbsolute())
                            arg = QDir::current().absoluteFilePath(arg);
                        saveAs(cur, arg);
                    }
                }
                else if (cmd == QStringLiteral("q"))
                {
                    bool anyModified = false;
                    for (int i = 0; i < tabs.count(); ++i)
                        if (tabs.bufferAt(i)->isModified())
                            anyModified = true;
                    if (!anyModified)
                        break;
                    statusMsg = QStringLiteral("Modified buffers - use :wq or :q! to force");
                }
                else if (cmd == QStringLiteral("q!"))
                {
                    break;
                }
                else if (cmd == QStringLiteral("wq") || cmd.startsWith(QStringLiteral("wq ")))
                {
                    QString arg = cmd.mid(2).trimmed();
                    if (!arg.isEmpty())
                    {
                        if (!QFileInfo(arg).isAbsolute())
                            arg = QDir::current().absoluteFilePath(arg);
                        saveAs(cur, arg);
                    }
                    else
                    {
                        saveCurrent(cur);
                    }
                    bool anyModified = false;
                    for (int i = 0; i < tabs.count(); ++i)
                        if (tabs.bufferAt(i)->isModified() && tabs.bufferAt(i)->filePath().isEmpty())
                            anyModified = true;
                    if (!anyModified)
                        break;
                }
                else if (cmd == QStringLiteral("e!"))
                {
                    if (!cur->filePath().isEmpty())
                    {
                        FileLoadResult res = FileService::load(cur->filePath());
                        if (res.ok)
                        {
                            cur->setText(res.text);
                            cur->setEncoding(res.encoding);
                            cur->setModified(false);
                            fileMtimes[cur->filePath()] = QFileInfo(cur->filePath()).lastModified();
                            statusMsg = QStringLiteral("Reloaded %1").arg(cur->filePath());
                        }
                        else
                        {
                            statusMsg = QStringLiteral("Reload failed: %1").arg(res.error);
                        }
                    }
                    else
                    {
                        statusMsg = QStringLiteral("No file name");
                    }
                }
                else if (cmd.startsWith(QStringLiteral("e! ")))
                {
                    QString arg = cmd.mid(3).trimmed();
                    if (!arg.isEmpty())
                    {
                        if (!QFileInfo(arg).isAbsolute())
                            arg = QDir::current().absoluteFilePath(arg);
                        FileLoadResult res = FileService::load(arg);
                        if (res.ok)
                        {
                            cur->setText(res.text);
                            cur->setEncoding(res.encoding);
                            cur->setFilePath(arg);
                            cur->setModified(false);
                            fileMtimes[arg] = QFileInfo(arg).lastModified();
                            statusMsg = QStringLiteral("Reloaded %1").arg(arg);
                        }
                        else
                        {
                            statusMsg = QStringLiteral("Reload failed: %1").arg(res.error);
                        }
                    }
                }
                else if (cmd.startsWith(QStringLiteral("e ")))
                {
                    QString arg = cmd.mid(2).trimmed();
                    if (!arg.isEmpty())
                    {
                        if (!QFileInfo(arg).isAbsolute())
                            arg = QDir::current().absoluteFilePath(arg);
                        FileLoadResult res = FileService::load(arg);
                        if (res.ok)
                        {
                            TuiBuffer buf(arg, res.text, res.encoding);
                            tabs.addBuffer(buf);
                            fileMtimes[arg] = QFileInfo(arg).lastModified();
                            statusMsg = QStringLiteral("Opened %1").arg(arg);
                        }
                        else
                        {
                            TuiBuffer buf(arg, QString(), QStringLiteral("UTF-8"));
                            tabs.addBuffer(buf);
                            statusMsg = QStringLiteral("New file %1").arg(arg);
                        }
                    }
                }
                else if (cmd.startsWith(QStringLiteral("s/")) || cmd.startsWith(QStringLiteral("%s/")))
                {
                    bool global = cmd.startsWith(QStringLiteral("%"));
                    QString rest = global ? cmd.mid(3) : cmd.mid(2);
                    // rest is "old/new/flags" - split by '/' keeping empty parts
                    QStringList parts = rest.split(QChar('/'));
                    if (parts.size() >= 2)
                    {
                        QString find = parts[0];
                        QString repl = parts[1];
                        QString flags = parts.size() >= 3 ? parts[2] : QString();
                        bool all = global || flags.contains(QLatin1Char('g'));
                        if (find.isEmpty())
                        {
                            statusMsg = QStringLiteral("Find string empty");
                        }
                        else if (all)
                        {
                            int cnt = cur->replaceAll(find, repl, findOpts);
                            statusMsg = cnt > 0 ? QStringLiteral("Replaced %1 occurrences").arg(cnt) : QStringLiteral("Not found: %1").arg(find);
                            if (cnt > 0)
                                lastSearch = {};
                        }
                        else
                        {
                            auto rr = cur->replaceNext(find, repl, findOpts, true);
                            if (rr.found)
                            {
                                lastSearch = {true, rr.line, rr.column, rr.length};
                                statusMsg = QStringLiteral("Replaced at %1:%2").arg(rr.line + 1).arg(rr.column + 1);
                            }
                            else
                            {
                                statusMsg = QStringLiteral("Not found: %1").arg(find);
                                lastSearch = {};
                            }
                        }
                    }
                    else
                    {
                        statusMsg = QStringLiteral("Usage: :s/old/new/[g] or :%%s/old/new/[g]");
                    }
                }
                else
                {
                    statusMsg = QStringLiteral("Unknown command: %1").arg(cmd);
                }
            }
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            {
                if (!commandInput.isEmpty())
                    commandInput.chop(1);
                else
                {
                    commandMode = false;
                }
            }
            else if (ch >= 32 && ch < 127)
            {
                commandInput.append(QChar(ch));
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
                int ch2 = getch();
                if (ch2 != 17)
                {
                    statusMsg = QStringLiteral("Quit cancelled");
                    if (ch2 == 19)
                        saveCurrent(cur);
                    continue;
                }
            }
            break;
        }
        else if (ch == 19) // Ctrl+S save
        {
            saveCurrent(cur);
        }
        else if (ch == 15) // Ctrl+O save as
        {
            saveAsMode = true;
            saveAsInput = cur->filePath();
            if (saveAsInput.isEmpty())
                saveAsInput = QDir::current().absoluteFilePath(QStringLiteral("untitled.txt"));
        }
        else if (ch == 7) // Ctrl+G goto line
        {
            gotoMode = true;
            gotoInput.clear();
        }
        else if (ch == 8) // Ctrl+H replace
        {
            replaceMode = true;
            replacePhase = 0;
            replaceFindInput.clear();
            replaceInput.clear();
        }
        else if (ch == 58) // : command mode
        {
            commandMode = true;
            commandInput.clear();
        }
        else if (ch == 26) // Ctrl+Z undo
        {
            if (!cur->undo())
                statusMsg = QStringLiteral("Nothing to undo");
            else
                statusMsg = QStringLiteral("Undo");
        }
        else if (ch == 25 || ch == 18) // Ctrl+Y or Ctrl+R redo (Ctrl+R=18)
        {
            if (!cur->redo())
                statusMsg = QStringLiteral("Nothing to redo");
            else
                statusMsg = QStringLiteral("Redo");
        }
        else if (ch == 1) // Ctrl+A select all
        {
            cur->selectAll();
            statusMsg = QStringLiteral("Selected all");
        }
        else if (ch == 3) // Ctrl+C copy
        {
            if (cur->hasSelection())
            {
                clipboard = cur->selectedText();
                statusMsg = QStringLiteral("Copied %1 chars").arg(clipboard.size());
            }
            else
            {
                statusMsg = QStringLiteral("No selection");
            }
        }
        else if (ch == 24) // Ctrl+X cut
        {
            if (cur->hasSelection())
            {
                clipboard = cur->selectedText();
                cur->deleteSelection();
                statusMsg = QStringLiteral("Cut %1 chars").arg(clipboard.size());
            }
            else
            {
                statusMsg = QStringLiteral("No selection");
            }
        }
        else if (ch == 22) // Ctrl+V paste
        {
            if (!clipboard.isEmpty())
            {
                cur->insertText(clipboard);
                statusMsg = QStringLiteral("Pasted");
            }
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
            if (cur->hasSelection())
                cur->clearSelection();
            cur->moveCursor(editorH - 2, 0);
        }
        else if (ch == KEY_PPAGE) // PageUp
        {
            if (cur->hasSelection())
                cur->clearSelection();
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
        else if (ch == 9) // Tab -> next tab
        {
            int next = (tabs.currentIndex() + 1) % tabs.count();
            tabs.setCurrentIndex(next);
            scrollTop = 0;
            if (cur->hasSelection())
                cur->clearSelection();
        }
        else if (ch == KEY_LEFT)
        {
            if (cur->hasSelection())
                cur->clearSelection();
            if (cur->cursorCol() > 0)
                cur->setCursor(cur->cursorLine(), cur->cursorCol() - 1);
            else if (cur->cursorLine() > 0)
                cur->setCursor(cur->cursorLine() - 1, cur->lines().at(cur->cursorLine() - 1).size());
        }
        else if (ch == KEY_RIGHT)
        {
            if (cur->hasSelection())
                cur->clearSelection();
            int lineLen = cur->lines().at(cur->cursorLine()).size();
            if (cur->cursorCol() < lineLen)
                cur->setCursor(cur->cursorLine(), cur->cursorCol() + 1);
            else if (cur->cursorLine() + 1 < cur->lineCount())
                cur->setCursor(cur->cursorLine() + 1, 0);
        }
        else if (ch == KEY_UP)
        {
            if (cur->hasSelection())
                cur->clearSelection();
            cur->moveCursor(-1, 0);
        }
        else if (ch == KEY_DOWN)
        {
            if (cur->hasSelection())
                cur->clearSelection();
            cur->moveCursor(1, 0);
        }
#ifdef KEY_SLEFT
        else if (ch == KEY_SLEFT)
        {
            if (!cur->hasSelection())
                cur->setSelectionAnchor(cur->cursorLine(), cur->cursorCol());
            if (cur->cursorCol() > 0)
                cur->setCursor(cur->cursorLine(), cur->cursorCol() - 1);
            else if (cur->cursorLine() > 0)
                cur->setCursor(cur->cursorLine() - 1, cur->lines().at(cur->cursorLine() - 1).size());
        }
        else if (ch == KEY_SRIGHT)
        {
            if (!cur->hasSelection())
                cur->setSelectionAnchor(cur->cursorLine(), cur->cursorCol());
            int lineLen = cur->lines().at(cur->cursorLine()).size();
            if (cur->cursorCol() < lineLen)
                cur->setCursor(cur->cursorLine(), cur->cursorCol() + 1);
            else if (cur->cursorLine() + 1 < cur->lineCount())
                cur->setCursor(cur->cursorLine() + 1, 0);
        }
        else if (ch == KEY_SR) // Shift + Up (scroll backward)
        {
            if (!cur->hasSelection())
                cur->setSelectionAnchor(cur->cursorLine(), cur->cursorCol());
            cur->moveCursor(-1, 0);
        }
        else if (ch == KEY_SF) // Shift + Down
        {
            if (!cur->hasSelection())
                cur->setSelectionAnchor(cur->cursorLine(), cur->cursorCol());
            cur->moveCursor(1, 0);
        }
        else if (ch == KEY_SHOME)
        {
            if (!cur->hasSelection())
                cur->setSelectionAnchor(cur->cursorLine(), cur->cursorCol());
            cur->setCursor(cur->cursorLine(), 0);
        }
        else if (ch == KEY_SEND)
        {
            if (!cur->hasSelection())
                cur->setSelectionAnchor(cur->cursorLine(), cur->cursorCol());
            cur->setCursor(cur->cursorLine(), cur->lines().at(cur->cursorLine()).size());
        }
#endif
        else if (ch == KEY_HOME)
        {
            if (cur->hasSelection())
                cur->clearSelection();
            cur->setCursor(cur->cursorLine(), 0);
        }
        else if (ch == KEY_END)
        {
            if (cur->hasSelection())
                cur->clearSelection();
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
        else if (ch == 2) // Ctrl+B
        {
            statusMsg = QStringLiteral("TUI: %1 buffers  Undo:%2 Redo:%3 Sel:%4").arg(tabs.count()).arg(cur->canUndo() ? "Y" : "N").arg(cur->canRedo() ? "Y" : "N").arg(cur->hasSelection() ? "Y" : "N");
        }
        else if (ch == KEY_F(1))
        {
            statusMsg = QStringLiteral("Help: Ctrl+Q quit  Ctrl+S save  Ctrl+O saveAs  Ctrl+G goto  Ctrl+H replace  :s/old/new/g :%%s/old/new/g  :w/:wa/:q/:e  Ctrl+Z undo  Ctrl+Y redo  Ctrl+A selAll  Ctrl+X/C/V  Shift+Arrows  Ctrl+F find  F2 bm  F3 next");
        }
    }

    endwin();
    return 0;
#endif
}
