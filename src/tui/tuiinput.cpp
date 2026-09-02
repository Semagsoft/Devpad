/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuiinput.h"

#include "core/fileservice.h"
#include "managers/settingsmanager.h"
#include "tui/tuicommand.h"
#include "tui/tuihighlighter.h"
#include "tui/tuisearchengine.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#ifdef BUILD_TUI
#include <ncurses.h>
#else
// Fallback key definitions when ncurses not available (non-TUI build)
#ifndef KEY_UP
#define KEY_UP 259
#define KEY_DOWN 258
#define KEY_LEFT 260
#define KEY_RIGHT 261
#define KEY_HOME 262
#define KEY_END 360
#define KEY_NPAGE 338
#define KEY_PPAGE 339
#define KEY_DC 330
#define KEY_BACKSPACE 263
#define KEY_ENTER 343
#define KEY_SLEFT 393
#define KEY_SRIGHT 402
#define KEY_SR 390
#define KEY_SF 391
#define KEY_SHOME 391
#define KEY_SEND 392
#define KEY_F(n) (264 + (n))
#ifndef KEY_SLEFT
#define KEY_SLEFT 393
#endif
#endif
#endif

#ifdef BUILD_TUI
static bool saveCurrentHelper(TuiBuffer* buf, QString& statusMsg, QHash<QString, QDateTime>& fileMtimes)
{
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
        statusMsg = err;
    return ok;
}
#endif

static bool saveAsHelper(TuiBuffer* buf, const QString& newPath, QString& statusMsg, QHash<QString, QDateTime>& fileMtimes)
{
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
        statusMsg = err;
    return ok;
}

bool TuiInput::handleInputModes(int ch, TuiTabModel& tabs, TuiBuffer* cur, TuiFileTree& fileTree, TuiInputState& st,
                                QHash<QString, QDateTime>& fileMtimes)
{
    Q_UNUSED(tabs);
    Q_UNUSED(fileMtimes);
    if (st.replaceMode)
    {
        if (ch == 27)
        {
            st.replaceMode = false;
            st.replacePhase = 0;
            st.replaceFindInput.clear();
            st.replaceInput.clear();
            st.statusMsg = QStringLiteral("Replace cancelled");
        }
        else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
        {
            if (st.replacePhase == 0)
                st.replacePhase = 1;
            else
            {
                QString find = st.replaceFindInput;
                QString repl = st.replaceInput;
                st.replaceMode = false;
                st.replacePhase = 0;
                if (find.isEmpty())
                    st.statusMsg = QStringLiteral("Find string empty");
                else if (cur)
                {
                    auto rr = cur->replaceNext(find, repl, st.findOpts, true);
                    if (rr.found)
                    {
                        st.lastSearch = {true, rr.line, rr.column, rr.length};
                        st.statusMsg = QStringLiteral("Replaced 1 at %1:%2").arg(rr.line + 1).arg(rr.column + 1);
                    }
                    else
                    {
                        st.statusMsg = QStringLiteral("Not found: %1").arg(find);
                        st.lastSearch = {};
                    }
                }
                st.replaceFindInput.clear();
                st.replaceInput.clear();
            }
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
        {
            if (st.replacePhase == 0)
            {
                if (!st.replaceFindInput.isEmpty())
                    st.replaceFindInput.chop(1);
            }
            else if (!st.replaceInput.isEmpty())
                st.replaceInput.chop(1);
        }
        else if (ch >= 32 && ch < 127)
        {
            if (st.replacePhase == 0)
                st.replaceFindInput.append(QChar(ch));
            else
                st.replaceInput.append(QChar(ch));
        }
        return true;
    }

    if (st.findMode)
    {
        if (ch == 27)
        {
            st.findMode = false;
            st.findInput.clear();
        }
        else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
        {
            st.findQuery = st.findInput;
            st.findMode = false;
            if (!st.findQuery.isEmpty() && cur)
            {
                SearchResult r =
                    TuiSearchEngine::findNext(cur->lines(), st.findQuery, st.findOpts, cur->cursorLine(), cur->cursorCol() + 1, true, true);
                if (r.found)
                {
                    cur->setCursor(r.line, r.column);
                    st.lastSearch = r;
                    st.statusMsg = QStringLiteral("Found at %1:%2").arg(r.line + 1).arg(r.column + 1);
                }
                else
                {
                    st.statusMsg = QStringLiteral("Not found: %1").arg(st.findQuery);
                    st.lastSearch = {};
                }
            }
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
        {
            if (!st.findInput.isEmpty())
                st.findInput.chop(1);
        }
        else if (ch >= 32 && ch < 127)
            st.findInput.append(QChar(ch));
        return true;
    }

    if (st.saveAsMode)
    {
        if (ch == 27)
        {
            st.saveAsMode = false;
            st.saveAsInput.clear();
            st.statusMsg = QStringLiteral("Save cancelled");
        }
        else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
        {
            QString path = st.saveAsInput.trimmed();
            if (!path.isEmpty())
            {
                if (!QFileInfo(path).isAbsolute())
                    path = QDir::current().absoluteFilePath(path);
                if (cur)
                    saveAsHelper(cur, path, st.statusMsg, st.fileMtimes);
            }
            st.saveAsMode = false;
            st.saveAsInput.clear();
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
        {
            if (!st.saveAsInput.isEmpty())
                st.saveAsInput.chop(1);
        }
        else if (ch >= 32 && ch < 127)
            st.saveAsInput.append(QChar(ch));
        return true;
    }

    if (st.gotoMode)
    {
        if (ch == 27)
        {
            st.gotoMode = false;
            st.gotoInput.clear();
        }
        else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
        {
            bool ok = false;
            int line = st.gotoInput.toInt(&ok);
            if (ok && cur && line >= 1 && line <= cur->lineCount())
            {
                cur->setCursor(line - 1, 0);
                st.statusMsg = QStringLiteral("Go to line %1").arg(line);
            }
            else
                st.statusMsg = QStringLiteral("Invalid line: %1").arg(st.gotoInput);
            st.gotoMode = false;
            st.gotoInput.clear();
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
        {
            if (!st.gotoInput.isEmpty())
                st.gotoInput.chop(1);
        }
        else if (ch >= 48 && ch <= 57)
            st.gotoInput.append(QChar(ch));
        return true;
    }

    if (st.commandMode)
    {
        if (ch == 27)
        {
            st.commandMode = false;
            st.commandInput.clear();
        }
        else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
        {
            QString cmd = st.commandInput.trimmed();
            st.commandMode = false;
            st.commandInput.clear();
            bool saveAsReq = false;
            auto res = TuiCommand::dispatch(cmd, tabs, cur, fileTree, st.view, st.statusMsg, st.lastSearch, st.findOpts, fileMtimes, saveAsReq);
            if (saveAsReq)
            {
                st.saveAsMode = true;
                st.saveAsInput.clear();
            }
            if (res == TuiCommand::Result::Quit)
                st.statusMsg = QStringLiteral("__QUIT__");
        }
        else if (ch == 9) // Tab completion
        {
            TuiCommand::complete(st.commandInput, st.statusMsg);
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
        {
            if (!st.commandInput.isEmpty())
                st.commandInput.chop(1);
            else
                st.commandMode = false;
        }
        else if (ch >= 32 && ch < 127)
            st.commandInput.append(QChar(ch));
        return true;
    }

    if (st.fileTreeFilterMode)
    {
        if (ch == 27)
        {
            st.fileTreeFilterMode = false;
            st.fileTreeFilterInput.clear();
            fileTree.setFilter(QString());
            st.statusMsg = QStringLiteral("Filter cleared");
        }
        else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
        {
            fileTree.setFilter(st.fileTreeFilterInput);
            st.fileTreeFilterMode = false;
            st.statusMsg =
                st.fileTreeFilterInput.isEmpty() ? QStringLiteral("Filter cleared") : QStringLiteral("Filter: %1").arg(st.fileTreeFilterInput);
        }
        else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
        {
            if (!st.fileTreeFilterInput.isEmpty())
                st.fileTreeFilterInput.chop(1);
            else
                st.fileTreeFilterMode = false;
            fileTree.setFilter(st.fileTreeFilterInput);
        }
        else if (ch >= 32 && ch < 127)
        {
            st.fileTreeFilterInput.append(QChar(ch));
            fileTree.setFilter(st.fileTreeFilterInput);
        }
        return true;
    }

    return false;
}

bool TuiInput::handleFileTreeInput(int ch, TuiFileTree& fileTree, TuiInputState& st, TuiTabModel& tabs, QHash<QString, QDateTime>& fileMtimes)
{
    if (!st.fileTreeVisible || !st.fileTreeFocused)
        return false;

    if (ch == 27)
    {
        st.fileTreeFocused = false;
        st.statusMsg = QStringLiteral("Editor focus");
        return true;
    }
    else if (ch == CtrlE)
    {
        st.fileTreeVisible = false;
        st.fileTreeFocused = false;
        st.statusMsg = QStringLiteral("File tree hidden");
        return true;
    }
    else if (ch == KEY_UP)
    {
        fileTree.moveCursor(-1);
        return true;
    }
    else if (ch == KEY_DOWN)
    {
        fileTree.moveCursor(1);
        return true;
    }
    else if (ch == KEY_LEFT)
    {
        const TuiFileNode* n = fileTree.currentNode();
        if (n && n->isDir && n->expanded)
            fileTree.setExpanded(n->absolutePath, false);
        else if (n)
        {
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
        return true;
    }
    else if (ch == KEY_RIGHT)
    {
        const TuiFileNode* n = fileTree.currentNode();
        if (n && n->isDir && !n->expanded)
            fileTree.setExpanded(n->absolutePath, true);
        return true;
    }
    else if (ch == '\n' || ch == KEY_ENTER || ch == 10 || ch == 13)
    {
        const TuiFileNode* n = fileTree.currentNode();
        if (!n)
            return true;
        if (n->isDir)
            fileTree.toggleExpanded(n->absolutePath);
        else
        {
            FileLoadResult res = FileService::load(n->absolutePath);
            if (res.ok)
            {
                TuiBuffer buf(n->absolutePath, res.text, res.encoding);
                tabs.addBuffer(buf);
                fileMtimes[n->absolutePath] = QFileInfo(n->absolutePath).lastModified();
                st.statusMsg = QStringLiteral("Opened %1").arg(n->absolutePath);
            }
            else
                st.statusMsg = QStringLiteral("Cannot open %1").arg(n->absolutePath);
            st.fileTreeFocused = false;
        }
        return true;
    }
    else if (ch == '/' || ch == 47)
    {
        st.fileTreeFilterMode = true;
        st.fileTreeFilterInput = fileTree.filter();
        return true;
    }
    else if (ch == KEY_NPAGE)
    {
        fileTree.moveCursor(10);
        return true;
    }
    else if (ch == KEY_PPAGE)
    {
        fileTree.moveCursor(-10);
        return true;
    }
    return true; // when focused, consume all keys
}

bool TuiInput::handleGlobalInput(int ch, TuiTabModel& tabs, TuiBuffer* cur, TuiFileTree& fileTree, TuiInputState& st,
                                 QHash<QString, QDateTime>& fileMtimes, int editorH)
{
    Q_UNUSED(ch);
    Q_UNUSED(tabs);
    Q_UNUSED(fileTree);
    Q_UNUSED(st);
    Q_UNUSED(fileMtimes);
    if (!cur)
    {
        return false;
    }

    // We need ncurses for getch second prompt on Ctrl+Q/Ctrl+W; handle those with direct ncurses calls
#ifdef BUILD_TUI
    if (ch == CtrlQ)
    {
        bool anyModified = false;
        for (int i = 0; i < tabs.count(); ++i)
            if (tabs.bufferAt(i)->isModified())
                anyModified = true;
        if (anyModified)
        {
            st.statusMsg = QStringLiteral("Modified buffers exist. Press Ctrl+Q again to quit without saving, or save with Ctrl+S");
            // Need to refresh before second getch — caller will handle? We do direct ncurses here
            refresh();
            int ch2 = getch();
            if (ch2 != CtrlQ)
            {
                st.statusMsg = QStringLiteral("Quit cancelled");
                if (ch2 == CtrlS)
                    saveCurrentHelper(cur, st.statusMsg, st.fileMtimes);
                return false;
            }
        }
        return true; // signal quit
    }
    else if (ch == CtrlS)
        saveCurrentHelper(cur, st.statusMsg, st.fileMtimes);
    else if (ch == CtrlE)
    {
        if (!st.fileTreeVisible)
        {
            if (!fileTree.hasRoot())
                fileTree.setRootPath(QDir::currentPath());
            st.fileTreeVisible = true;
            st.fileTreeFocused = true;
            st.statusMsg = QStringLiteral("File tree — arrows nav, Enter open/toggle, / filter, Esc editor");
        }
        else if (st.fileTreeFocused)
        {
            st.fileTreeFocused = false;
            st.statusMsg = QStringLiteral("Editor focus");
        }
        else
        {
            st.fileTreeFocused = true;
            st.statusMsg = QStringLiteral("File tree focus");
        }
    }
    else if (ch == CtrlK)
    {
        st.commandMode = true;
        st.commandInput = QStringLiteral("set encoding ");
        st.statusMsg = QStringLiteral("Enter encoding (e.g., UTF-8, UTF-16LE, ISO-8859-1)");
    }
    else if (ch == CtrlO)
    {
        st.saveAsMode = true;
        st.saveAsInput = cur->filePath();
        if (st.saveAsInput.isEmpty())
            st.saveAsInput = QDir::current().absoluteFilePath(QStringLiteral("untitled.txt"));
    }
    else if (ch == CtrlG)
    {
        st.gotoMode = true;
        st.gotoInput.clear();
    }
    else if (ch == CtrlH)
    {
        st.replaceMode = true;
        st.replacePhase = 0;
        st.replaceFindInput.clear();
        st.replaceInput.clear();
    }
    else if (ch == Colon)
    {
        st.commandMode = true;
        st.commandInput.clear();
    }
    else if (ch == CtrlZ)
    {
        if (!cur->undo())
            st.statusMsg = QStringLiteral("Nothing to undo");
        else
            st.statusMsg = QStringLiteral("Undo");
    }
    else if (ch == CtrlY || ch == CtrlR)
    {
        if (!cur->redo())
            st.statusMsg = QStringLiteral("Nothing to redo");
        else
            st.statusMsg = QStringLiteral("Redo");
    }
    else if (ch == CtrlA)
    {
        cur->selectAll();
        st.statusMsg = QStringLiteral("Selected all");
    }
    else if (ch == CtrlC)
    {
        if (cur->hasSelection())
        {
            st.clipboard = cur->selectedText();
            st.statusMsg = QStringLiteral("Copied %1 chars").arg(st.clipboard.size());
        }
        else
            st.statusMsg = QStringLiteral("No selection");
    }
    else if (ch == CtrlX)
    {
        if (cur->hasSelection())
        {
            st.clipboard = cur->selectedText();
            cur->deleteSelection();
            st.statusMsg = QStringLiteral("Cut %1 chars").arg(st.clipboard.size());
        }
        else
            st.statusMsg = QStringLiteral("No selection");
    }
    else if (ch == CtrlV)
    {
        if (!st.clipboard.isEmpty())
        {
            cur->insertText(st.clipboard);
            st.statusMsg = QStringLiteral("Pasted");
        }
    }
    else if (ch == CtrlW)
    {
        if (cur->isModified())
        {
            st.statusMsg = QStringLiteral("Buffer modified. Save? (y/n/c)");
            refresh();
            int a = getch();
            if (a == 'y' || a == 'Y')
                saveCurrentHelper(cur, st.statusMsg, st.fileMtimes);
            else if (a == 'c' || a == 'C' || a == 27)
            {
                st.statusMsg = QStringLiteral("Close cancelled");
                return false;
            }
        }
        tabs.closeCurrent();
        if (tabs.isEmpty())
            tabs.addBuffer(TuiBuffer());
        st.statusMsg = QStringLiteral("Tab closed");
    }
    else if (ch == CtrlN)
    {
        tabs.addBuffer(TuiBuffer());
        st.statusMsg = QStringLiteral("New buffer");
        st.view.scrollTop = 0;
    }
    else if (ch == CtrlF)
    {
        st.findMode = true;
        st.findInput = st.findQuery;
        st.statusMsg = QStringLiteral("Find mode");
    }
    else if (ch == KEY_F(2))
    {
        cur->toggleBookmark(cur->cursorLine());
        st.statusMsg = cur->hasBookmark(cur->cursorLine()) ? QStringLiteral("Bookmark set") : QStringLiteral("Bookmark cleared");
    }
    else if (ch == KEY_F(14))
    {
        int nLine;
        if (cur->nextBookmark(cur->cursorLine(), &nLine))
        {
            cur->setCursor(nLine, 0);
            st.statusMsg = QStringLiteral("Next bookmark %1").arg(nLine + 1);
        }
        else
            st.statusMsg = QStringLiteral("No bookmarks");
    }
    else if (ch == KEY_F(26))
    {
        int pLine;
        if (cur->prevBookmark(cur->cursorLine(), &pLine))
        {
            cur->setCursor(pLine, 0);
            st.statusMsg = QStringLiteral("Prev bookmark %1").arg(pLine + 1);
        }
        else
            st.statusMsg = QStringLiteral("No bookmarks");
    }
    else if (ch == KEY_F(38))
    {
        cur->clearBookmarks();
        st.statusMsg = QStringLiteral("Bookmarks cleared");
    }
    else if (ch == KEY_NPAGE)
    {
        if (cur->hasSelection())
            cur->clearSelection();
        cur->moveCursor(editorH - 2, 0);
    }
    else if (ch == KEY_PPAGE)
    {
        if (cur->hasSelection())
            cur->clearSelection();
        cur->moveCursor(-editorH + 2, 0);
    }
    else if (ch == KEY_F(3))
    {
        if (!st.findQuery.isEmpty())
        {
            SearchResult r = TuiSearchEngine::findNext(cur->lines(), st.findQuery, st.findOpts, cur->cursorLine(), cur->cursorCol() + 1, true, true);
            if (r.found)
            {
                cur->setCursor(r.line, r.column);
                st.lastSearch = r;
            }
            else
                st.statusMsg = QStringLiteral("Not found");
        }
    }
    else if (ch == TabKey)
    {
        int next = (tabs.currentIndex() + 1) % tabs.count();
        tabs.setCurrentIndex(next);
        st.view.scrollTop = 0;
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
    else if (ch == KEY_SR)
    {
        if (!cur->hasSelection())
            cur->setSelectionAnchor(cur->cursorLine(), cur->cursorCol());
        cur->moveCursor(-1, 0);
    }
    else if (ch == KEY_SF)
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
                    target->setText(res.text);
                    target->setEncoding(res.encoding);
                }
                else
                {
                    TuiBuffer buf(fp, res.text, res.encoding);
                    tabs.addBuffer(buf);
                    target = tabs.currentBuffer();
                }
                st.fileMtimes[fp] = QFileInfo(fp).lastModified();
                if (target)
                    target->setCursor(l, c);
                st.statusMsg = QStringLiteral("Jumped to %1:%2").arg(fp).arg(l + 1);
            }
            else
                st.statusMsg = QStringLiteral("Cannot open %1").arg(fp);
        }
        else
            st.statusMsg = QStringLiteral("No file on this line");
    }
    else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8)
        cur->backspace();
    else if (ch == KEY_DC)
        cur->deleteChar();
    else if (ch == KEY_ENTER || ch == 10 || ch == 13)
        cur->newLine();
    else if (ch >= 32 && ch < 127)
        cur->insertChar(QChar(ch));
    else if (ch == CtrlB)
        st.statusMsg = QStringLiteral("TUI: %1 buffers  Undo:%2 Redo:%3 Sel:%4")
                           .arg(tabs.count())
                           .arg(cur->canUndo() ? "Y" : "N")
                           .arg(cur->canRedo() ? "Y" : "N")
                           .arg(cur->hasSelection() ? "Y" : "N");
    else if (ch == KEY_F(1))
        st.statusMsg = QStringLiteral(
            "Help: Ctrl+Q quit  Ctrl+S save  Ctrl+O saveAs  Ctrl+G goto  Ctrl+H replace  :s/old/new/g :%%s/old/new/g  :w/:wa/:q/:e/:cd/:set "
            "encoding/:reopen/:set wrap/:set syntax  Ctrl+Z undo  Ctrl+Y redo  Ctrl+A selAll  Ctrl+X/C/V  Shift+Arrows  Ctrl+F find  F2 "
            "bm/bnext/bprev/bclear  F3 next  Ctrl+E tree  Ctrl+K enc  / filter  F4 wrap  F5 syntax");
    else if (ch == KEY_F(4))
    {
        st.view.wordWrap = !st.view.wordWrap;
        SettingsManager::instance().setWordWrap(st.view.wordWrap);
        st.view.wrapScrollTop = 0;
        st.statusMsg = st.view.wordWrap ? QStringLiteral("Wrap on") : QStringLiteral("Wrap off");
    }
    else if (ch == KEY_F(5))
    {
        st.view.syntaxEnabled = !st.view.syntaxEnabled;
        TuiHighlighter::setEnabled(st.view.syntaxEnabled);
        st.statusMsg = st.view.syntaxEnabled ? QStringLiteral("Syntax on") : QStringLiteral("Syntax off");
    }
    else
        return false;
#else
    Q_UNUSED(editorH);
#endif
    return false; // false = not quit, true = quit (only CtrlQ)
}
