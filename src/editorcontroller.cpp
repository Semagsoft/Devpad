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
#include "editorcontroller.h"

#include "actionmanager.h"
#include "appstrings.h"
#include "backupmanager.h"
#include "codeeditor.h"
#include "defaultsyntax.h"
#include "filemanager.h"
#include "filewatchermanager.h"
#include "logger.h"
#include "settingsmanager.h"
#include "snippetmanager.h"
#include "tabmanager.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QPointer>
#include <QTabWidget>

#include <Qsci/qsciscintilla.h>

EditorController::EditorController(TabManager* tabManager, FileManager* fileManager, FileWatcherManager* watcher, ActionManager* actionManager,
                                   QTabWidget* tabWidget, QObject* parent)
    : QObject(parent), m_tabManager(tabManager), m_fileManager(fileManager), m_fileWatcherManager(watcher), m_actionManager(actionManager),
      m_tabWidget(tabWidget)
{
}

CodeEditor* EditorController::currentEditor() const
{
    return m_tabManager->currentEditor();
}

CodeEditor* EditorController::findEditorByFileName(const QString& fileName) const
{
    return m_tabManager->findEditorByFileName(fileName);
}

void EditorController::newFile()
{
    CodeEditor* editor = m_tabManager->createEditor();
    editor->setFileName(Strings::untitled());
    connectEditorSignals(editor);
    SettingsManager::instance().applyToEditor(editor);
    applyDefaultSyntax(editor);
    m_tabManager->addEditor(editor, Strings::untitled());
    updateUndoRedoState();
    Logger::instance().info(QString("Created new file: %1").arg(editor->fileName()));
}

bool EditorController::saveFile()
{
    CodeEditor* editor = currentEditor();
    if (!editor)
    {
        return false;
    }

    if (editor->isReadOnlyMode())
    {
        QMessageBox::information(qobject_cast<QWidget*>(parent()), tr("Read Only"),
                                 tr("This file is in read-only mode.\nDisable read-only mode to save changes."));
        return false;
    }

    if (editor->fileName() == Strings::untitled() || editor->fileName().isEmpty())
    {
        return saveFileAs();
    }

    return saveEditor(editor, editor->fileName());
}

bool EditorController::saveFileAs()
{
    QPointer<CodeEditor> editor = currentEditor();
    if (!editor)
        return false;

    QString fileName = QFileDialog::getSaveFileName(qobject_cast<QWidget*>(parent()), tr("Save File As"), QString(), Strings::fileFilter());
    if (fileName.isEmpty())
        return false;

    if (!editor)
        return false;

    editor->setFileName(fileName);
    saveEditor(editor, fileName);
    m_tabManager->updateTabTitle(editor);
    return true;
}

bool EditorController::saveEditor(CodeEditor* editor, const QString& fileName)
{
    m_fileWatcherManager->unwatchFile(fileName);
    bool saved = m_fileManager->saveFile(fileName, editor);
    if (!saved)
    {
        m_fileWatcherManager->watchFile(fileName);
        QMessageBox::warning(qobject_cast<QWidget*>(parent()), tr("Error"), tr("Cannot save file: ") + fileName);
        return false;
    }
    m_tabManager->updateTabTitle(editor);
    m_fileWatcherManager->watchFile(fileName);
    emit fileSaved(fileName);
    return true;
}

void EditorController::saveAll()
{
    int skippedCount = 0;
    for (int i = 0; i < m_tabManager->count(); ++i)
    {
        QPointer<CodeEditor> editor = m_tabManager->editorAt(i);
        if (editor && editor->isModified())
        {
            if (editor->fileName() == Strings::untitled() || editor->fileName().isEmpty())
            {
                QString fileName =
                    QFileDialog::getSaveFileName(qobject_cast<QWidget*>(parent()), tr("Save File As"), QString(), Strings::fileFilter());
                if (fileName.isEmpty())
                {
                    skippedCount++;
                    continue;
                }

                if (!editor)
                {
                    skippedCount++;
                    continue;
                }

                editor->setFileName(fileName);
                saveEditor(editor, fileName);
                if (editor)
                    m_tabManager->updateTabTitle(editor);
            }
            else
            {
                saveEditor(editor, editor->fileName());
            }
        }
    }
    if (skippedCount > 0)
    {
        QMessageBox::information(qobject_cast<QWidget*>(parent()), tr("Save All"),
                                 tr("%1 file(s) were not saved because no location was selected.").arg(skippedCount));
    }
}

bool EditorController::maybeSave(CodeEditor* editor)
{
    if (!editor || !editor->isModified())
        return true;

    QMessageBox::StandardButton ret =
        QMessageBox::warning(qobject_cast<QWidget*>(parent()), tr("Save"), tr("The document has been modified.\nDo you want to save your changes?"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
    {
        if (editor->fileName() == Strings::untitled() || editor->fileName().isEmpty())
        {
            QString fileName = QFileDialog::getSaveFileName(qobject_cast<QWidget*>(parent()), tr("Save File As"), QString(), Strings::fileFilter());
            if (fileName.isEmpty())
                return false;
            editor->setFileName(fileName);
            saveEditor(editor, fileName);
            m_tabManager->updateTabTitle(editor);
            return true;
        }
        saveEditor(editor, editor->fileName());
        return true;
    }
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}

void EditorController::closeCurrentTab()
{
    QWidget* w = QApplication::focusWidget();
    CodeEditor* editor = nullptr;
    while (w)
    {
        editor = qobject_cast<CodeEditor*>(w);
        if (editor)
            break;
        w = w->parentWidget();
    }
    if (!editor)
    {
        editor = currentEditor();
    }
    if (!editor)
        return;
    int globalIdx = m_tabManager->indexOf(editor);
    if (globalIdx < 0)
        return;
    onTabCloseRequested(globalIdx);
}

bool EditorController::onTabCloseRequested(int index)
{
    if (index < 0)
        return true;

    CodeEditor* editor = m_tabManager->editorAt(index);
    if (!editor)
        return true;

    if (maybeSave(editor))
    {
        QString fileName = editor->fileName();
        m_tabManager->closeEditor(index);
        if (fileName != Strings::untitled() && !fileName.isEmpty())
        {
            m_fileWatcherManager->unwatchFile(fileName);
        }
        return true;
    }
    return false;
}

void EditorController::closeAllTabs()
{
    for (int i = m_tabManager->count() - 1; i >= 0; --i)
    {
        CodeEditor* editor = m_tabManager->editorAt(i);
        if (!editor)
            continue;
        if (!maybeSave(editor))
        {
            return;
        }
        QString fileName = editor->fileName();
        m_tabManager->closeEditor(i);
        if (fileName != Strings::untitled() && !fileName.isEmpty())
        {
            m_fileWatcherManager->unwatchFile(fileName);
        }
    }
}

void EditorController::undo()
{
    if (auto* editor = currentEditor())
        editor->undo();
}

void EditorController::redo()
{
    if (auto* editor = currentEditor())
        editor->redo();
}

void EditorController::cut()
{
    if (auto* editor = currentEditor())
        editor->cut();
}

void EditorController::copy()
{
    if (auto* editor = currentEditor())
        editor->copy();
}

void EditorController::paste()
{
    if (auto* editor = currentEditor())
        editor->paste();
}

void EditorController::selectAll()
{
    if (auto* editor = currentEditor())
        editor->selectAll();
}

void EditorController::deleteText()
{
    if (auto* editor = currentEditor())
        editor->removeSelectedText();
}

void EditorController::zoomIn()
{
    if (auto* editor = currentEditor())
        editor->zoomIn();
}

void EditorController::zoomOut()
{
    if (auto* editor = currentEditor())
        editor->zoomOut();
}

void EditorController::zoomReset()
{
    if (auto* editor = currentEditor())
        editor->zoomReset();
}

void EditorController::toggleWordWrap()
{
    bool enabled = m_actionManager->wordWrapAct()->isChecked();
    SettingsManager::instance().setWordWrap(enabled);
    for (int i = 0; i < m_tabManager->count(); ++i)
    {
        CodeEditor* editor = m_tabManager->editorAt(i);
        if (editor)
        {
            editor->setWrapMode(enabled ? QsciScintilla::WrapWord : QsciScintilla::WrapNone);
        }
    }
}

void EditorController::toggleReadOnly()
{
    CodeEditor* editor = currentEditor();
    if (!editor)
        return;

    bool newMode = !editor->isReadOnlyMode();
    editor->setReadOnlyMode(newMode);
    m_actionManager->readOnlyAct()->setChecked(newMode);
    m_tabManager->updateTabTitle(editor);
    updateUndoRedoState();

    if (newMode)
    {
        Logger::instance().info(QString("Read-only mode enabled for: %1").arg(editor->fileName()));
    }
    else
    {
        Logger::instance().info(QString("Read-only mode disabled for: %1").arg(editor->fileName()));
    }
}

void EditorController::toggleBookmark()
{
    CodeEditor* editor = currentEditor();
    if (editor)
    {
        editor->toggleBookmark();
    }
}

void EditorController::nextBookmark()
{
    CodeEditor* editor = currentEditor();
    if (editor)
    {
        editor->nextBookmark();
    }
}

void EditorController::prevBookmark()
{
    CodeEditor* editor = currentEditor();
    if (editor)
    {
        editor->prevBookmark();
    }
}

void EditorController::clearBookmarks()
{
    CodeEditor* editor = currentEditor();
    if (editor)
    {
        editor->clearBookmarks();
    }
}

void EditorController::insertSnippet()
{
    CodeEditor* editor = currentEditor();
    if (!editor || editor->isReadOnly())
        return;

    if (SnippetManager* sm = SnippetManager::instance())
    {
        QList<Snippet> snippets = sm->snippetsForLanguage(editor->syntax());
        if (snippets.isEmpty())
            return;

        QStringList names;
        for (const auto& s : snippets)
            names << s.name;

        bool ok;
        QString chosen = QInputDialog::getItem(qobject_cast<QWidget*>(parent()), tr("Insert Snippet"), tr("Select a snippet:"), names, 0, false, &ok);

        if (ok && !chosen.isEmpty())
        {
            Snippet snip = sm->snippetByName(chosen);
            if (!snip.prefix.isEmpty())
            {
                editor->insertSnippet(snip);
            }
        }
    }
}

void EditorController::toggleComment()
{
    CodeEditor* editor = currentEditor();
    if (editor && !editor->isReadOnly())
        editor->toggleComment();
}

void EditorController::updateUndoRedoState()
{
    CodeEditor* editor = currentEditor();
    if (editor)
    {
        bool readOnly = editor->isReadOnlyMode();
        m_actionManager->undoAct()->setEnabled(!readOnly && editor->isUndoAvailable());
        m_actionManager->redoAct()->setEnabled(!readOnly && editor->isRedoAvailable());
    }
    else
    {
        m_actionManager->undoAct()->setEnabled(false);
        m_actionManager->redoAct()->setEnabled(false);
    }
}

void EditorController::updateReadOnlyActionState()
{
    CodeEditor* editor = currentEditor();
    if (editor)
    {
        m_actionManager->readOnlyAct()->setChecked(editor->isReadOnlyMode());
    }
    else
    {
        m_actionManager->readOnlyAct()->setChecked(false);
    }
}

void EditorController::autoSave()
{
    if (!SettingsManager::instance().autoSaveEnabled())
    {
        return;
    }

    bool saveToOriginal = SettingsManager::instance().autoSaveToOriginalFile();

    for (int i = 0; i < m_tabManager->count(); ++i)
    {
        CodeEditor* editor = m_tabManager->editorAt(i);
        if (editor && editor->isModified())
        {
            QString fileName = editor->fileName();
            if (!fileName.isEmpty() && fileName != Strings::untitled())
            {
                if (saveToOriginal)
                {
                    if (m_fileManager->saveFile(fileName, editor))
                    {
                        m_fileWatcherManager->updateModificationTime(fileName);
                        m_tabManager->updateTabTitle(editor);
                        emit fileSaved(fileName);
                        Logger::instance().debug(QString("Auto-saved file: %1").arg(fileName));
                    }
                }
                else
                {
                    if (BackupManager::saveBackup(fileName, editor->text()))
                    {
                        Logger::instance().debug(QString("Auto-saved backup: %1").arg(fileName));
                    }
                }
            }
            else
            {
                if (BackupManager::saveBackup(fileName, editor->text()))
                {
                    Logger::instance().debug(QString("Auto-saved backup: %1").arg(fileName));
                }
            }
        }
    }
}

void EditorController::promptBackupRestore(const QString& filePath)
{
    if (!BackupManager::hasBackup(filePath))
    {
        return;
    }

    if (!BackupManager::isBackupNewer(filePath))
    {
        BackupManager::deleteBackup(filePath);
        return;
    }

    QMessageBox::StandardButton result =
        QMessageBox::question(qobject_cast<QWidget*>(parent()), tr("Auto-Save Backup Found"),
                              tr("A more recent auto-save backup exists for:\n%1\n\nDo you want to restore it?").arg(QFileInfo(filePath).fileName()),
                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (result == QMessageBox::Yes)
    {
        QString backupContent = BackupManager::getBackupContent(filePath);
        if (!backupContent.isEmpty())
        {
            CodeEditor* editor = findEditorByFileName(filePath);
            if (editor)
            {
                editor->setText(backupContent);
                editor->setModified(true);
                Logger::instance().info(QString("Restored auto-save backup: %1").arg(filePath));
            }
        }
        BackupManager::deleteBackup(filePath);
    }
    else if (result == QMessageBox::No)
    {
        BackupManager::deleteBackup(filePath);
    }
}

void EditorController::connectEditorSignals(CodeEditor* editor)
{
    connect(editor, &QsciScintilla::textChanged, this, &EditorController::updateUndoRedoState);
    connect(editor, &QsciScintilla::modificationChanged, this,
            [this](bool modified)
            {
                Q_UNUSED(modified)
                CodeEditor* e = qobject_cast<CodeEditor*>(sender());
                if (e)
                {
                    m_tabManager->updateTabTitle(e);
                }
            });
    emit editorConnected(editor);
}

CodeEditor* EditorController::openFile(const QString& fileName, const QString& encoding)
{
    CodeEditor* editor = m_tabManager->createEditor();

    if (!m_fileManager->loadFile(fileName, editor, encoding))
    {
        editor->setParent(nullptr);
        editor->deleteLater();
        return nullptr;
    }

    SettingsManager::instance().applyToEditor(editor);

    QString syntax = SettingsManager::instance().syntaxForFile(fileName);
    if (!syntax.isEmpty())
        editor->setSyntax(syntax);

    connectEditorSignals(editor);

    QFileInfo fileInfo(fileName);
    if (!fileInfo.isWritable())
        editor->setReadOnlyMode(true);

    m_tabManager->addEditor(editor, fileInfo.fileName());
    m_fileWatcherManager->watchFile(fileName);

    return editor;
}

void EditorController::reloadWithEncoding(const QString& encoding)
{
    CodeEditor* editor = currentEditor();
    if (!editor)
        return;

    QString fileName = editor->fileName();
    if (fileName.isEmpty() || fileName == Strings::untitled())
    {
        QMessageBox::warning(qobject_cast<QWidget*>(parent()), tr("Error"), tr("Cannot reopen an unsaved file."));
        return;
    }

    QList<int> bookmarks = editor->bookmarkLines();
    QString syntax = editor->syntax();
    if (!maybeSave(editor))
        return;

    int idx = m_tabManager->indexOf(editor);
    if (idx >= 0)
        m_tabManager->closeEditor(idx);

    CodeEditor* newEditor = openFile(fileName, encoding);
    if (newEditor)
    {
        if (!syntax.isEmpty())
            newEditor->setSyntax(syntax);
        newEditor->setBookmarks(bookmarks);
    }
}

void EditorController::saveWithEncoding(const QString& encoding)
{
    CodeEditor* editor = currentEditor();
    if (!editor)
        return;

    QString fileName = editor->fileName();
    if (fileName.isEmpty() || fileName == Strings::untitled())
    {
        if (saveFileAs())
        {
            editor = currentEditor();
            if (editor)
            {
                editor->setEncoding(encoding);
                saveEditor(editor, editor->fileName());
            }
        }
        return;
    }

    editor->setEncoding(encoding);
    saveEditor(editor, fileName);
}

void EditorController::applyDefaultSyntax(CodeEditor* editor) const
{
    auto langs = defaultSyntaxLanguages();
    int format = SettingsManager::instance().defaultFormat();
    if (format >= 0 && format < langs.size())
    {
        editor->setSyntax(langs[format]);
    }
}
