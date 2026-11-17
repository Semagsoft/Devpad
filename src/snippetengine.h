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
#ifndef SNIPPETENGINE_H
#define SNIPPETENGINE_H

#include "snippet.h"

#include <QList>
#include <QObject>

class CodeEditor;

class SnippetEngine : public QObject
{
    Q_OBJECT

public:
    explicit SnippetEngine(CodeEditor* editor);

    void insertSnippet(const Snippet& snippet);
    bool isActive() const
    {
        return m_snippetActive;
    }
    void exitSnippetMode();

    bool handleKeyPress(int key, Qt::KeyboardModifiers modifiers);

    static constexpr int SNIPPET_INDICATOR = 21;

signals:
    void snippetModeChanged(bool active);

private:
    struct TabStopInfo
    {
        int number = 0;
        int pos = 0;
        int length = 0;
        QString defaultValue;
    };

    void enterSnippetMode(const Snippet::ExpandedSnippet& expanded, int insertPos, const QString& triggerText = QString());
    void advanceTabStop();
    void retreatTabStop();
    void clearSnippetMarkers();
    void selectTabStopRange(int pos, int len);
    // Adjust member ranges in place for an edit at byte position @a pos
    // with type @a mtype and @a length bytes (SC_MOD_INSERTTEXT/DELETETEXT).
    void adjustMemberPositionsForEdit(int pos, int mtype, int length);

    // ${1}-style mirror tab stops: same-numbered stops are one logical
    // field. When the user edits a member that is currently selected, the
    // same edit is propagated to the other members.
    QList<int> mirrorMemberIndices(int number) const;
    void updateMirrorSnapshot();
    void syncMirrorEdit();
    void applyMirrorEdit(int changedIdx, const QString& after, const QList<int>& memberIdx, int number);

    CodeEditor* m_editor;

    bool m_snippetActive = false;
    int m_currentTabStopIdx = -1;
    int m_snippetStartPos = 0;
    int m_snippetEndPos = 0;
    QList<TabStopInfo> m_tabStopInfos;
    // Text of the current mirror group captured before each edit, used to
    // derive the edit delta. Empty when the current stop is not a mirror.
    QString m_mirrorSnapshot;
    bool m_applyingMirrorEdit = false;
};

#endif
