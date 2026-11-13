/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef EDITORCONTROLLER_H
#define EDITORCONTROLLER_H

#include "codeeditor.h"

#include <QObject>
#include <QString>

class TabManager;
class FileManager;
class FileWatcherManager;
class ActionManager;
class QTabWidget;

class EditorController : public QObject
{
    Q_OBJECT

public:
    EditorController(TabManager* tabManager, FileManager* fileManager, FileWatcherManager* watcher, ActionManager* actionManager,
                     QTabWidget* tabWidget, QObject* parent = nullptr);

    void newFile();
    bool saveFile();
    bool saveFileAs();
    void saveAll();
    bool maybeSave(CodeEditor* editor);

    void closeCurrentTab();
    void closeAllTabs();
    bool onTabCloseRequested(int index);

    CodeEditor* currentEditor() const;
    CodeEditor* findEditorByFileName(const QString& fileName) const;

    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void deleteText();
    void zoomIn();
    void zoomOut();
    void zoomReset();
    void toggleWordWrap();
    void toggleReadOnly();
    void toggleBookmark();
    void nextBookmark();
    void insertSnippet();
    void toggleComment();
    void prevBookmark();
    void clearBookmarks();

    void updateUndoRedoState();
    void updateReadOnlyActionState();

    void autoSave();
    void promptBackupRestore(const QString& filePath);
    void connectEditorSignals(CodeEditor* editor);
    bool saveEditor(CodeEditor* editor, const QString& fileName);
    // Drop the synthetic auto-save backup of an untitled editor. Used when a
    // document was explicitly discarded ("Don't Save") so it is not
    // resurrected on the next launch.
    void discardUntitledBackup(CodeEditor* editor);

    // Load a file from disk into a new editor tab.
    // Returns the editor on success, nullptr on failure.
    // Does NOT check if file is already open (caller handles that).
    CodeEditor* openFile(const QString& fileName, const QString& encoding = QString());

    // Reload the file from disk into an existing editor, in place.
    // Preserves the editor's tab, pin state and (best effort) bookmarks.
    // Used by external-change reload and encoding re-read; unlike a
    // close-and-reopen it also works for pinned tabs.
    bool reloadFileInPlace(CodeEditor* editor, const QString& encoding = QString(), const QList<int>& bookmarks = {});

    // Reload the current editor using a different encoding
    void reloadWithEncoding(const QString& encoding);

    // Save the current editor with a specific encoding
    void saveWithEncoding(const QString& encoding);

    // Update every editor (and its watcher) when a file is renamed on disk
    // via the project panel.
    void retargetEditorFile(const QString& oldPath, const QString& newPath);

    // Stop watching a file that was deleted on disk (keep the editor tab so
    // unsaved changes survive; a later save recreates the file).
    void handleFileDeleted(const QString& path);

signals:
    void fileSaved(const QString& fileName);
    void editorConnected(CodeEditor* editor);

private:
    void applyDefaultSyntax(CodeEditor* editor) const;
    bool checkSaveConflict(CodeEditor* editor, const QString& fileName);

    // Synthetic backup key for an untitled (never-saved) document, so its
    // content is covered by auto-save. Stored as a dynamic property on the
    // editor; stable for the editor's lifetime. Keys look like
    // "untitled-<pid>-<n>" and are removed once the document is saved or the
    // backup is restored at startup.
    QString untitledBackupKey(CodeEditor* editor);
    void deleteUntitledBackup(CodeEditor* editor);

    TabManager* m_tabManager;
    FileManager* m_fileManager;
    FileWatcherManager* m_fileWatcherManager;
    ActionManager* m_actionManager;
    QTabWidget* m_tabWidget;
    int m_untitledBackupSeq = 0;
};

#endif
