/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "nextgenactions.h"

#include "core/fileservice.h"
#include "devpad_version.h"
#include "encodingutils.h"
#include "managers/settingsmanager.h"
#include "managers/externaltoolmanager.h"
#include "managers/externaltoolmanager.h"
#include "nextgen/primoeditor.h"
#include "nextgen/primoterminal.h"
#include "theme.h"

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>

NextgenActions::NextgenActions(QObject* parent) : QObject(parent), m_editor(nullptr), m_externalToolManager(new ExternalToolManager(this))
{
    connect(&SettingsManager::instance(), &SettingsManager::settingsChanged, this,
            [this]()
            {
                emit showToolbarChanged();
                emit showStatusbarChanged();
                emit showMenuBarChanged();
                emit recentFilesChanged();
            });
}
NextgenActions::NextgenActions(PrimoEditor* editor, QObject* parent) : QObject(parent), m_editor(editor), m_externalToolManager(new ExternalToolManager(this))
{
    connect(&SettingsManager::instance(), &SettingsManager::settingsChanged, this,
            [this]()
            {
                emit showToolbarChanged();
                emit showStatusbarChanged();
                emit showMenuBarChanged();
                emit recentFilesChanged();
            });
}
QObject* NextgenActions::editorObj() const
{
    return m_editor;
}
void NextgenActions::setEditorObj(QObject* obj)
{
    auto* ed = qobject_cast<PrimoEditor*>(obj);
    if (ed == m_editor)
        return;
    m_editor = ed;
    emit editorChanged();
    emit currentEncodingChanged();
}

bool NextgenActions::showToolbar() const
{
    return SettingsManager::instance().showToolbar();
}
void NextgenActions::setShowToolbar(bool v)
{
    SettingsManager::instance().setShowToolbar(v);
    emit showToolbarChanged();
}
bool NextgenActions::showStatusbar() const
{
    return SettingsManager::instance().showStatusbar();
}
void NextgenActions::setShowStatusbar(bool v)
{
    SettingsManager::instance().setShowStatusbar(v);
    emit showStatusbarChanged();
}
bool NextgenActions::showMenuBar() const
{
    return SettingsManager::instance().showMenuBar();
}
void NextgenActions::setShowMenuBar(bool v)
{
    SettingsManager::instance().setShowMenuBar(v);
    emit showMenuBarChanged();
}
QStringList NextgenActions::recentFiles() const
{
    return SettingsManager::instance().recentFiles();
}
QStringList NextgenActions::recentFolders() const
{
    return SettingsManager::instance().recentFolders();
}
QString NextgenActions::currentEncoding() const
{
    return m_editor ? m_editor->encoding() : QStringLiteral("UTF-8");
}
QStringList NextgenActions::themeNames() const
{
    QStringList names;
    for (auto id : allBuiltInThemes())
        names << themeDisplayName(id);
    names << customThemeNames();
    return names;
}
QString NextgenActions::currentThemeName() const
{
    // If custom theme stored as ThemeId::System? For now return built-in name
    ThemeId tid = SettingsManager::instance().theme();
    // Check if custom? For simplicity, if currentTheme is beyond Count, treat as custom
    // SettingsManager stores ThemeId, so custom not yet; just return built-in
    return themeDisplayName(tid);
}
void NextgenActions::setThemeByName(const QString& name)
{
    for (auto id : allBuiltInThemes())
    {
        if (themeDisplayName(id) == name)
        {
            setThemeById(static_cast<int>(id));
            return;
        }
    }
    // Try custom
    QStringList customs = customThemeNames();
    if (customs.contains(name))
    {
        ThemeColors c = getCustomThemeColors(name);
        if (m_editor)
        {
            m_editor->setCustomTheme(name);
            m_editor->setBackgroundColor(c.background);
            m_editor->setForegroundColor(c.foreground);
            // Apply full theme colors via setTheme to update highlight
            m_editor->applyThemeColors(c);
            // trigger highlight re-evaluation
            m_editor->setLanguage(m_editor->language());
        }
        SettingsManager::instance().setCustomTheme(name);
        SettingsManager::instance().setTheme(ThemeId::Light); // placeholder; custom tracked separately
        emit themeChanged();
    }
}
void NextgenActions::setThemeById(int id)
{
    if (id < 0 || id >= static_cast<int>(ThemeId::Count))
        return;
    ThemeId tid = static_cast<ThemeId>(id);
    SettingsManager::instance().setTheme(tid);
    if (m_editor)
    {
        ThemeColors c = getThemeColors(tid);
        m_editor->setBackgroundColor(c.background);
        m_editor->setForegroundColor(c.foreground);
        // trigger highlight re-evaluation
        m_editor->setLanguage(m_editor->language());
    }
    emit themeChanged();
}
void NextgenActions::refreshRecent()
{
    emit recentFilesChanged();
}
void NextgenActions::clearRecentFiles()
{
    SettingsManager::instance().clearRecentFiles();
    emit recentFilesChanged();
}

void NextgenActions::newFile()
{
    if (!m_editor)
        return;
    m_editor->setText(QString());
    m_editor->setFilePath(QString());
    m_editor->setCursorPosition(0, 0);
    emit currentEncodingChanged();
}
void NextgenActions::openFile(const QString& path)
{
    if (!m_editor)
        return;
    QString p = path;
    if (p.isEmpty())
    {
        emit requestOpenFileDialog();
        return;
    }
    m_editor->loadFile(p);
    SettingsManager::instance().addRecentFile(p);
    emit recentFilesChanged();
    emit currentEncodingChanged();
}
void NextgenActions::openFileDialog()
{
    emit requestOpenFileDialog();
}
void NextgenActions::openFolderDialog()
{
    emit requestOpenFolderDialog();
}
void NextgenActions::openRemoteDialog()
{
    emit requestOpenRemoteDialog();
}
void NextgenActions::openRemoteFolderDialog()
{
    emit requestOpenRemoteFolderDialog();
}
void NextgenActions::saveFile()
{
    if (!m_editor)
        return;
    if (m_editor->filePath().isEmpty())
    {
        emit requestSaveAsDialog();
        return;
    }
    m_editor->save();
}
void NextgenActions::saveFileAs()
{
    emit requestSaveAsDialog();
}
void NextgenActions::saveFileAsDialog()
{
    emit requestSaveAsDialog();
}
void NextgenActions::closeFile()
{
    newFile();
}
void NextgenActions::exitApp()
{
    QApplication::quit();
}
void NextgenActions::undo()
{
    if (m_editor && !m_editor->isReadOnly() && m_editor->canUndo())
        m_editor->undo();
    else if (m_editor && m_editor->isUndoDisabled())
        emit showMessage(QStringLiteral("Undo disabled for large file >50MB (perf)"));
}
void NextgenActions::redo()
{
    if (m_editor && m_editor->canRedo())
        m_editor->redo();
}
void NextgenActions::cut()
{
    if (m_editor)
        m_editor->cut();
}
void NextgenActions::copy()
{
    if (m_editor)
        m_editor->copy();
}
void NextgenActions::paste()
{
    if (m_editor)
        m_editor->paste();
}
void NextgenActions::selectAll()
{
    if (m_editor)
        m_editor->selectAll();
}
void NextgenActions::find()
{
    emit requestFindDialog();
}
void NextgenActions::replace()
{
    emit requestReplaceDialog();
}
void NextgenActions::findNext()
{
    emit showMessage(QStringLiteral("Find Next – use Find bar (Ctrl+F) or press F3 in editor"));
}
void NextgenActions::findPrevious()
{
    emit showMessage(QStringLiteral("Find Previous – use Find bar (Ctrl+F) or press Shift+F3 in editor"));
}
void NextgenActions::goToLine()
{
    emit requestGoToLineDialog();
}
void NextgenActions::toggleBookmark()
{
    if (m_editor)
        m_editor->toggleBookmarkCurrent();
}
void NextgenActions::nextBookmark()
{
    if (m_editor)
        m_editor->nextBookmark();
}
void NextgenActions::prevBookmark()
{
    if (m_editor)
        m_editor->prevBookmark();
}
void NextgenActions::clearBookmarks()
{
    if (m_editor)
        m_editor->clearBookmarks();
}
void NextgenActions::zoomIn()
{
    if (!m_editor)
        return;
    QFont f = m_editor->font();
    f.setPointSize(f.pointSize() + 1);
    m_editor->setFont(f);
    SettingsManager::instance().setDefaultFontFamily(f.family());
    SettingsManager::instance().setDefaultFontSize(f.pointSize());
}
void NextgenActions::zoomOut()
{
    if (!m_editor)
        return;
    QFont f = m_editor->font();
    f.setPointSize(qMax(6, f.pointSize() - 1));
    m_editor->setFont(f);
    SettingsManager::instance().setDefaultFontSize(f.pointSize());
}
void NextgenActions::zoomReset()
{
    if (!m_editor)
        return;
    QFont f = SettingsManager::instance().defaultFont();
    m_editor->setFont(f);
}
void NextgenActions::toggleFullScreen()
{
    emit showMessage(QStringLiteral("Fullscreen toggle via View menu – use F11"));
}
void NextgenActions::toggleWordWrap()
{
    if (!m_editor)
        return;
    m_editor->setWordWrap(!m_editor->wordWrap());
    SettingsManager::instance().setWordWrap(m_editor->wordWrap());
}
void NextgenActions::showAbout()
{
    emit requestAboutDialog();
}
void NextgenActions::showOptions()
{
    emit requestOptionsDialog();
}
void NextgenActions::reopenWithEncoding(const QString& enc)
{
    if (m_editor)
    {
        m_editor->setEncoding(enc); // reload would need FileService re-load
        emit currentEncodingChanged();
        emit showMessage(QStringLiteral("Reopen with ") + enc);
    }
}
void NextgenActions::saveWithEncoding(const QString& enc)
{
    if (m_editor)
    {
        m_editor->setEncoding(enc);
        m_editor->save();
        emit currentEncodingChanged();
    }
}
void NextgenActions::openRecentFile(const QString& path)
{
    openFile(path);
}
void NextgenActions::toggleTerminal()
{
    emit requestTerminalToggle();
}
void NextgenActions::findInFiles(const QString& pattern)
{
    Q_UNUSED(pattern);
    emit requestFindInFiles();
}

// ── Settings getters/setters ──────────────────────────────────

bool NextgenActions::checkForUpdatesEnabled() const
{
    return SettingsManager::instance().checkForUpdatesEnabled();
}
void NextgenActions::setCheckForUpdatesEnabled(bool enabled)
{
    SettingsManager::instance().setCheckForUpdatesEnabled(enabled);
    emit settingsChanged();
}
bool NextgenActions::wordWrap() const
{
    return SettingsManager::instance().wordWrap();
}
void NextgenActions::setWordWrap(bool enabled)
{
    SettingsManager::instance().setWordWrap(enabled);
    emit settingsChanged();
    if (m_editor) m_editor->setWordWrap(enabled);
}
bool NextgenActions::showLineNumbers() const
{
    return SettingsManager::instance().showLineNumbers();
}
void NextgenActions::setShowLineNumbers(bool visible)
{
    SettingsManager::instance().setShowLineNumbers(visible);
    emit settingsChanged();
}
bool NextgenActions::autoCloseBrackets() const
{
    return SettingsManager::instance().autoCloseBrackets();
}
void NextgenActions::setAutoCloseBrackets(bool enabled)
{
    SettingsManager::instance().setAutoCloseBrackets(enabled);
    emit settingsChanged();
}
bool NextgenActions::snippetsEnabled() const
{
    return SettingsManager::instance().snippetsEnabled();
}
void NextgenActions::setSnippetsEnabled(bool enabled)
{
    SettingsManager::instance().setSnippetsEnabled(enabled);
    emit settingsChanged();
}
bool NextgenActions::predictiveSnippets() const
{
    return SettingsManager::instance().predictiveSnippets();
}
void NextgenActions::setPredictiveSnippets(bool enabled)
{
    SettingsManager::instance().setPredictiveSnippets(enabled);
    emit settingsChanged();
}
bool NextgenActions::autoCompletionEnabled() const
{
    return SettingsManager::instance().autoCompletionEnabled();
}
void NextgenActions::setAutoCompletionEnabled(bool enabled)
{
    SettingsManager::instance().setAutoCompletionEnabled(enabled);
    emit settingsChanged();
}
int NextgenActions::tabWidth() const
{
    return SettingsManager::instance().tabWidth();
}
void NextgenActions::setTabWidth(int width)
{
    SettingsManager::instance().setTabWidth(width);
    emit settingsChanged();
    if (m_editor) m_editor->setTabWidth(width);
}
int NextgenActions::defaultFontSize() const
{
    return SettingsManager::instance().defaultFontSize();
}
void NextgenActions::setDefaultFontSize(int size)
{
    SettingsManager::instance().setDefaultFontSize(size);
    emit settingsChanged();
    if (m_editor) m_editor->setFont(SettingsManager::instance().defaultFont());
}
QString NextgenActions::defaultFontFamily() const
{
    return SettingsManager::instance().defaultFontFamily();
}
void NextgenActions::setDefaultFontFamily(const QString& family)
{
    SettingsManager::instance().setDefaultFontFamily(family);
    emit settingsChanged();
    if (m_editor) m_editor->setFont(SettingsManager::instance().defaultFont());
}
bool NextgenActions::showHiddenFiles() const
{
    return SettingsManager::instance().showHiddenFiles();
}
void NextgenActions::setShowHiddenFiles(bool visible)
{
    SettingsManager::instance().setShowHiddenFiles(visible);
    emit settingsChanged();
}
bool NextgenActions::useGitIgnore() const
{
    return SettingsManager::instance().useGitIgnore();
}
void NextgenActions::setUseGitIgnore(bool enabled)
{
    SettingsManager::instance().setUseGitIgnore(enabled);
    emit settingsChanged();
}
bool NextgenActions::autoSaveEnabled() const
{
    return SettingsManager::instance().autoSaveEnabled();
}
void NextgenActions::setAutoSaveEnabled(bool enabled)
{
    SettingsManager::instance().setAutoSaveEnabled(enabled);
    emit settingsChanged();
}
int NextgenActions::autoSaveInterval() const
{
    return SettingsManager::instance().autoSaveInterval();
}
void NextgenActions::setAutoSaveInterval(int seconds)
{
    SettingsManager::instance().setAutoSaveInterval(seconds);
    emit settingsChanged();
}

// ── External tools ────────────────────────────────────────────

int NextgenActions::externalToolCount() const
{
    return SettingsManager::instance().externalToolCount();
}
QString NextgenActions::externalToolName(int index) const
{
    return SettingsManager::instance().externalToolName(index);
}
QString NextgenActions::externalToolCommand(int index) const
{
    return SettingsManager::instance().externalToolCommand(index);
}
QString NextgenActions::externalToolArguments(int index) const
{
    return SettingsManager::instance().externalToolArguments(index);
}
QString NextgenActions::externalToolWorkingDir(int index) const
{
    return SettingsManager::instance().externalToolWorkingDir(index);
}
QString NextgenActions::externalToolShortcut(int index) const
{
    return SettingsManager::instance().externalToolShortcut(index);
}
bool NextgenActions::externalToolRunInTerminal(int index) const
{
    return SettingsManager::instance().externalToolRunInTerminal(index);
}
void NextgenActions::addExternalTool(const QString& name, const QString& command, const QString& arguments,
                                     const QString& workingDir, const QString& shortcut, bool runInTerminal)
{
    SettingsManager::instance().addExternalTool(name, command, arguments, workingDir, shortcut, runInTerminal);
    emit externalToolsChanged();
}
void NextgenActions::removeExternalTool(int index)
{
    SettingsManager::instance().removeExternalTool(index);
    emit externalToolsChanged();
}
void NextgenActions::runExternalTool(int index, const QString& filePath, const QString& projectDir,
                                     const QString& selectedText, int lineNumber)
{
    // Terminal sender: write to PrimoTerminal shared instance
    auto* term = PrimoTerminal::instance();
    auto sender = [term](const QString& cmd) {
        if (term) term->write(cmd);
    };
    m_externalToolManager->runTool(index, filePath, projectDir, selectedText, lineNumber, sender);
}

// ── Command mode (:) ────────────────────────────────────────────
QString NextgenActions::runCommand(const QString& rawCmd)
{
    QString cmd = rawCmd.trimmed();
    if (cmd.isEmpty())
        return QStringLiteral("");

    // ── File ──────────────────────────────────────────
    if (cmd == QStringLiteral("wa"))
    {
        // Save all modified buffers with file paths (best effort via current editor)
        if (m_editor && !m_editor->filePath().isEmpty() && m_editor->document() && m_editor->document()->isModified())
            m_editor->save();
        return QStringLiteral("Saved all");
    }
    if (cmd == QStringLiteral("wqa") || cmd == QStringLiteral("waq"))
    {
        if (m_editor && !m_editor->filePath().isEmpty() && m_editor->document() && m_editor->document()->isModified())
            m_editor->save();
        return QStringLiteral("quit"); // handled by QML
    }
    if (cmd == QStringLiteral("w") || cmd.startsWith(QStringLiteral("w ")))
    {
        if (!m_editor)
            return QStringLiteral("No editor");
        QString arg = cmd.mid(1).trimmed();
        if (arg.isEmpty())
        {
            if (m_editor->filePath().isEmpty())
                emit requestSaveAsDialog();
            else
                m_editor->save();
        }
        else
        {
            if (!QFileInfo(arg).isAbsolute())
                arg = QDir::current().absoluteFilePath(arg);
            m_editor->saveAs(arg);
        }
        return QStringLiteral("Saved");
    }
    if (cmd == QStringLiteral("q"))
    {
        if (m_editor && m_editor->document() && m_editor->document()->isModified())
            return QStringLiteral("Modified buffers - use :wq or :q! to force");
        return QStringLiteral("quit");
    }
    if (cmd == QStringLiteral("q!"))
        return QStringLiteral("quit");
    if (cmd == QStringLiteral("wq") || cmd.startsWith(QStringLiteral("wq ")))
    {
        if (m_editor)
        {
            if (m_editor->filePath().isEmpty())
                emit requestSaveAsDialog();
            else
                m_editor->save();
        }
        return QStringLiteral("quit");
    }
    if (cmd == QStringLiteral("e!"))
    {
        if (m_editor && !m_editor->filePath().isEmpty())
        {
            // Reload from disk: best effort via loadFile
            m_editor->loadFile(m_editor->filePath());
            return QStringLiteral("Reloaded");
        }
        return QStringLiteral("No file name");
    }
    if (cmd.startsWith(QStringLiteral("e! ")))
    {
        QString arg = cmd.mid(3).trimmed();
        if (!arg.isEmpty())
        {
            if (!QFileInfo(arg).isAbsolute())
                arg = QDir::current().absoluteFilePath(arg);
            if (m_editor)
                m_editor->loadFile(arg);
            return QStringLiteral("Opened ") + arg;
        }
        return QStringLiteral("Usage: :e! <path>");
    }
    if (cmd.startsWith(QStringLiteral("e ")))
    {
        QString arg = cmd.mid(2).trimmed();
        if (!arg.isEmpty())
        {
            if (!QFileInfo(arg).isAbsolute())
                arg = QDir::current().absoluteFilePath(arg);
            if (m_editor)
                m_editor->loadFile(arg);
            return QStringLiteral("Opened ") + arg;
        }
        return QStringLiteral("Usage: :e <path>");
    }
    if (cmd.startsWith(QStringLiteral("cd ")) || cmd == QStringLiteral("cd"))
    {
        QString arg = cmd == QStringLiteral("cd") ? QDir::currentPath() : cmd.mid(3).trimmed();
        if (!QFileInfo(arg).isAbsolute())
            arg = QDir::current().absoluteFilePath(arg);
        QFileInfo fi(arg);
        if (fi.isDir() && fi.exists())
        {
            emit showMessage(QStringLiteral("Project: %1").arg(arg));
            return QStringLiteral("Project: ") + arg;
        }
        return QStringLiteral("No such directory: ") + arg;
    }

    // ── Settings ──────────────────────────────────────
    if (cmd.startsWith(QStringLiteral("set wrap")))
    {
        QString arg = cmd.mid(QStringLiteral("set wrap").size()).trimmed().toLower();
        if (arg.isEmpty())
            return QStringLiteral("Wrap is %1").arg(m_editor && m_editor->wordWrap() ? QStringLiteral("on") : QStringLiteral("off"));
        bool on = (arg == QStringLiteral("on") || arg == QStringLiteral("1") || arg == QStringLiteral("true"));
        if (arg == QStringLiteral("off") || arg == QStringLiteral("0") || arg == QStringLiteral("false"))
            on = false;
        if (m_editor)
            m_editor->setWordWrap(on);
        SettingsManager::instance().setWordWrap(on);
        return QStringLiteral("Wrap %1").arg(on ? QStringLiteral("on") : QStringLiteral("off"));
    }
    if (cmd == QStringLiteral("set number") || cmd == QStringLiteral("set number on") || cmd == QStringLiteral("set number true"))
    {
        if (m_editor)
            m_editor->setShowLineNumbers(true);
        SettingsManager::instance().setShowLineNumbers(true);
        return QStringLiteral("Line numbers on");
    }
    if (cmd == QStringLiteral("set nonumber") || cmd == QStringLiteral("set number off") || cmd == QStringLiteral("set number false"))
    {
        if (m_editor)
            m_editor->setShowLineNumbers(false);
        SettingsManager::instance().setShowLineNumbers(false);
        return QStringLiteral("Line numbers off");
    }
    if (cmd.startsWith(QStringLiteral("set number")))
    {
        QString arg = cmd.mid(QStringLiteral("set number").size()).trimmed().toLower();
        if (arg.isEmpty())
            return QStringLiteral("Line numbers %1").arg(m_editor && m_editor->showLineNumbers() ? QStringLiteral("on") : QStringLiteral("off"));
        return QStringLiteral("Usage: :set number [on|off]");
    }
    if (cmd.startsWith(QStringLiteral("set syntax ")) || cmd.startsWith(QStringLiteral("set ft ")))
    {
        QString arg = cmd.startsWith(QStringLiteral("set syntax ")) ? cmd.mid(QStringLiteral("set syntax ").size()).trimmed()
                                                                    : cmd.mid(QStringLiteral("set ft ").size()).trimmed();
        if (arg.isEmpty() || arg == QStringLiteral("off") || arg == QStringLiteral("none"))
        {
            if (m_editor)
                m_editor->setLanguage(QString());
            return QStringLiteral("Syntax language auto-detected");
        }
        if (m_editor)
            m_editor->setLanguage(arg);
        return QStringLiteral("Syntax language set to %1").arg(arg);
    }
    if (cmd.startsWith(QStringLiteral("set encoding")))
    {
        QString arg = cmd.mid(QStringLiteral("set encoding").size()).trimmed();
        if (arg.isEmpty())
            return QStringLiteral("Encoding: %1").arg(m_editor ? m_editor->encoding() : QStringLiteral("UTF-8"));
        if (m_editor)
            m_editor->setEncoding(arg);
        return QStringLiteral("Encoding set to %1 (save to apply)").arg(arg);
    }

    // ── Tabs ──────────────────────────────────────────
    if (cmd == QStringLiteral("tabs"))
    {
        emit showMessage(QStringLiteral("Tabs: use Tab key or Ctrl+Tab in editor"));
        return QStringLiteral("Tabs");
    }
    if (cmd == QStringLiteral("tabnext") || cmd == QStringLiteral("tabn"))
    {
        emit requestTabNext();
        return QStringLiteral("Tab next");
    }
    if (cmd == QStringLiteral("tabprev") || cmd == QStringLiteral("tabp"))
    {
        emit requestTabPrev();
        return QStringLiteral("Tab prev");
    }
    if (cmd == QStringLiteral("tabfirst") || cmd == QStringLiteral("tabf"))
    {
        emit requestTabFirst();
        return QStringLiteral("Tab first");
    }
    if (cmd == QStringLiteral("tablast") || cmd == QStringLiteral("tabl"))
    {
        emit requestTabLast();
        return QStringLiteral("Tab last");
    }
    if (cmd == QStringLiteral("tabclose") || cmd == QStringLiteral("tabc"))
    {
        emit requestTabClose();
        return QStringLiteral("Tab closed");
    }

    // ── Bookmarks ─────────────────────────────────────
    if (cmd == QStringLiteral("bn") || cmd == QStringLiteral("bnext"))
    {
        if (m_editor)
            m_editor->nextBookmark();
        return QStringLiteral("Next bookmark");
    }
    if (cmd == QStringLiteral("bp") || cmd == QStringLiteral("bprev"))
    {
        if (m_editor)
            m_editor->prevBookmark();
        return QStringLiteral("Prev bookmark");
    }
    if (cmd == QStringLiteral("bc") || cmd == QStringLiteral("bclear"))
    {
        if (m_editor)
            m_editor->clearBookmarks();
        return QStringLiteral("Bookmarks cleared");
    }

    // ── Replace ───────────────────────────────────────
    if (cmd.startsWith(QStringLiteral("s/")) || cmd.startsWith(QStringLiteral("%s/")))
    {
        if (!m_editor || !m_editor->document())
            return QStringLiteral("No buffer");
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
                return QStringLiteral("Find string empty");
            QString txt = m_editor->document()->text();
            if (all)
            {
                int cnt = 0;
                QString newTxt = txt;
                int pos = 0;
                while ((pos = newTxt.indexOf(find, pos)) >= 0)
                {
                    newTxt.replace(pos, find.size(), repl);
                    pos += repl.size();
                    ++cnt;
                }
                if (cnt > 0)
                {
                    m_editor->setText(newTxt);
                    return QStringLiteral("Replaced %1 occurrences").arg(cnt);
                }
                return QStringLiteral("Not found: %1").arg(find);
            }
            else
            {
                int pos = txt.indexOf(find);
                if (pos >= 0)
                {
                    QString newTxt = txt;
                    newTxt.replace(pos, find.size(), repl);
                    m_editor->setText(newTxt);
                    return QStringLiteral("Replaced at %1").arg(pos);
                }
                return QStringLiteral("Not found: %1").arg(find);
            }
        }
        return QStringLiteral("Usage: :s/old/new/[g] or :%%s/old/new/[g]");
    }

    // ── Grep / Find in Files ──────────────────────────
    if (cmd.startsWith(QStringLiteral("grep")) || cmd.startsWith(QStringLiteral("findinfiles")))
    {
        emit requestFindInFiles();
        return QStringLiteral("Find in Files drawer opened");
    }

    // ── Help / Version ────────────────────────────────
    if (cmd == QStringLiteral("help") || cmd == QStringLiteral("h"))
        return QStringLiteral("Commands: :w/:wa/:wq/:q/:q!/:e/:e!/:cd/:set wrap/:set number/:set syntax/:set encoding/:set searchcase/:set searchwhole/:set searchregex/:tabs/:tabnext/:tabprev/:tabfirst/:tablast/:tabclose/:bn/:bp/:bc/:s/:/%s/:/grep/:help/:version");
    if (cmd == QStringLiteral("version") || cmd == QStringLiteral("ver"))
        return QStringLiteral("Devpad Nextgen ") + QStringLiteral(DEVPAD_VERSION);

    // ── LSP ───────────────────────────────────────────
    if (cmd.startsWith(QStringLiteral("lsp start")) || cmd.startsWith(QStringLiteral("lsp ")))
    {
        QString rest = cmd.startsWith(QStringLiteral("lsp start")) ? cmd.mid(QStringLiteral("lsp start").size()).trimmed()
                                                                    : cmd.mid(QStringLiteral("lsp ").size()).trimmed();
        QStringList parts = rest.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.isEmpty())
            return QStringLiteral("Usage: :lsp start <command> [args...]");
        QString command = parts[0];
        QStringList args = parts.mid(1);
        QString rootUri = QStringLiteral("file://") + QDir::currentPath();
        emit requestLspStart(command, args, rootUri);
        return QStringLiteral("Starting LSP server: ") + command;
    }
    if (cmd == QStringLiteral("lsp stop"))
    {
        emit requestLspStop();
        return QStringLiteral("LSP server stopped");
    }
    if (cmd == QStringLiteral("lsp status"))
    {
        return QStringLiteral("LSP server status: check server output");
    }

    return QStringLiteral("Unknown command: %1").arg(cmd);
}
