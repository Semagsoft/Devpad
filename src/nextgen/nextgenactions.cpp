/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "nextgenactions.h"

#include "managers/settingsmanager.h"
#include "nextgen/primoeditor.h"

#include <QApplication>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

NextgenActions::NextgenActions(QObject* parent) : QObject(parent), m_editor(nullptr)
{
    connect(&SettingsManager::instance(), &SettingsManager::settingsChanged, this, [this](){
        emit showToolbarChanged();
        emit showStatusbarChanged();
        emit showMenuBarChanged();
        emit recentFilesChanged();
    });
}
NextgenActions::NextgenActions(PrimoEditor* editor, QObject* parent) : QObject(parent), m_editor(editor)
{
    connect(&SettingsManager::instance(), &SettingsManager::settingsChanged, this, [this](){
        emit showToolbarChanged();
        emit showStatusbarChanged();
        emit showMenuBarChanged();
        emit recentFilesChanged();
    });
}
QObject* NextgenActions::editorObj() const { return m_editor; }
void NextgenActions::setEditorObj(QObject* obj) {
    auto* ed = qobject_cast<PrimoEditor*>(obj);
    if (ed == m_editor) return;
    m_editor = ed;
    emit editorChanged();
    emit currentEncodingChanged();
}

bool NextgenActions::showToolbar() const { return SettingsManager::instance().showToolbar(); }
void NextgenActions::setShowToolbar(bool v) { SettingsManager::instance().setShowToolbar(v); emit showToolbarChanged(); }
bool NextgenActions::showStatusbar() const { return SettingsManager::instance().showStatusbar(); }
void NextgenActions::setShowStatusbar(bool v) { SettingsManager::instance().setShowStatusbar(v); emit showStatusbarChanged(); }
bool NextgenActions::showMenuBar() const { return SettingsManager::instance().showMenuBar(); }
void NextgenActions::setShowMenuBar(bool v) { SettingsManager::instance().setShowMenuBar(v); emit showMenuBarChanged(); }
QStringList NextgenActions::recentFiles() const { return SettingsManager::instance().recentFiles(); }
QStringList NextgenActions::recentFolders() const { return SettingsManager::instance().recentFolders(); }
QString NextgenActions::currentEncoding() const { return m_editor ? m_editor->encoding() : QStringLiteral("UTF-8"); }
void NextgenActions::refreshRecent() { emit recentFilesChanged(); }
void NextgenActions::clearRecentFiles() { SettingsManager::instance().clearRecentFiles(); emit recentFilesChanged(); }

void NextgenActions::newFile() { if (!m_editor) return; m_editor->setText(QString()); m_editor->setFilePath(QString()); m_editor->setCursorPosition(0,0); emit currentEncodingChanged(); }
void NextgenActions::openFile(const QString& path) {
    if (!m_editor) return;
    QString p = path;
    if (p.isEmpty()) { emit requestOpenFileDialog(); return; }
    m_editor->loadFile(p);
    SettingsManager::instance().addRecentFile(p);
    emit recentFilesChanged(); emit currentEncodingChanged();
}
void NextgenActions::openFileDialog() { emit requestOpenFileDialog(); }
void NextgenActions::openFolderDialog() { emit requestOpenFolderDialog(); }
void NextgenActions::saveFile() { if (!m_editor) return; if (m_editor->filePath().isEmpty()) { emit requestSaveAsDialog(); return; } m_editor->save(); }
void NextgenActions::saveFileAs() { emit requestSaveAsDialog(); }
void NextgenActions::saveFileAsDialog() { emit requestSaveAsDialog(); }
void NextgenActions::closeFile() { newFile(); }
void NextgenActions::exitApp() { QApplication::quit(); }
void NextgenActions::undo() { if (m_editor && !m_editor->isReadOnly()) { /* PrimoDocument undo not yet separate – placeholder */ emit showMessage(QStringLiteral("Undo not yet implemented in primoEditor")); } }
void NextgenActions::redo() { emit showMessage(QStringLiteral("Redo not yet implemented")); }
void NextgenActions::cut() { emit showMessage(QStringLiteral("Cut via Ctrl+X in editor")); }
void NextgenActions::copy() { emit showMessage(QStringLiteral("Copy via Ctrl+C in editor")); }
void NextgenActions::paste() { emit showMessage(QStringLiteral("Paste via Ctrl+V in editor")); }
void NextgenActions::selectAll() { if(m_editor){ m_editor->setCursorPosition(0,0); /* select all placeholder */ } }
void NextgenActions::find() { emit requestFindDialog(); }
void NextgenActions::replace() { emit requestReplaceDialog(); }
void NextgenActions::findNext() { emit showMessage(QStringLiteral("Find Next placeholder")); }
void NextgenActions::findPrevious() { emit showMessage(QStringLiteral("Find Previous placeholder")); }
void NextgenActions::goToLine() { emit requestGoToLineDialog(); }
void NextgenActions::toggleBookmark() { if(m_editor) m_editor->toggleBookmarkCurrent(); }
void NextgenActions::nextBookmark() { emit showMessage(QStringLiteral("Next bookmark – use gutter click")); }
void NextgenActions::prevBookmark() { emit showMessage(QStringLiteral("Prev bookmark – use gutter click")); }
void NextgenActions::clearBookmarks() { if(m_editor) m_editor->clearBookmarks(); }
void NextgenActions::zoomIn() { if(!m_editor) return; QFont f=m_editor->font(); f.setPointSize(f.pointSize()+1); m_editor->setFont(f); SettingsManager::instance().setDefaultFontFamily(f.family()); SettingsManager::instance().setDefaultFontSize(f.pointSize());}
void NextgenActions::zoomOut() { if(!m_editor) return; QFont f=m_editor->font(); f.setPointSize(qMax(6,f.pointSize()-1)); m_editor->setFont(f); SettingsManager::instance().setDefaultFontSize(f.pointSize());}
void NextgenActions::zoomReset() { if(!m_editor) return; QFont f=SettingsManager::instance().defaultFont(); m_editor->setFont(f); }
void NextgenActions::toggleFullScreen() { emit showMessage(QStringLiteral("Fullscreen toggle via View menu – use F11")); }
void NextgenActions::toggleWordWrap() { if(!m_editor) return; m_editor->setWordWrap(!m_editor->wordWrap()); SettingsManager::instance().setWordWrap(m_editor->wordWrap()); }
void NextgenActions::showAbout() { emit requestAboutDialog(); }
void NextgenActions::showOptions() { emit requestOptionsDialog(); }
void NextgenActions::reopenWithEncoding(const QString& enc) { if(m_editor){ m_editor->setEncoding(enc); // reload would need FileService re-load
        emit currentEncodingChanged(); emit showMessage(QStringLiteral("Reopen with ")+enc); } }
void NextgenActions::saveWithEncoding(const QString& enc) { if(m_editor){ m_editor->setEncoding(enc); m_editor->save(); emit currentEncodingChanged(); } }
void NextgenActions::openRecentFile(const QString& path) { openFile(path); }
