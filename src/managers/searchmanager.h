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
#ifndef SEARCHMANAGER_H
#define SEARCHMANAGER_H

#include <QObject>
#include <QString>
#include <QPointer>
#include <QMainWindow>
#include "finddialog.h"
#include "replacedialog.h"

class QsciScintilla;

class SearchManager : public QObject {
    Q_OBJECT

public:
    explicit SearchManager(QMainWindow *parent, QsciScintilla *editor = nullptr);
    ~SearchManager();

    void setEditor(QsciScintilla *editor);
    void showFindDialog();
    void showReplaceDialog();
    void findNext();
    void findPrevious();
    bool hasActiveSearch() const { return !m_lastSearchText.isEmpty(); }

    const QString& lastSearchText() const { return m_lastSearchText; }
    bool lastUseRegex() const { return m_lastUseRegex; }
    bool lastMatchCase() const { return m_lastMatchCase; }
    bool lastMatchWholeWord() const { return m_lastMatchWholeWord; }

private:
    void performSearch(bool searchForward);

    QMainWindow *m_parent;
    QPointer<FindDialog> m_findDialog;
    QPointer<ReplaceDialog> m_replaceDialog;
    QPointer<QsciScintilla> m_editor;

    QString m_lastSearchText;
    bool m_lastUseRegex = false;
    bool m_lastMatchCase = false;
    bool m_lastMatchWholeWord = false;
};

#endif // SEARCHMANAGER_H