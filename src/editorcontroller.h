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

#include <QObject>
#include <QString>

class CodeEditor;
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

    [[nodiscard]] CodeEditor* currentEditor() const;
    [[nodiscard]] CodeEditor* findEditorByFileName(const QString& fileName) const;

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

    // Load a file from disk into a new editor tab.
    // Returns the editor on success, nullptr on failure.
    // Does NOT check if file is already open (caller handles that).
    CodeEditor* openFile(const QString& fileName, const QString& encoding = QString());

    // Reload the current editor using a different encoding
    void reloadWithEncoding(const QString& encoding);

    // Save the current editor with a specific encoding
    void saveWithEncoding(const QString& encoding);

signals:
    void fileSaved(const QString& fileName);
    void editorConnected(CodeEditor* editor);

private:
    void applyDefaultSyntax(CodeEditor* editor) const;

    TabManager* m_tabManager;
    FileManager* m_fileManager;
    FileWatcherManager* m_fileWatcherManager;
    ActionManager* m_actionManager;
    QTabWidget* m_tabWidget;
};

#endif
