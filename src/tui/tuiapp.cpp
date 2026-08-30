/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuiapp.h"

#include "core/fileservice.h"
#include "encodingutils.h"
#include "managers/sessionmanager.h"
#include "managers/settingsmanager.h"
#include "tui/tuibuffer.h"
#include "tui/tuifiletree.h"
#include "tui/tuifindinfiles.h"
#include "tui/tuihighlighter.h"
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
    TuiFileTree fileTree;

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
    {
        tabs.addBuffer(TuiBuffer(QString(), QString(), QStringLiteral("UTF-8")));
    }

    // Session restore (headless) if no positional files and not --no-session
    {
        bool noSession = parser.isSet(QStringLiteral("no-session"));
        if (positionalFiles.isEmpty() && !noSession)
        {
            SessionManager sm;
            QStringList sFiles = sm.sessionFiles();
            if (!sFiles.isEmpty())
            {
                // Check if tabs is just single untitled empty buffer, then replace
                bool isUntitledEmpty = (tabs.count() == 1 && tabs.bufferAt(0) && tabs.bufferAt(0)->filePath().isEmpty()
                                        && tabs.bufferAt(0)->text().isEmpty());
                if (isUntitledEmpty)
                {
                    TuiTabModel newTabs;
                    auto bookmarks = sm.loadSessionBookmarks();
                    QStringList sPinned = sm.loadSessionPinnedFiles();
                    QSet<QString> pinnedSet;
                    for (const QString& p : sPinned) pinnedSet.insert(p);
                    int loaded = 0;
                    for (const QString& fp : sFiles)
                    {
                        QFileInfo fi(fp);
                        if (!fi.exists() || !fi.isFile())
                            continue;
                        FileLoadResult res = FileService::load(fp);
                        if (res.ok)
                        {
                            TuiBuffer buf(fp, res.text, res.encoding);
                            auto it = bookmarks.find(fp);
                            if (it != bookmarks.end())
                                buf.setBookmarks(it.value());
                            newTabs.addBuffer(buf);
                            ++loaded;
                        }
                    }
                    if (loaded > 0)
                    {
                        tabs = newTabs;
                        int active = sm.sessionActiveIndex();
                        tabs.setCurrentIndex(qBound(0, active, tabs.count() - 1));
                        tabs.setPinnedFiles(pinnedSet);
                        QString proj = sm.sessionProjectPath();
                        if (!proj.isEmpty() && QDir(proj).exists())
                            fileTree.setRootPath(proj);
                    }
                }
            }
        }
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
        init_pair(4, COLOR_BLUE, -1); // keyword
        init_pair(5, COLOR_RED, -1); // string
        init_pair(6, COLOR_GREEN, -1); // comment
        init_pair(7, COLOR_MAGENTA, -1); // number
        init_pair(8, COLOR_CYAN, -1); // preprocessor
    }

    int scrollTop = 0;
    int wrapScrollTop = 0;
    bool wordWrap = SettingsManager::instance().wordWrap();
    bool syntaxEnabled = true;
    TuiHighlighter::setEnabled(syntaxEnabled);
    qint64 lastTreePollMs = 0;
    QDateTime lastTreeDirMTime;
    QString statusMsg = QStringLiteral("Ctrl+Q quit  Ctrl+S save  Ctrl+E tree  Ctrl+F find  F1 help");
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
    bool fileTreeVisible = fileTree.hasRoot();
    bool fileTreeFocused = false;
    int fileTreeScroll = 0;
    bool fileTreeFilterMode = false;
    QString fileTreeFilterInput;

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

        // Poll for external file changes (all buffers)
        for (int ti = 0; ti < tabs.count(); ++ti)
        {
            TuiBuffer* b = tabs.bufferAt(ti);
            if (!b || b->filePath().isEmpty())
                continue;
            QFileInfo fi(b->filePath());
            if (!fi.exists())
                continue;
            QDateTime curMod = fi.lastModified();
            if (!fileMtimes.contains(b->filePath()))
            {
                fileMtimes[b->filePath()] = curMod;
                continue;
            }
            QDateTime lastMod = fileMtimes.value(b->filePath());
            if (!curMod.isValid() || !lastMod.isValid() || curMod == lastMod)
                continue;
            if (!b->isModified())
            {
                FileLoadResult res = FileService::load(b->filePath());
                if (res.ok)
                {
                    bool isCur = (b == cur);
                    b->setText(res.text);
                    b->setEncoding(res.encoding);
                    b->setModified(false);
                    fileMtimes[b->filePath()] = curMod;
                    if (isCur)
                        statusMsg = QStringLiteral("File reloaded (external change): %1").arg(b->filePath());
                }
                else
                {
                    fileMtimes[b->filePath()] = curMod;
                    if (b == cur)
                        statusMsg = QStringLiteral("External change detected but reload failed");
                }
            }
            else
            {
                fileMtimes[b->filePath()] = curMod;
                if (b == cur)
                    statusMsg = QStringLiteral("WARNING: File changed on disk — :e! to reload, :w to overwrite");
            }
        }

        // Directory inotify for fileTree - throttled poll
        if (fileTreeVisible && fileTree.hasRoot())
        {
            qint64 now = QDateTime::currentMSecsSinceEpoch();
            if (now - lastTreePollMs > 1500)
            {
                lastTreePollMs = now;
                QFileInfo dirFi(fileTree.rootPath());
                QDateTime curDirMod = dirFi.exists() ? dirFi.lastModified() : QDateTime();
                bool needRefresh = false;
                if (!lastTreeDirMTime.isValid() || curDirMod != lastTreeDirMTime)
                    needRefresh = true;
                else
                {
                    // Periodic forced refresh for nested changes (every 5s)
                    static qint64 lastForcedRefresh = 0;
                    if (now - lastForcedRefresh > 5000)
                    {
                        lastForcedRefresh = now;
                        needRefresh = true;
                    }
                }
                if (needRefresh)
                {
                    int beforeCount = fileTree.visibleNodes().size();
                    fileTree.refresh();
                    int afterCount = fileTree.visibleNodes().size();
                    if (beforeCount != afterCount)
                        statusMsg = QStringLiteral("Tree refreshed (%1 items)").arg(afterCount);
                    lastTreeDirMTime = curDirMod;
                }
            }
        }

        int rows, cols;
        getmaxyx(stdscr, rows, cols);
        int tabBarH = 1;
        int statusBarH = 2;
        int editorH = rows - tabBarH - statusBarH;
        if (editorH < 1)
            editorH = 1;

        // Keep cursor in view (non-wrap)
        if (!wordWrap)
        {
            if (cur->cursorLine() < scrollTop)
                scrollTop = cur->cursorLine();
            if (cur->cursorLine() >= scrollTop + editorH)
                scrollTop = cur->cursorLine() - editorH + 1;
        }

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

        const int treeWidth = 30;
        const int editorXOffset = fileTreeVisible ? treeWidth + 1 : 0;
        const int textXGlobal = editorXOffset + 7;
        const int availGlobal = (textXGlobal < cols) ? cols - textXGlobal : 0;
        QList<TuiFileNode> treeNodes;
        if (fileTreeVisible)
        {
            treeNodes = fileTree.visibleNodes();
            if (fileTree.cursorIndex() < fileTreeScroll)
                fileTreeScroll = fileTree.cursorIndex();
            if (fileTree.cursorIndex() >= fileTreeScroll + editorH)
                fileTreeScroll = fileTree.cursorIndex() - editorH + 1;
            if (fileTreeScroll < 0)
                fileTreeScroll = 0;
        }
        struct WrapInfo
        {
            int line;
            int startCol;
            int len;
        };
        QList<WrapInfo> wrapInfos;
        int cursorVisualRow = -1;
        if (wordWrap && availGlobal > 0)
        {
            for (int i = 0; i < cur->lineCount(); ++i)
            {
                QString line = cur->lines().at(i);
                if (line.isEmpty())
                {
                    wrapInfos.append({i, 0, 0});
                    if (i == cur->cursorLine() && cur->cursorCol() == 0)
                        cursorVisualRow = wrapInfos.size() - 1;
                }
                else
                {
                    for (int off = 0; off < line.size(); off += availGlobal)
                    {
                        int len = qMin(availGlobal, line.size() - off);
                        wrapInfos.append({i, off, len});
                        if (i == cur->cursorLine() && cur->cursorCol() >= off && cur->cursorCol() < off + len)
                            cursorVisualRow = wrapInfos.size() - 1;
                    }
                    if (i == cur->cursorLine() && cur->cursorCol() == line.size())
                    {
                        // cursor at end of line -> last segment
                        for (int v = wrapInfos.size() - 1; v >= 0; --v)
                        {
                            if (wrapInfos[v].line == i)
                            {
                                cursorVisualRow = v;
                                break;
                            }
                        }
                    }
                }
            }
            if (cursorVisualRow == -1 && !wrapInfos.isEmpty())
                cursorVisualRow = 0;
            if (cursorVisualRow < wrapScrollTop)
                wrapScrollTop = cursorVisualRow;
            if (cursorVisualRow >= wrapScrollTop + editorH)
                wrapScrollTop = cursorVisualRow - editorH + 1;
            if (wrapScrollTop < 0)
                wrapScrollTop = 0;
            if (wrapScrollTop >= wrapInfos.size())
                wrapScrollTop = qMax(0, wrapInfos.size() - editorH);
        }

        auto drawSyntaxVisible = [&](int y, int x, const QString& visible, int offsetInLine, const QList<HighlightSegment>& segs) {
            if (!syntaxEnabled || !hasColors || segs.isEmpty())
            {
                mvaddnstr(y, x, visible.toUtf8().constData(), visible.toUtf8().size());
                return;
            }
            // Per-char color application
            for (int i = 0; i < visible.size(); ++i)
            {
                int pos = offsetInLine + i;
                HighlightKind kind = HighlightKind::Normal;
                for (const auto& s : segs)
                {
                    if (pos >= s.start && pos < s.start + s.length)
                    {
                        kind = s.kind;
                        break;
                    }
                }
                int pair = 0;
                switch (kind)
                {
                case HighlightKind::Keyword: pair = 4; break;
                case HighlightKind::String: pair = 5; break;
                case HighlightKind::Comment: pair = 6; break;
                case HighlightKind::Number: pair = 7; break;
                case HighlightKind::Preprocessor: pair = 8; break;
                default: pair = 0; break;
                }
                if (pair)
                    attron(COLOR_PAIR(pair));
                // Handle unicode: take single QChar and convert to utf8
                QString chStr = visible.mid(i, 1);
                QByteArray utf = chStr.toUtf8();
                mvaddnstr(y, x + i, utf.constData(), utf.size());
                if (pair)
                    attroff(COLOR_PAIR(pair));
            }
        };

        // Draw editor
        for (int r = 0; r < editorH; ++r)
        {
            int lineIdx = scrollTop + r;
            int y = tabBarH + r;
            for (int c = 0; c < cols; ++c)
                mvaddch(y, c, ' ');

            // File tree pane
            if (fileTreeVisible)
            {
                int treeRow = fileTreeScroll + r;
                if (treeRow < treeNodes.size())
                {
                    const TuiFileNode& node = treeNodes[treeRow];
                    bool isSelected = (treeRow == fileTree.cursorIndex());
                    if (isSelected)
                        attron(A_REVERSE);
                    if (fileTreeFocused && isSelected)
                        attron(A_BOLD);
                    QString indent = QString(node.depth * 2, QLatin1Char(' '));
                    QString prefix = node.isDir ? (node.expanded ? QStringLiteral("- ") : QStringLiteral("+ ")) : QStringLiteral("  ");
                    QString label = indent + prefix + node.name + (node.isDir ? QStringLiteral("/") : QString());
                    mvaddnstr(y, 0, label.toUtf8().constData(), qMin(label.toUtf8().size(), treeWidth));
                    if (isSelected)
                        attroff(A_REVERSE);
                    if (fileTreeFocused && isSelected)
                        attroff(A_BOLD);
                }
                mvaddch(y, treeWidth, ACS_VLINE);
            }

            if (wordWrap && availGlobal > 0)
            {
                int vIdx = wrapScrollTop + r;
                if (vIdx < 0 || vIdx >= wrapInfos.size())
                    continue;
                WrapInfo wi = wrapInfos[vIdx];
                int wLineIdx = wi.line;
                QString wLine = cur->lines().at(wLineIdx);
                bool isFirstSeg = (wi.startCol == 0);
                bool isBm = isFirstSeg && cur->hasBookmark(wLineIdx);
                QString gutter = (hasColors && isBm) ? QString::fromUtf8("\xE2\x98\x85 ") : QStringLiteral("  ");
                QString lnStr = isFirstSeg ? QStringLiteral("%1 ").arg(wLineIdx + 1, 4) : QStringLiteral("     ");
                if (hasColors && isFirstSeg)
                    attron(COLOR_PAIR(2));
                mvaddnstr(y, editorXOffset, lnStr.toUtf8().constData(), qMin(lnStr.toUtf8().size(), cols - editorXOffset));
                if (hasColors && isFirstSeg)
                    attroff(COLOR_PAIR(2));
                int gutterX = editorXOffset + 5;
                if (hasColors && isBm)
                    attron(COLOR_PAIR(3));
                mvaddnstr(y, gutterX, gutter.toUtf8().constData(), gutter.size());
                if (hasColors && isBm)
                    attroff(COLOR_PAIR(3));
                int textX = editorXOffset + 7;
                QString visible = wLine.mid(wi.startCol, wi.len);
                int selStart = -1, selEnd = -1;
                if (cur->hasSelection())
                {
                    int aLine = cur->selectionAnchorLine();
                    int aCol = cur->selectionAnchorCol();
                    int cLine = cur->cursorLine();
                    int cCol = cur->cursorCol();
                    if (aLine > cLine || (aLine == cLine && aCol > cCol)) { std::swap(aLine,cLine); std::swap(aCol,cCol); }
                    if (wLineIdx >= aLine && wLineIdx <= cLine)
                    {
                        if (aLine == cLine) { selStart = aCol - wi.startCol; selEnd = cCol - wi.startCol; }
                        else if (wLineIdx == aLine) { selStart = aCol - wi.startCol; selEnd = wi.len; if (selStart < 0) selStart = 0; }
                        else if (wLineIdx == cLine) { selStart = 0; selEnd = cCol - wi.startCol; }
                        else { selStart = 0; selEnd = wi.len; }
                        selStart = qBound(0, selStart, wi.len);
                        selEnd = qBound(0, selEnd, wi.len);
                        if (selStart == selEnd) selStart = -1;
                    }
                }
                bool searchInSeg = false;
                int searchStart = -1, searchLen = 0;
                if (lastSearch.found && lastSearch.line == wLineIdx)
                {
                    int s = lastSearch.column;
                    int e = s + lastSearch.length;
                    int segS = wi.startCol;
                    int segE = wi.startCol + wi.len;
                    if (!(e <= segS || s >= segE))
                    {
                        searchInSeg = true;
                        searchStart = qMax(0, s - segS);
                        searchLen = qMin(e, segE) - qMax(s, segS);
                    }
                }
                if (selStart != -1)
                {
                    mvaddnstr(y, textX, visible.left(selStart).toUtf8().constData(), selStart);
                    attron(A_REVERSE);
                    mvaddnstr(y, textX + selStart, visible.mid(selStart, selEnd - selStart).toUtf8().constData(), selEnd - selStart);
                    attroff(A_REVERSE);
                    if (selEnd < visible.size())
                        mvaddnstr(y, textX + selEnd, visible.mid(selEnd).toUtf8().constData(), visible.size() - selEnd);
                }
                else if (searchInSeg)
                {
                    mvaddnstr(y, textX, visible.left(searchStart).toUtf8().constData(), searchStart);
                    attron(A_REVERSE);
                    mvaddnstr(y, textX + searchStart, visible.mid(searchStart, searchLen).toUtf8().constData(), searchLen);
                    attroff(A_REVERSE);
                    if (searchStart + searchLen < visible.size())
                        mvaddnstr(y, textX + searchStart + searchLen, visible.mid(searchStart + searchLen).toUtf8().constData(), visible.size() - searchStart - searchLen);
                }
                else
                {
                    auto segs = TuiHighlighter::highlightLine(wLine, TuiHighlighter::currentLanguage(cur->filePath()));
                    drawSyntaxVisible(y, textX, visible, wi.startCol, segs);
                }
                if (vIdx == cursorVisualRow && !fileTreeFocused)
                {
                    int curX = textX + (cur->cursorCol() - wi.startCol);
                    if (curX >= textX && curX < cols)
                        move(y, curX);
                }
                continue;
            }

            if (lineIdx >= cur->lineCount())
            {
                if (lineIdx == cur->cursorLine() && !fileTreeFocused)
                {
                    int curX = editorXOffset + 7 + (cur->cursorCol());
                    if (curX >= editorXOffset && curX < cols)
                        move(y, curX);
                }
                continue;
            }

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
            mvaddnstr(y, editorXOffset, lnStr.toUtf8().constData(), qMin(lnStr.toUtf8().size(), cols - editorXOffset));
            if (hasColors)
                attroff(COLOR_PAIR(2));

            int gutterX = editorXOffset + 5;
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
                auto segs = TuiHighlighter::highlightLine(line, TuiHighlighter::currentLanguage(cur->filePath()));
                drawSyntaxVisible(y, textX, visible, hScroll, segs);
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
        else if (fileTreeFilterMode)
            msg = QStringLiteral("Filter: ") + fileTreeFilterInput;
        else if (saveAsMode)
            msg = QStringLiteral("Save As: ") + saveAsInput;
        else if (gotoMode)
            msg = QStringLiteral("Go to line: ") + gotoInput;
        else if (commandMode)
            msg = QStringLiteral(":") + commandInput;
        else
            msg = statusMsg;
        if (fileTreeVisible && !findMode && !replaceMode && !fileTreeFilterMode && !saveAsMode && !gotoMode && !commandMode)
            msg = QStringLiteral("[Tree %1] ").arg(fileTreeFocused ? QStringLiteral("FOCUS") : QStringLiteral(" ")) + msg;
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
        else if (fileTreeFilterMode)
            move(msgY, 8 + fileTreeFilterInput.size());
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
                else if (cmd.startsWith(QStringLiteral("cd ")) || cmd == QStringLiteral("cd"))
                {
                    QString arg = cmd == QStringLiteral("cd") ? QDir::currentPath() : cmd.mid(3).trimmed();
                    if (arg.isEmpty())
                        arg = QDir::currentPath();
                    if (!QFileInfo(arg).isAbsolute())
                        arg = QDir::current().absoluteFilePath(arg);
                    QFileInfo fi(arg);
                    if (fi.isDir() && fi.exists())
                    {
                        fileTree.setRootPath(arg);
                        fileTreeVisible = true;
                        fileTreeFocused = true;
                        fileMtimes.clear();
                        // Refresh mtimes for existing tabs? Keep
                        statusMsg = QStringLiteral("Project: %1").arg(arg);
                    }
                    else
                    {
                        statusMsg = QStringLiteral("No such directory: %1").arg(arg);
                    }
                }
                else if (cmd.startsWith(QStringLiteral("set encoding")))
                {
                    QString arg = cmd.mid(QStringLiteral("set encoding").size()).trimmed();
                    if (arg.isEmpty())
                    {
                        QStringList encs;
                        for (const auto& ei : supportedEncodings())
                            encs << ei.displayName;
                        statusMsg = QStringLiteral("Encoding: %1 | Available: %2").arg(cur->encoding(), encs.join(QStringLiteral(", ")));
                    }
                    else
                    {
                        // Validate encoding name
                        bool valid = false;
                        for (const auto& ei : supportedEncodings())
                        {
                            if (ei.displayName.compare(arg, Qt::CaseInsensitive) == 0)
                            {
                                valid = true;
                                arg = ei.displayName;
                                break;
                            }
                        }
                        if (!valid)
                        {
                            statusMsg = QStringLiteral("Unknown encoding: %1").arg(arg);
                        }
                        else
                        {
                            cur->setEncoding(arg);
                            statusMsg = QStringLiteral("Encoding set to %1 (save to apply)").arg(arg);
                        }
                    }
                }
                else if (cmd.startsWith(QStringLiteral("reopen")))
                {
                    QString arg = cmd.mid(QStringLiteral("reopen").size()).trimmed();
                    // Syntax: reopen  or  reopen <encoding>  or  reopen ++enc=<encoding>
                    if (arg.startsWith(QStringLiteral("++enc=")))
                        arg = arg.mid(6).trimmed();
                    if (arg.isEmpty())
                        arg = cur->encoding();
                    FileLoadResult res = FileService::load(cur->filePath().isEmpty() ? QString() : cur->filePath(), arg);
                    if (cur->filePath().isEmpty())
                    {
                        statusMsg = QStringLiteral("No file name");
                    }
                    else if (res.ok)
                    {
                        cur->setText(res.text);
                        cur->setEncoding(res.encoding);
                        cur->setModified(false);
                        fileMtimes[cur->filePath()] = QFileInfo(cur->filePath()).lastModified();
                        statusMsg = QStringLiteral("Reopened with %1").arg(res.encoding);
                    }
                    else
                    {
                        statusMsg = QStringLiteral("Reopen failed: %1").arg(res.error);
                    }
                }
                else if (cmd.startsWith(QStringLiteral("set wrap")))
                {
                    QString arg = cmd.mid(QStringLiteral("set wrap").size()).trimmed().toLower();
                    if (arg.isEmpty())
                    {
                        statusMsg = QStringLiteral("Wrap is %1").arg(wordWrap ? QStringLiteral("on") : QStringLiteral("off"));
                    }
                    else if (arg == QStringLiteral("on") || arg == QStringLiteral("1") || arg == QStringLiteral("true"))
                    {
                        wordWrap = true;
                        SettingsManager::instance().setWordWrap(true);
                        statusMsg = QStringLiteral("Wrap on");
                    }
                    else if (arg == QStringLiteral("off") || arg == QStringLiteral("0") || arg == QStringLiteral("false"))
                    {
                        wordWrap = false;
                        SettingsManager::instance().setWordWrap(false);
                        wrapScrollTop = 0;
                        statusMsg = QStringLiteral("Wrap off");
                    }
                    else
                    {
                        statusMsg = QStringLiteral("Usage: :set wrap [on|off]");
                    }
                }
                else if (cmd == QStringLiteral("syntax on") || cmd == QStringLiteral("set syntax on") || cmd == QStringLiteral("set syntax true"))
                {
                    syntaxEnabled = true;
                    TuiHighlighter::setEnabled(true);
                    statusMsg = QStringLiteral("Syntax on");
                }
                else if (cmd == QStringLiteral("syntax off") || cmd == QStringLiteral("set syntax off") || cmd == QStringLiteral("set syntax false"))
                {
                    syntaxEnabled = false;
                    TuiHighlighter::setEnabled(false);
                    statusMsg = QStringLiteral("Syntax off");
                }
                else if (cmd == QStringLiteral("bn") || cmd == QStringLiteral("bnext"))
                {
                    int nLine;
                    if (cur->nextBookmark(cur->cursorLine(), &nLine))
                    {
                        cur->setCursor(nLine, 0);
                        statusMsg = QStringLiteral("Next bookmark %1").arg(nLine + 1);
                    }
                    else
                        statusMsg = QStringLiteral("No bookmarks");
                }
                else if (cmd == QStringLiteral("bp") || cmd == QStringLiteral("bprev"))
                {
                    int pLine;
                    if (cur->prevBookmark(cur->cursorLine(), &pLine))
                    {
                        cur->setCursor(pLine, 0);
                        statusMsg = QStringLiteral("Prev bookmark %1").arg(pLine + 1);
                    }
                    else
                        statusMsg = QStringLiteral("No bookmarks");
                }
                else if (cmd == QStringLiteral("bc") || cmd == QStringLiteral("bclear"))
                {
                    cur->clearBookmarks();
                    statusMsg = QStringLiteral("Bookmarks cleared");
                }
                else if (cmd.startsWith(QStringLiteral("set searchcase")) || cmd.startsWith(QStringLiteral("set searchwhole")) || cmd.startsWith(QStringLiteral("set searchregex")))
                {
                    // Handled below as find options
                    if (cmd.startsWith(QStringLiteral("set searchcase")))
                    {
                        QString arg = cmd.mid(QStringLiteral("set searchcase").size()).trimmed().toLower();
                        if (arg == QStringLiteral("on") || arg == QStringLiteral("1")) findOpts.caseSensitive = true;
                        else if (arg == QStringLiteral("off") || arg == QStringLiteral("0")) findOpts.caseSensitive = false;
                        statusMsg = QStringLiteral("Search caseSensitive %1").arg(findOpts.caseSensitive ? QStringLiteral("on") : QStringLiteral("off"));
                    }
                    else if (cmd.startsWith(QStringLiteral("set searchwhole")))
                    {
                        QString arg = cmd.mid(QStringLiteral("set searchwhole").size()).trimmed().toLower();
                        if (arg == QStringLiteral("on") || arg == QStringLiteral("1")) findOpts.wholeWords = true;
                        else if (arg == QStringLiteral("off") || arg == QStringLiteral("0")) findOpts.wholeWords = false;
                        statusMsg = QStringLiteral("Search wholeWords %1").arg(findOpts.wholeWords ? QStringLiteral("on") : QStringLiteral("off"));
                    }
                    else if (cmd.startsWith(QStringLiteral("set searchregex")))
                    {
                        QString arg = cmd.mid(QStringLiteral("set searchregex").size()).trimmed().toLower();
                        if (arg == QStringLiteral("on") || arg == QStringLiteral("1")) findOpts.regex = true;
                        else if (arg == QStringLiteral("off") || arg == QStringLiteral("0")) findOpts.regex = false;
                        statusMsg = QStringLiteral("Search regex %1").arg(findOpts.regex ? QStringLiteral("on") : QStringLiteral("off"));
                    }
                }
                else if (cmd.startsWith(QStringLiteral("grep")) || cmd.startsWith(QStringLiteral("findinfiles")))
                {
                    QString raw = cmd;
                    QString pattern;
                    QString dir;
                    QString glob;
                    if (raw.startsWith(QStringLiteral("grep")))
                        raw = raw.mid(4).trimmed();
                    else
                        raw = raw.mid(11).trimmed();
                    // raw may be "pattern [dir] [glob]" - split
                    // Use simple split: first token pattern, rest dir/glob
                    // Allow quoted pattern? Keep simple
                    QStringList tokens = raw.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
                    if (!tokens.isEmpty())
                        pattern = tokens[0];
                    if (tokens.size() >= 2)
                        dir = tokens[1];
                    if (tokens.size() >= 3)
                        glob = tokens[2];
                    if (pattern.isEmpty())
                    {
                        statusMsg = QStringLiteral("Usage: :grep <pattern> [dir] [glob]");
                    }
                    else
                    {
                        QString root = dir.isEmpty() ? (fileTree.hasRoot() ? fileTree.rootPath() : QDir::currentPath()) : dir;
                        if (!QFileInfo(root).isAbsolute())
                            root = QDir::current().absoluteFilePath(root);
                        QList<FindInFilesResult> results = TuiFindInFiles::search(root, pattern, findOpts, glob);
                        if (results.isEmpty())
                        {
                            statusMsg = QStringLiteral("No matches for %1").arg(pattern);
                        }
                        else
                        {
                            QStringList lines;
                            lines << QStringLiteral("Grep: %1 in %2 (%3 matches)").arg(pattern, root).arg(results.size());
                            lines << QStringLiteral("---");
                            for (const auto& r : results)
                                lines << QStringLiteral("%1:%2:%3: %4").arg(r.filePath).arg(r.line + 1).arg(r.column + 1).arg(r.lineText);
                            TuiBuffer resBuf(QStringLiteral("*grep*"), lines.join(QStringLiteral("\n")));
                            resBuf.setReadOnly(true);
                            tabs.addBuffer(resBuf);
                            statusMsg = QStringLiteral("Found %1 matches — Enter on line to jump").arg(results.size());
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
            else if (ch == 9) // Tab completion
            {
                // Command completion
                QString input = commandInput;
                QStringList allCmds = {
                    QStringLiteral("w"), QStringLiteral("wa"), QStringLiteral("wqa"), QStringLiteral("wq"),
                    QStringLiteral("q"), QStringLiteral("q!"), QStringLiteral("e "), QStringLiteral("e!"),
                    QStringLiteral("cd "), QStringLiteral("set encoding "), QStringLiteral("set wrap "),
                    QStringLiteral("set syntax on"), QStringLiteral("set syntax off"), QStringLiteral("syntax on"), QStringLiteral("syntax off"),
                    QStringLiteral("set searchcase "), QStringLiteral("set searchwhole "), QStringLiteral("set searchregex "),
                    QStringLiteral("reopen"), QStringLiteral("grep "), QStringLiteral("findinfiles "),
                    QStringLiteral("s/"), QStringLiteral("%s/"), QStringLiteral("bn"), QStringLiteral("bnext"),
                    QStringLiteral("bp"), QStringLiteral("bprev"), QStringLiteral("bc"), QStringLiteral("bclear")
                };
                // Handle set encoding completion with encodings list
                if (input.startsWith(QStringLiteral("set encoding ")))
                {
                    QString prefix = input.mid(QStringLiteral("set encoding ").size());
                    QStringList encs;
                    for (const auto& ei : supportedEncodings())
                        encs << ei.displayName;
                    QStringList cands;
                    for (const QString& e : encs)
                        if (e.startsWith(prefix, Qt::CaseInsensitive))
                            cands << e;
                    if (cands.size() == 1)
                        commandInput = QStringLiteral("set encoding ") + cands.first();
                    else if (cands.size() > 1)
                    {
                        QString common = cands.first();
                        for (int i=1; i<cands.size(); ++i)
                        {
                            int j=0;
                            while (j < common.size() && j < cands[i].size() && common[j].toLower() == cands[i][j].toLower()) ++j;
                            common = common.left(j);
                        }
                        if (common.size() > prefix.size())
                            commandInput = QStringLiteral("set encoding ") + common;
                        statusMsg = QStringLiteral("Encodings: %1").arg(cands.join(QStringLiteral(", ")));
                    }
                }
                else
                {
                    QStringList cands;
                    for (const QString& c : allCmds)
                        if (c.startsWith(input))
                            cands << c;
                    if (cands.size() == 1)
                    {
                        commandInput = cands.first();
                        // Add space if not already and not ending with /
                        if (!commandInput.endsWith(QLatin1Char(' ')) && !commandInput.endsWith(QLatin1Char('/')))
                            commandInput += QLatin1Char(' ');
                    }
                    else if (cands.size() > 1)
                    {
                        QString common = cands.first();
                        for (int i=1; i<cands.size(); ++i)
                        {
                            int j=0;
                            while (j < common.size() && j < cands[i].size() && common[j] == cands[i][j]) ++j;
                            common = common.left(j);
                        }
                        if (common.size() > input.size())
                            commandInput = common;
                        statusMsg = QStringLiteral("Candidates: %1").arg(cands.join(QStringLiteral(", ")));
                    }
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

        if (fileTreeFilterMode)
        {
            if (ch == 27)
            {
                fileTreeFilterMode = false;
                fileTreeFilterInput.clear();
                fileTree.setFilter(QString());
                statusMsg = QStringLiteral("Filter cleared");
            }
            else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
            {
                fileTree.setFilter(fileTreeFilterInput);
                fileTreeFilterMode = false;
                statusMsg = fileTreeFilterInput.isEmpty() ? QStringLiteral("Filter cleared") : QStringLiteral("Filter: %1").arg(fileTreeFilterInput);
            }
            else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
            {
                if (!fileTreeFilterInput.isEmpty())
                    fileTreeFilterInput.chop(1);
                else
                    fileTreeFilterMode = false;
                fileTree.setFilter(fileTreeFilterInput);
            }
            else if (ch >= 32 && ch < 127)
            {
                fileTreeFilterInput.append(QChar(ch));
                fileTree.setFilter(fileTreeFilterInput);
            }
            continue;
        }

        // File tree focused navigation
        if (fileTreeVisible && fileTreeFocused)
        {
            if (ch == 27) // Esc back to editor
            {
                fileTreeFocused = false;
                statusMsg = QStringLiteral("Editor focus");
                continue;
            }
            else if (ch == 5) // Ctrl+E toggle tree
            {
                fileTreeVisible = false;
                fileTreeFocused = false;
                statusMsg = QStringLiteral("File tree hidden");
                continue;
            }
            else if (ch == KEY_UP)
            {
                fileTree.moveCursor(-1);
                continue;
            }
            else if (ch == KEY_DOWN)
            {
                fileTree.moveCursor(1);
                continue;
            }
            else if (ch == KEY_LEFT)
            {
                const TuiFileNode* n = fileTree.currentNode();
                if (n && n->isDir && n->expanded)
                    fileTree.setExpanded(n->absolutePath, false);
                else if (n)
                {
                    // Move to parent
                    QFileInfo fi(n->absolutePath);
                    QString parent = fi.absolutePath();
                    auto nodes = fileTree.visibleNodes();
                    for (int i = fileTree.cursorIndex() - 1; i >= 0; --i)
                    {
                        const TuiFileNode* cand = fileTree.nodeAt(i);
                        if (cand && cand->absolutePath == parent)
                        {
                            fileTree.setCursor(i);
                            break;
                        }
                    }
                }
                continue;
            }
            else if (ch == KEY_RIGHT)
            {
                const TuiFileNode* n = fileTree.currentNode();
                if (n && n->isDir && !n->expanded)
                    fileTree.setExpanded(n->absolutePath, true);
                continue;
            }
            else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
            {
                const TuiFileNode* n = fileTree.currentNode();
                if (!n)
                    continue;
                if (n->isDir)
                {
                    fileTree.toggleExpanded(n->absolutePath);
                }
                else
                {
                    FileLoadResult res = FileService::load(n->absolutePath);
                    if (res.ok)
                    {
                        TuiBuffer buf(n->absolutePath, res.text, res.encoding);
                        tabs.addBuffer(buf);
                        fileMtimes[n->absolutePath] = QFileInfo(n->absolutePath).lastModified();
                        statusMsg = QStringLiteral("Opened %1").arg(n->absolutePath);
                    }
                    else
                    {
                        statusMsg = QStringLiteral("Cannot open %1").arg(n->absolutePath);
                    }
                    fileTreeFocused = false;
                }
                continue;
            }
            else if (ch == '/' || ch == 47)
            {
                fileTreeFilterMode = true;
                fileTreeFilterInput = fileTree.filter();
                continue;
            }
            else if (ch == KEY_NPAGE)
            {
                fileTree.moveCursor(10);
                continue;
            }
            else if (ch == KEY_PPAGE)
            {
                fileTree.moveCursor(-10);
                continue;
            }
            // Other keys when tree focused: ignore and stay
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
        else if (ch == 5) // Ctrl+E file tree
        {
            if (!fileTreeVisible)
            {
                if (!fileTree.hasRoot())
                    fileTree.setRootPath(QDir::currentPath());
                fileTreeVisible = true;
                fileTreeFocused = true;
                statusMsg = QStringLiteral("File tree — arrows nav, Enter open/toggle, / filter, Esc editor");
            }
            else if (fileTreeFocused)
            {
                fileTreeFocused = false;
                statusMsg = QStringLiteral("Editor focus");
            }
            else
            {
                fileTreeFocused = true;
                statusMsg = QStringLiteral("File tree focus");
            }
        }
        else if (ch == 11) // Ctrl+K encoding picker
        {
            commandMode = true;
            commandInput = QStringLiteral("set encoding ");
            statusMsg = QStringLiteral("Enter encoding (e.g., UTF-8, UTF-16LE, ISO-8859-1)");
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
        else if (ch == KEY_F(14)) // Shift+F2 next
        {
            int nLine;
            if (cur->nextBookmark(cur->cursorLine(), &nLine))
            {
                cur->setCursor(nLine, 0);
                statusMsg = QStringLiteral("Next bookmark %1").arg(nLine + 1);
            }
            else
                statusMsg = QStringLiteral("No bookmarks");
        }
        else if (ch == KEY_F(26)) // Ctrl+F2 prev (also Shift+F2 fallback)
        {
            int pLine;
            if (cur->prevBookmark(cur->cursorLine(), &pLine))
            {
                cur->setCursor(pLine, 0);
                statusMsg = QStringLiteral("Prev bookmark %1").arg(pLine + 1);
            }
            else
                statusMsg = QStringLiteral("No bookmarks");
        }
        else if (ch == KEY_F(38)) // Ctrl+Shift+F2 clear
        {
            cur->clearBookmarks();
            statusMsg = QStringLiteral("Bookmarks cleared");
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
        else if (cur->filePath() == QStringLiteral("*grep*") && (ch == KEY_ENTER || ch == 10 || ch == 13))
        {
            QString line = cur->lines().at(cur->cursorLine());
            QRegularExpression re(QStringLiteral("^(.*?):(\\d+):(\\d+):"));
            auto m = re.match(line);
            if (m.hasMatch())
            {
                QString fp = m.captured(1);
                int l = m.captured(2).toInt() - 1;
                int c = m.captured(3).toInt() - 1;
                FileLoadResult res = FileService::load(fp);
                if (res.ok)
                {
                    int idx = tabs.findByFilePath(fp);
                    TuiBuffer* target = nullptr;
                    if (idx != -1)
                    {
                        tabs.setCurrentIndex(idx);
                        target = tabs.currentBuffer();
                        // Refresh content if needed
                        target->setText(res.text);
                        target->setEncoding(res.encoding);
                    }
                    else
                    {
                        TuiBuffer buf(fp, res.text, res.encoding);
                        tabs.addBuffer(buf);
                        target = tabs.currentBuffer();
                    }
                    fileMtimes[fp] = QFileInfo(fp).lastModified();
                    target->setCursor(l, c);
                    statusMsg = QStringLiteral("Jumped to %1:%2").arg(fp).arg(l + 1);
                }
                else
                {
                    statusMsg = QStringLiteral("Cannot open %1").arg(fp);
                }
            }
            else
            {
                statusMsg = QStringLiteral("No file on this line");
            }
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
            statusMsg = QStringLiteral("Help: Ctrl+Q quit  Ctrl+S save  Ctrl+O saveAs  Ctrl+G goto  Ctrl+H replace  :s/old/new/g :%%s/old/new/g  :w/:wa/:q/:e/:cd/:set encoding/:reopen/:set wrap/:set syntax  Ctrl+Z undo  Ctrl+Y redo  Ctrl+A selAll  Ctrl+X/C/V  Shift+Arrows  Ctrl+F find  F2 bm/bnext/bprev/bclear  F3 next  Ctrl+E tree  Ctrl+K enc  / filter  F4 wrap  F5 syntax");
        }
        else if (ch == KEY_F(4))
        {
            wordWrap = !wordWrap;
            SettingsManager::instance().setWordWrap(wordWrap);
            wrapScrollTop = 0;
            statusMsg = wordWrap ? QStringLiteral("Wrap on") : QStringLiteral("Wrap off");
        }
        else if (ch == KEY_F(5))
        {
            syntaxEnabled = !syntaxEnabled;
            TuiHighlighter::setEnabled(syntaxEnabled);
            statusMsg = syntaxEnabled ? QStringLiteral("Syntax on") : QStringLiteral("Syntax off");
        }
    }

    // Session save (headless)
    if (!parser.isSet(QStringLiteral("no-session")))
    {
        SessionManager sm;
        QStringList files = tabs.allFilePaths();
        int active = tabs.currentIndex();
        QString proj = fileTree.hasRoot() ? fileTree.rootPath() : QString();
        sm.saveSessionData(files, active, proj, QDir::currentPath());
        QHash<QString, QList<int>> bms;
        for (int i = 0; i < tabs.count(); ++i)
        {
            const TuiBuffer* b = tabs.bufferAt(i);
            if (b && !b->filePath().isEmpty() && !b->bookmarks().isEmpty())
                bms.insert(b->filePath(), b->bookmarks());
        }
        sm.saveSessionBookmarks(bms);
        QStringList pinned = tabs.pinnedFiles().values();
        sm.saveSessionPinnedFiles(pinned);
    }

    endwin();
    return 0;
#endif
}
