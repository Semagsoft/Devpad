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
#include "searchmanager.h"
#include "finddialog.h"
#include "replacedialog.h"
#include <Qsci/qsciscintilla.h>
#include <QStatusBar>

SearchManager::SearchManager(QMainWindow *parent, QsciScintilla *editor)
    : QObject(parent), m_parent(parent), m_editor(editor) {
}

SearchManager::~SearchManager() {
}

void SearchManager::setEditor(QsciScintilla *editor) {
    m_editor = editor;
    if (m_findDialog) {
        m_findDialog->setEditor(editor);
    }
    if (m_replaceDialog) {
        m_replaceDialog->setEditor(editor);
    }
}

void SearchManager::showFindDialog() {
    if (!m_findDialog) {
        m_findDialog = new FindDialog(m_parent);
        connect(m_findDialog, &QDialog::finished, this, [this]() {
            if (!m_editor || !m_findDialog) return;
            m_lastSearchText = m_findDialog->searchText();
            m_lastMatchCase = m_findDialog->matchCase();
            m_lastMatchWholeWord = m_findDialog->matchWholeWord();
            m_lastUseRegex = m_findDialog->useRegex();
        });
        connect(m_findDialog, &FindDialog::searchFinished, this, [this](bool found) {
            if (!found && m_parent && m_parent->statusBar()) {
                m_parent->statusBar()->showMessage(tr("Text not found"), 3000);
            }
        });
    }
    if (m_editor) {
        m_findDialog->setEditor(m_editor);
        QString selectedText = m_editor->selectedText();
        if (!selectedText.isEmpty()) {
            m_findDialog->setSearchText(selectedText);
        }
    }
    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
}

void SearchManager::showReplaceDialog() {
    if (!m_replaceDialog) {
        m_replaceDialog = new ReplaceDialog(m_parent);
        connect(m_replaceDialog, &QDialog::finished, this, [this]() {
            if (!m_editor || !m_replaceDialog) return;
            m_lastSearchText = m_replaceDialog->findText();
            m_lastMatchCase = m_replaceDialog->matchCase();
            m_lastMatchWholeWord = m_replaceDialog->matchWholeWord();
            m_lastUseRegex = m_replaceDialog->useRegex();
        });
        connect(m_replaceDialog, &ReplaceDialog::searchFinished, this, [this](bool found) {
            if (!found && m_parent && m_parent->statusBar()) {
                m_parent->statusBar()->showMessage(tr("Text not found"), 3000);
            }
        });
    }
    if (m_editor) {
        m_replaceDialog->setEditor(m_editor);
        QString selectedText = m_editor->selectedText();
        if (!selectedText.isEmpty()) {
            m_replaceDialog->setFindText(selectedText);
        }
    }
    m_replaceDialog->show();
    m_replaceDialog->raise();
    m_replaceDialog->activateWindow();
}

void SearchManager::findNext() {
    if (!m_lastSearchText.isEmpty()) {
        performSearch(true);
    } else if (m_findDialog && m_findDialog->isVisible()) {
        m_findDialog->raise();
        m_findDialog->activateWindow();
    } else {
        showFindDialog();
    }
}

void SearchManager::findPrevious() {
    if (!m_lastSearchText.isEmpty()) {
        performSearch(false);
    } else if (m_findDialog && m_findDialog->isVisible()) {
        m_findDialog->raise();
        m_findDialog->activateWindow();
    } else {
        showFindDialog();
    }
}

void SearchManager::performSearch(bool searchForward) {
    if (!m_editor || m_lastSearchText.isEmpty()) return;

    bool found = m_editor->findFirst(m_lastSearchText, m_lastUseRegex,
                                     m_lastMatchCase, m_lastMatchWholeWord,
                                     true, searchForward);
    if (!found && m_parent && m_parent->statusBar()) {
        m_parent->statusBar()->showMessage(tr("Text not found"), 3000);
    }
}