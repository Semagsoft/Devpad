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
#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QPointer>
#include <QTabWidget>
#include <QTimer>

#include <Qsci/qsciscintilla.h>

EditorController::EditorController(TabManager* tabManager, FileManager* fileManager, FileWatcherManager* watcher, ActionManager* actionManager,
                                   QTabWidget* tabWidget, QObject* parent)
    : QObject(parent), m_tabManager(tabManager), m_fileManager(fileManager), m_fileWatcherManager(watcher), m_actionManager(actionManager),
      m_tabWidget(tabWidget)
{
    // Remote (ssh://) saves complete asynchronously; the editor and tab
    // title are updated here, once the upload actually succeeded.
    connect(m_fileManager, &FileManager::remoteSaveFinished, this,
            [this](bool ok, const QString& fileName, const QString& errorDetail)
            {
                if (ok)
                {
                    if (CodeEditor* editor = findEditorByFileName(fileName))
                    {
                        m_tabManager->updateTabTitle(editor);
                        deleteUntitledBackup(editor);
                    }
                    emit fileSaved(fileName);
                    return;
                }
                QString detail = errorDetail.trimmed();
                if (detail.isEmpty())
                    detail = fileName;
                QMessageBox::warning(qobject_cast<QWidget*>(QObject::parent()), tr("Error"), tr("Cannot save file: %1").arg(detail));
            });
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

    if (!saveEditor(editor.data(), fileName))
    {
        QMessageBox::warning(qobject_cast<QWidget*>(parent()), tr("Save Failed"),
                             tr("Could not save \"%1\". Changes were not saved.").arg(QFileInfo(fileName).fileName()));
        return false;
    }
    if (editor)
    {
        editor->setFileName(fileName);
        m_tabManager->updateTabTitle(editor);
    }
    return true;
}

bool EditorController::saveEditor(CodeEditor* editor, const QString& fileName)
{
    if (checkSaveConflict(editor, fileName))
    {
        bool isRemote = fileName.startsWith(QStringLiteral("ssh://"), Qt::CaseInsensitive);
        if (!isRemote)
            m_fileWatcherManager->unwatchFile(fileName);
        bool saved = m_fileManager->saveFile(fileName, editor);
        if (!saved)
        {
            if (!isRemote)
                m_fileWatcherManager->watchFile(fileName);
            QString detail = m_fileManager->lastError();
            if (detail.isEmpty())
                detail = fileName;
            QMessageBox::warning(qobject_cast<QWidget*>(parent()), tr("Error"), tr("Cannot save file: %1").arg(detail));
            return false;
        }
        if (isRemote)
        {
            // The upload runs asynchronously; tab-title refresh, the fileSaved
            // signal and the save-failed dialog are delivered by the
            // remoteSaveFinished handler connected in the constructor.
            return true;
        }
        m_tabManager->updateTabTitle(editor);
        m_fileWatcherManager->watchFile(fileName);
        deleteUntitledBackup(editor);
        emit fileSaved(fileName);
        Logger::instance().debug(QString("Saved file: %1").arg(fileName));
        return true;
    }
    return false;
}

bool EditorController::checkSaveConflict(CodeEditor* editor, const QString& fileName)
{
    if (!editor)
        return false;

    if (fileName.isEmpty() || fileName == Strings::untitled())
        return true;

    const QDateTime& known = editor->diskModifiedAt();
    if (!known.isValid())
        return true; // No snapshot (never loaded from disk) — nothing to compare.

    QFileInfo info(fileName);
    if (!info.exists())
        return true; // Deleted externally — allow saving to recreate it.

    if (info.lastModified() == known)
        return true;

    Logger::instance().warning(QString("File changed on disk since load: %1").arg(fileName));
    QMessageBox::StandardButton ret = QMessageBox::warning(
        qobject_cast<QWidget*>(parent()), tr("File Changed on Disk"),
        tr("%1 has been modified by another program.\n\nOverwrite the external changes?").arg(info.fileName()), QMessageBox::Yes | QMessageBox::No);
    return ret == QMessageBox::Yes;
}

void EditorController::saveAll()
{
    int skippedCount = 0;
    int failedCount = 0;
    QStringList failedFiles;
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

                if (!saveEditor(editor, fileName))
                {
                    failedCount++;
                    failedFiles << QFileInfo(fileName).fileName();
                }
                else if (editor)
                {
                    editor->setFileName(fileName);
                    m_tabManager->updateTabTitle(editor);
                }
            }
            else
            {
                if (!saveEditor(editor, editor->fileName()))
                {
                    failedCount++;
                    failedFiles << QFileInfo(editor->fileName()).fileName();
                }
            }
        }
    }
    QString message;
    if (skippedCount > 0)
    {
        message = tr("%1 file(s) were not saved because no location was selected.").arg(skippedCount);
    }
    if (failedCount > 0)
    {
        if (!message.isEmpty())
            message += QLatin1Char('\n');
        message += tr("%1 file(s) failed to save: %2").arg(failedCount).arg(failedFiles.join(QLatin1String(", ")));
    }
    if (!message.isEmpty())
    {
        QMessageBox::warning(qobject_cast<QWidget*>(parent()), tr("Save All"), message);
    }
}

bool EditorController::maybeSave(CodeEditor* editor)
{
    if (!editor || !editor->isModified())
        return true;

    // A remote save may still be in flight (uploads complete asynchronously).
    // Wait, bounded, so quitting/closing doesn't kill the upload mid-transfer
    // and the modified flag settles before the user is prompted.
    const QString waitName = editor->fileName();
    if (waitName.startsWith(QStringLiteral("ssh://"), Qt::CaseInsensitive) && m_fileManager->hasPendingRemoteSave(waitName))
    {
        QEventLoop waitLoop;
        QTimer timeout;
        timeout.setSingleShot(true);
        timeout.setInterval(30000); // bounded wait; uploads stream asynchronously and may outlast this
        QObject::connect(&timeout, &QTimer::timeout, &waitLoop, &QEventLoop::quit);
        QObject::connect(m_fileManager, &FileManager::remoteSaveFinished, &waitLoop,
                         [&waitLoop, &waitName](bool, const QString& fileName, const QString&)
                         {
                             if (fileName == waitName)
                                 waitLoop.quit();
                         });
        timeout.start();
        waitLoop.exec();
        if (!editor->isModified())
            return true;
    }

    QPointer<CodeEditor> guard(editor);
    QMessageBox::StandardButton ret =
        QMessageBox::warning(qobject_cast<QWidget*>(parent()), tr("Save"), tr("The document has been modified.\nDo you want to save your changes?"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (!guard)
        return false; // Editor was destroyed while the dialog ran its nested event loop.

    if (ret == QMessageBox::Save)
    {
        if (guard->fileName() == Strings::untitled() || guard->fileName().isEmpty())
        {
            QString fileName = QFileDialog::getSaveFileName(qobject_cast<QWidget*>(parent()), tr("Save File As"), QString(), Strings::fileFilter());
            if (fileName.isEmpty())
                return false;
            if (!guard)
                return false;
            if (!saveEditor(guard.data(), fileName))
            {
                QMessageBox::warning(qobject_cast<QWidget*>(parent()), tr("Save Failed"),
                                     tr("Could not save \"%1\". Changes were not saved.").arg(QFileInfo(fileName).fileName()));
                return false;
            }
            if (guard)
            {
                guard->setFileName(fileName);
                m_tabManager->updateTabTitle(guard);
            }
            return true;
        }
        if (!saveEditor(guard.data(), guard->fileName()))
        {
            QMessageBox::warning(qobject_cast<QWidget*>(parent()), tr("Save Failed"),
                                 tr("Could not save \"%1\". Changes were not saved.").arg(QFileInfo(guard->fileName()).fileName()));
            return false;
        }
        return true;
    }
    if (ret == QMessageBox::Cancel)
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

    QPointer<CodeEditor> guard(editor);
    if (maybeSave(editor))
    {
        if (!guard)
            return true; // Editor destroyed during the save prompt.
        QString fileName = guard->fileName();
        // If the user discarded the document (or saved it before closing),
        // drop any untitled backup so it is not resurrected next launch.
        if (fileName == Strings::untitled() || fileName.isEmpty())
            deleteUntitledBackup(guard);
        bool closed = m_tabManager->closeEditor(index);
        if (!closed && m_tabManager->isTabPinned(guard))
        {
            QMessageBox::information(qobject_cast<QWidget*>(parent()), tr("Pinned Tab"),
                                     tr("This tab is pinned. Unpin it before closing."));
            return true; // Handled (user informed); tab stays open.
        }
        if (closed && fileName != Strings::untitled() && !fileName.isEmpty())
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
        QPointer<CodeEditor> editor = m_tabManager->editorAt(i);
        if (!editor)
            continue;
        if (!maybeSave(editor))
        {
            return;
        }
        if (!editor)
            continue; // Editor destroyed during the save prompt.
        QString fileName = editor->fileName();
        if (fileName == Strings::untitled() || fileName.isEmpty())
            deleteUntitledBackup(editor);
        bool closed = m_tabManager->closeEditor(i);
        if (closed && fileName != Strings::untitled() && !fileName.isEmpty())
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

    {
        SnippetManager& sm = SnippetManager::instance();
        QList<Snippet> snippets = sm.snippetsForLanguage(editor->syntax());
        if (snippets.isEmpty())
            return;

        QStringList names;
        for (const auto& s : snippets)
            names << s.name;

        bool ok;
        QString chosen = QInputDialog::getItem(qobject_cast<QWidget*>(parent()), tr("Insert Snippet"), tr("Select a snippet:"), names, 0, false, &ok);

        if (ok && !chosen.isEmpty())
        {
            Snippet snip = sm.snippetByName(chosen);
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
        m_actionManager->insertSnippetAct()->setEnabled(!readOnly);
        m_actionManager->toggleCommentAct()->setEnabled(!readOnly);
    }
    else
    {
        m_actionManager->undoAct()->setEnabled(false);
        m_actionManager->redoAct()->setEnabled(false);
        m_actionManager->insertSnippetAct()->setEnabled(false);
        m_actionManager->toggleCommentAct()->setEnabled(false);
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

    for (int i = m_tabManager->count() - 1; i >= 0; --i)
    {
        QPointer<CodeEditor> editor = m_tabManager->editorAt(i);
        if (editor && editor->isModified())
        {
            QString fileName = editor->fileName();
            if (!fileName.isEmpty() && fileName != Strings::untitled())
            {
                if (saveToOriginal)
                {
                    // Never let background auto-save silently overwrite external
                    // changes made since the file was loaded.
                    if (editor->diskModifiedAt().isValid() && QFileInfo(fileName).lastModified() != editor->diskModifiedAt())
                    {
                        Logger::instance().warning(QString("Auto-save skipped (file changed on disk): %1").arg(fileName));
                        continue;
                    }
                    if (m_fileManager->saveFile(fileName, editor.data()))
                    {
                        if (fileName.startsWith(QStringLiteral("ssh://"), Qt::CaseInsensitive))
                        {
                            // Remote uploads complete asynchronously; the tab
                            // title and fileSaved signal are handled by the
                            // remoteSaveFinished handler.
                            Logger::instance().debug(QString("Auto-save upload started: %1").arg(fileName));
                        }
                        else
                        {
                            // QSaveFile replaces the file (rename), killing the old
                            // inotify watch — re-add it so external changes are
                            // still detected.
                            m_fileWatcherManager->watchFile(fileName);
                            m_fileWatcherManager->updateModificationTime(fileName);
                            if (editor)
                                m_tabManager->updateTabTitle(editor);
                            emit fileSaved(fileName);
                            Logger::instance().debug(QString("Auto-saved file: %1").arg(fileName));
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
            else
            {
                QString key = untitledBackupKey(editor);
                if (!key.isEmpty() && BackupManager::saveBackup(key, editor->text()))
                {
                    Logger::instance().debug(QString("Auto-saved untitled backup: %1").arg(key));
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
        CodeEditor* editor = findEditorByFileName(filePath);
        if (editor)
        {
            // Restore even when the backup is empty: the empty text may be
            // exactly what the user typed before the crash.
            editor->setText(backupContent);
            editor->setModified(true);
            Logger::instance().info(QString("Restored auto-save backup: %1").arg(filePath));
        }
        else
        {
            Logger::instance().warning(QString("Auto-save backup for %1 could not be restored: no open editor for the file").arg(filePath));
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

bool EditorController::reloadFileInPlace(CodeEditor* editor, const QString& encoding, const QList<int>& bookmarks)
{
    if (!editor)
        return false;

    const QString fileName = editor->fileName();
    if (fileName.isEmpty() || fileName == Strings::untitled())
        return false;

    if (!m_fileManager->loadFile(fileName, editor, encoding))
    {
        QMessageBox::warning(qobject_cast<QWidget*>(parent()), tr("Error"), tr("Cannot reload file: ") + fileName);
        return false;
    }

    SettingsManager::instance().applyToEditor(editor);

    QString syntax = editor->syntax(); // Preserve the current format selection.
    if (syntax.isEmpty())
        syntax = SettingsManager::instance().syntaxForFile(fileName);
    if (!syntax.isEmpty())
        editor->setSyntax(syntax);

    QFileInfo fileInfo(fileName);
    if (fileInfo.exists() && !fileInfo.isWritable())
        editor->setReadOnlyMode(true);

    m_fileWatcherManager->watchFile(fileName);
    m_fileWatcherManager->updateModificationTime(fileName);

    // Only replace bookmark markers when the caller provides them (callers
    // that reload in place from elsewhere replace them afterwards).
    if (!bookmarks.isEmpty())
        editor->setBookmarks(bookmarks);

    m_tabManager->updateTabTitle(editor);
    return true;
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
    if (!maybeSave(editor))
        return;

    QPointer<CodeEditor> guard(editor);
    if (!guard)
        return; // Editor destroyed while the save prompt ran.

    reloadFileInPlace(editor, encoding);
    QPointer<CodeEditor> guard2(editor);
    if (guard2)
        guard2->setBookmarks(bookmarks); // Replace stale markers unconditionally.
    updateUndoRedoState();
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

void EditorController::retargetEditorFile(const QString& oldPath, const QString& newPath)
{
    for (int i = 0; i < m_tabManager->count(); ++i)
    {
        QPointer<CodeEditor> editor = m_tabManager->editorAt(i);
        if (editor && editor->fileName() == oldPath)
        {
            editor->setFileName(newPath);
            editor->setDiskModifiedAt(QFileInfo(newPath).lastModified());
            m_tabManager->updateTabTitle(editor.data());
            m_fileWatcherManager->unwatchFile(oldPath);
            m_fileWatcherManager->watchFile(newPath);
        }
    }
}

void EditorController::handleFileDeleted(const QString& path)
{
    for (int i = m_tabManager->count() - 1; i >= 0; --i)
    {
        QPointer<CodeEditor> editor = m_tabManager->editorAt(i);
        if (editor && editor->fileName() == path)
        {
            m_fileWatcherManager->unwatchFile(path);
        }
    }
}

QString EditorController::untitledBackupKey(CodeEditor* editor)
{
    if (!editor)
        return QString();
    QVariant existing = editor->property("untitledBackupKey");
    if (existing.isValid() && !existing.toString().isEmpty())
        return existing.toString();
    QString key = QStringLiteral("untitled-%1-%2").arg(QCoreApplication::applicationPid()).arg(m_untitledBackupSeq++);
    editor->setProperty("untitledBackupKey", key);
    return key;
}

void EditorController::discardUntitledBackup(CodeEditor* editor)
{
    deleteUntitledBackup(editor);
}

void EditorController::deleteUntitledBackup(CodeEditor* editor)
{
    if (!editor)
        return;
    QVariant existing = editor->property("untitledBackupKey");
    if (existing.isValid())
    {
        BackupManager::deleteBackup(existing.toString());
        editor->setProperty("untitledBackupKey", QVariant());
    }
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
