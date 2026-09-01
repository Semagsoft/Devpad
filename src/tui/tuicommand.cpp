/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuicommand.h"

#include "core/fileservice.h"
#include "encodingutils.h"
#include "managers/settingsmanager.h"
#include "tui/tuifindinfiles.h"
#include "tui/tuihighlighter.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

TuiCommand::Result TuiCommand::dispatch(const QString& rawCmd, TuiTabModel& tabs, TuiBuffer* cur, TuiFileTree& fileTree, TuiViewState& view,
                                        QString& statusMsg, SearchResult& lastSearch, SearchOptions& findOpts, QHash<QString, QDateTime>& fileMtimes,
                                        bool& outSaveAsRequest)
{
    outSaveAsRequest = false;
    QString cmd = rawCmd.trimmed();

    auto saveCurrent = [&](TuiBuffer* buf) -> bool
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
        {
            statusMsg = err;
        }
        return ok;
    };

    auto saveAs = [&](TuiBuffer* buf, const QString& newPath) -> bool
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
        {
            statusMsg = err;
        }
        return ok;
    };

    auto saveAll = [&]()
    {
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
            return Result::Quit;
    }
    else if (cmd == QStringLiteral("w") || cmd.startsWith(QStringLiteral("w ")))
    {
        QString arg = cmd.mid(1).trimmed();
        if (arg.isEmpty())
        {
            if (!saveCurrent(cur))
                outSaveAsRequest = true;
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
            return Result::Quit;
        statusMsg = QStringLiteral("Modified buffers - use :wq or :q! to force");
    }
    else if (cmd == QStringLiteral("q!"))
    {
        return Result::Quit;
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
            return Result::Quit;
    }
    else if (cmd == QStringLiteral("e!"))
    {
        if (cur && !cur->filePath().isEmpty())
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
                statusMsg = QStringLiteral("Reload failed: %1").arg(res.error);
        }
        else
            statusMsg = QStringLiteral("No file name");
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
                if (cur)
                {
                    cur->setText(res.text);
                    cur->setEncoding(res.encoding);
                    cur->setFilePath(arg);
                    cur->setModified(false);
                    fileMtimes[arg] = QFileInfo(arg).lastModified();
                    statusMsg = QStringLiteral("Reloaded %1").arg(arg);
                }
            }
            else
                statusMsg = QStringLiteral("Reload failed: %1").arg(res.error);
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
            statusMsg = QStringLiteral("Project: %1").arg(arg);
        }
        else
            statusMsg = QStringLiteral("No such directory: %1").arg(arg);
    }
    else if (cmd.startsWith(QStringLiteral("set encoding")))
    {
        QString arg = cmd.mid(QStringLiteral("set encoding").size()).trimmed();
        if (arg.isEmpty())
        {
            QStringList encs;
            for (const auto& ei : supportedEncodings())
                encs << ei.displayName;
            statusMsg =
                QStringLiteral("Encoding: %1 | Available: %2").arg(cur ? cur->encoding() : QStringLiteral("UTF-8"), encs.join(QStringLiteral(", ")));
        }
        else
        {
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
                statusMsg = QStringLiteral("Unknown encoding: %1").arg(arg);
            else if (cur)
            {
                cur->setEncoding(arg);
                statusMsg = QStringLiteral("Encoding set to %1 (save to apply)").arg(arg);
            }
        }
    }
    else if (cmd.startsWith(QStringLiteral("reopen")))
    {
        QString arg = cmd.mid(QStringLiteral("reopen").size()).trimmed();
        if (arg.startsWith(QStringLiteral("++enc=")))
            arg = arg.mid(6).trimmed();
        if (arg.isEmpty() && cur)
            arg = cur->encoding();
        if (!cur || cur->filePath().isEmpty())
            statusMsg = QStringLiteral("No file name");
        else
        {
            FileLoadResult res = FileService::load(cur->filePath(), arg);
            if (res.ok)
            {
                cur->setText(res.text);
                cur->setEncoding(res.encoding);
                cur->setModified(false);
                fileMtimes[cur->filePath()] = QFileInfo(cur->filePath()).lastModified();
                statusMsg = QStringLiteral("Reopened with %1").arg(res.encoding);
            }
            else
                statusMsg = QStringLiteral("Reopen failed: %1").arg(res.error);
        }
    }
    else if (cmd.startsWith(QStringLiteral("set wrap")))
    {
        QString arg = cmd.mid(QStringLiteral("set wrap").size()).trimmed().toLower();
        if (arg.isEmpty())
            statusMsg = QStringLiteral("Wrap is %1").arg(view.wordWrap ? QStringLiteral("on") : QStringLiteral("off"));
        else if (arg == QStringLiteral("on") || arg == QStringLiteral("1") || arg == QStringLiteral("true"))
        {
            view.wordWrap = true;
            SettingsManager::instance().setWordWrap(true);
            view.wrapScrollTop = 0;
            statusMsg = QStringLiteral("Wrap on");
        }
        else if (arg == QStringLiteral("off") || arg == QStringLiteral("0") || arg == QStringLiteral("false"))
        {
            view.wordWrap = false;
            SettingsManager::instance().setWordWrap(false);
            view.wrapScrollTop = 0;
            statusMsg = QStringLiteral("Wrap off");
        }
        else
            statusMsg = QStringLiteral("Usage: :set wrap [on|off]");
    }
    else if (cmd == QStringLiteral("syntax on") || cmd == QStringLiteral("set syntax on") || cmd == QStringLiteral("set syntax true"))
    {
        view.syntaxEnabled = true;
        TuiHighlighter::setEnabled(true);
        statusMsg = QStringLiteral("Syntax on");
    }
    else if (cmd == QStringLiteral("syntax off") || cmd == QStringLiteral("set syntax off") || cmd == QStringLiteral("set syntax false"))
    {
        view.syntaxEnabled = false;
        TuiHighlighter::setEnabled(false);
        statusMsg = QStringLiteral("Syntax off");
    }
    else if (cmd == QStringLiteral("bn") || cmd == QStringLiteral("bnext"))
    {
        if (cur)
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
    }
    else if (cmd == QStringLiteral("bp") || cmd == QStringLiteral("bprev"))
    {
        if (cur)
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
    }
    else if (cmd == QStringLiteral("bc") || cmd == QStringLiteral("bclear"))
    {
        if (cur)
            cur->clearBookmarks();
        statusMsg = QStringLiteral("Bookmarks cleared");
    }
    else if (cmd.startsWith(QStringLiteral("set searchcase")) || cmd.startsWith(QStringLiteral("set searchwhole")) ||
             cmd.startsWith(QStringLiteral("set searchregex")))
    {
        if (cmd.startsWith(QStringLiteral("set searchcase")))
        {
            QString arg = cmd.mid(QStringLiteral("set searchcase").size()).trimmed().toLower();
            if (arg == QStringLiteral("on") || arg == QStringLiteral("1"))
                findOpts.caseSensitive = true;
            else if (arg == QStringLiteral("off") || arg == QStringLiteral("0"))
                findOpts.caseSensitive = false;
            statusMsg = QStringLiteral("Search caseSensitive %1").arg(findOpts.caseSensitive ? QStringLiteral("on") : QStringLiteral("off"));
        }
        else if (cmd.startsWith(QStringLiteral("set searchwhole")))
        {
            QString arg = cmd.mid(QStringLiteral("set searchwhole").size()).trimmed().toLower();
            if (arg == QStringLiteral("on") || arg == QStringLiteral("1"))
                findOpts.wholeWords = true;
            else if (arg == QStringLiteral("off") || arg == QStringLiteral("0"))
                findOpts.wholeWords = false;
            statusMsg = QStringLiteral("Search wholeWords %1").arg(findOpts.wholeWords ? QStringLiteral("on") : QStringLiteral("off"));
        }
        else if (cmd.startsWith(QStringLiteral("set searchregex")))
        {
            QString arg = cmd.mid(QStringLiteral("set searchregex").size()).trimmed().toLower();
            if (arg == QStringLiteral("on") || arg == QStringLiteral("1"))
                findOpts.regex = true;
            else if (arg == QStringLiteral("off") || arg == QStringLiteral("0"))
                findOpts.regex = false;
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
        QStringList tokens = raw.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (!tokens.isEmpty())
            pattern = tokens[0];
        if (tokens.size() >= 2)
            dir = tokens[1];
        if (tokens.size() >= 3)
            glob = tokens[2];
        if (pattern.isEmpty())
            statusMsg = QStringLiteral("Usage: :grep <pattern> [dir] [glob]");
        else
        {
            QString root = dir.isEmpty() ? (fileTree.hasRoot() ? fileTree.rootPath() : QDir::currentPath()) : dir;
            if (!QFileInfo(root).isAbsolute())
                root = QDir::current().absoluteFilePath(root);
            QList<FindInFilesResult> results = TuiFindInFiles::search(root, pattern, findOpts, glob);
            if (results.isEmpty())
                statusMsg = QStringLiteral("No matches for %1").arg(pattern);
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
        QStringList parts = rest.split(QChar('/'));
        if (parts.size() >= 2)
        {
            QString find = parts[0];
            QString repl = parts[1];
            QString flags = parts.size() >= 3 ? parts[2] : QString();
            bool all = global || flags.contains(QLatin1Char('g'));
            if (find.isEmpty())
                statusMsg = QStringLiteral("Find string empty");
            else if (!cur)
                statusMsg = QStringLiteral("No buffer");
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
            statusMsg = QStringLiteral("Usage: :s/old/new/[g] or :%%s/old/new/[g]");
    }
    else
        statusMsg = QStringLiteral("Unknown command: %1").arg(cmd);

    return Result::Continue;
}

void TuiCommand::complete(QString& commandInput, QString& statusMsg)
{
    QString input = commandInput;
    QStringList allCmds = {QStringLiteral("w"),
                           QStringLiteral("wa"),
                           QStringLiteral("wqa"),
                           QStringLiteral("wq"),
                           QStringLiteral("q"),
                           QStringLiteral("q!"),
                           QStringLiteral("e "),
                           QStringLiteral("e!"),
                           QStringLiteral("cd "),
                           QStringLiteral("set encoding "),
                           QStringLiteral("set wrap "),
                           QStringLiteral("set syntax on"),
                           QStringLiteral("set syntax off"),
                           QStringLiteral("syntax on"),
                           QStringLiteral("syntax off"),
                           QStringLiteral("set searchcase "),
                           QStringLiteral("set searchwhole "),
                           QStringLiteral("set searchregex "),
                           QStringLiteral("reopen"),
                           QStringLiteral("grep "),
                           QStringLiteral("findinfiles "),
                           QStringLiteral("s/"),
                           QStringLiteral("%s/"),
                           QStringLiteral("bn"),
                           QStringLiteral("bnext"),
                           QStringLiteral("bp"),
                           QStringLiteral("bprev"),
                           QStringLiteral("bc"),
                           QStringLiteral("bclear")};
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
            for (int i = 1; i < cands.size(); ++i)
            {
                int j = 0;
                while (j < common.size() && j < cands[i].size() && common[j].toLower() == cands[i][j].toLower())
                    ++j;
                common = common.left(j);
            }
            if (common.size() > prefix.size())
                commandInput = QStringLiteral("set encoding ") + common;
            statusMsg = QStringLiteral("Encodings: %1").arg(cands.join(QStringLiteral(", ")));
        }
        return;
    }
    QStringList cands;
    for (const QString& c : allCmds)
        if (c.startsWith(input))
            cands << c;
    if (cands.size() == 1)
    {
        commandInput = cands.first();
        if (!commandInput.endsWith(QLatin1Char(' ')) && !commandInput.endsWith(QLatin1Char('/')))
            commandInput += QLatin1Char(' ');
    }
    else if (cands.size() > 1)
    {
        QString common = cands.first();
        for (int i = 1; i < cands.size(); ++i)
        {
            int j = 0;
            while (j < common.size() && j < cands[i].size() && common[j] == cands[i][j])
                ++j;
            common = common.left(j);
        }
        if (common.size() > input.size())
            commandInput = common;
        statusMsg = QStringLiteral("Candidates: %1").arg(cands.join(QStringLiteral(", ")));
    }
}
