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
#include "snippetengine.h"

#include "codeeditor.h"

#include <QKeyEvent>
#include <algorithm>
#include <limits>
#include <QTimer>

#include <Qsci/qsciscintilla.h>

namespace
{
// Numbered stops visit low to high; the final stop(s) ($0) visit last.
inline int stopRank(int number)
{
    return number == 0 ? std::numeric_limits<int>::max() : number;
}
} // namespace

SnippetEngine::SnippetEngine(CodeEditor* editor) : QObject(editor), m_editor(editor)
{
    connect(m_editor, static_cast<void (QsciScintillaBase::*)(int, int, const char *, int, int, int, int, int, int, int)>(&QsciScintillaBase::SCN_MODIFIED), this,
            [this](int pos, int mtype, const char*, int length, int, int, int, int, int, int)
            {
                if (!m_snippetActive)
                    return;
                if (!(mtype & (QsciScintillaBase::SC_MOD_INSERTTEXT | QsciScintillaBase::SC_MOD_DELETETEXT)))
                    return;
                // Bookkeeping always runs (even for our own mirror edits):
                // member positions must follow every edit. Only the text
                // sync is suppressed while we edit, to avoid feedback loops.
                adjustMemberPositionsForEdit(pos, mtype, length);
                if (m_applyingMirrorEdit)
                    return;
                // Defer the text sync out of the notification; mutating the
                // document from inside SCN_MODIFIED is silently dropped by
                // Scintilla. Bookkeeping above is notification-safe.
                QTimer::singleShot(0, this, &SnippetEngine::syncMirrorEdit);
            });
}

namespace
{
// UTF-8 byte size of the first \a units code units of \a s. If \a units
// splits a surrogate pair, back off to the pair boundary so the conversion
// stays valid (the caller passes snippet-internal offsets in code units).
int utf8ByteSizeForCodeUnitPrefix(const QString& s, int units)
{
    int u = qBound(0, units, s.size());
    if (u > 0 && u < s.size() && (s.at(u - 1).unicode() & 0xFC00u) == 0xD800u && (s.at(u).unicode() & 0xFC00u) == 0xDC00u)
        --u;
    return s.left(u).toUtf8().size();
}
} // namespace

void SnippetEngine::insertSnippet(const Snippet& snippet)
{
    if (m_editor->isReadOnly())
        return;

    Snippet::ExpandedSnippet expanded = snippet.expand();
    int line, col;
    m_editor->getCursorPosition(&line, &col);
    int pos = m_editor->positionFromLineIndex(line, col);
    enterSnippetMode(expanded, pos);
}

void SnippetEngine::enterSnippetMode(const Snippet::ExpandedSnippet& expanded, int insertPos, const QString& triggerText)
{
    if (m_snippetActive)
        clearSnippetMarkers();

    QString textToInsert = expanded.text;

    if (!triggerText.isEmpty())
    {
        int triggerStart = insertPos - static_cast<int>(triggerText.toUtf8().size());
        if (triggerStart >= 0)
        {
            int tl, tc, il, ic;
            m_editor->lineIndexFromPosition(triggerStart, &tl, &tc);
            m_editor->lineIndexFromPosition(insertPos, &il, &ic);
            m_editor->setSelection(tl, tc, il, ic);
            insertPos = triggerStart;
        }
    }

    m_editor->beginUndoAction();

    if (m_editor->hasSelectedText())
    {
        if (triggerText.isEmpty())
        {
            int selLineFrom, selColFrom, selLineTo, selColTo;
            m_editor->getSelection(&selLineFrom, &selColFrom, &selLineTo, &selColTo);
            insertPos = m_editor->positionFromLineIndex(selLineFrom, selColFrom);
        }
    }

    m_editor->insert(textToInsert);
    m_editor->endUndoAction();

    // All positions in this engine are Scintilla (UTF-8 byte) positions;
    // convert the snippet-internal code unit offsets once here.
    m_snippetStartPos = insertPos;
    m_snippetEndPos = insertPos + textToInsert.toUtf8().size();

    m_tabStopInfos.clear();
    for (const auto& ts : expanded.tabStops)
    {
        for (int offset : ts.positions)
        {
            TabStopInfo info;
            info.number = ts.number;
            info.pos = insertPos + utf8ByteSizeForCodeUnitPrefix(textToInsert, offset);
            info.length = utf8ByteSizeForCodeUnitPrefix(textToInsert, offset + ts.length) - utf8ByteSizeForCodeUnitPrefix(textToInsert, offset);
            info.defaultValue = ts.defaultValue;
            m_tabStopInfos.append(info);
        }
    }

    // Visit order: numbered stops ascending first, the final stop(s) ($0)
    // last. Mirrors of the same number stay grouped together in position
    // order, and entering at a low number must visit it before higher ones
    // even when it appears later in the text.
    std::sort(m_tabStopInfos.begin(), m_tabStopInfos.end(),
              [](const TabStopInfo& a, const TabStopInfo& b)
              {
                  int ra = stopRank(a.number);
                  int rb = stopRank(b.number);
                  if (ra != rb)
                      return ra < rb;
                  return a.pos < b.pos;
              });

    if (m_tabStopInfos.isEmpty())
    {
        int el, ec;
        m_editor->lineIndexFromPosition(m_snippetEndPos, &el, &ec);
        m_editor->setCursorPosition(el, ec);
        m_snippetActive = false;
        emit snippetModeChanged(false);
        return;
    }

    for (const auto& info : m_tabStopInfos)
    {
        if (info.length > 0)
        {
            // Zero-length stops cannot be indicator-marked (Scintilla drops
            // zero-width runs); they are positioned via the cursor anyway.
            int lf, cf, lt, ct;
            m_editor->lineIndexFromPosition(info.pos, &lf, &cf);
            m_editor->lineIndexFromPosition(info.pos + info.length, &lt, &ct);
            m_editor->fillIndicatorRange(lf, cf, lt, ct, SNIPPET_INDICATOR);
        }
    }

    for (int i = 0; i < m_tabStopInfos.size(); ++i)
    m_snippetActive = true;
    m_currentTabStopIdx = 0;
    emit snippetModeChanged(true);

    updateMirrorSnapshot();
    selectTabStopRange(m_tabStopInfos[0].pos, m_tabStopInfos[0].length);
}

void SnippetEngine::advanceTabStop()
{
    if (!m_snippetActive || m_tabStopInfos.isEmpty())
        return;

    int currentNumber = m_tabStopInfos[m_currentTabStopIdx].number;
    int nextIdx = -1;

    for (int i = 0; i < m_tabStopInfos.size(); ++i)
    {
        // Visit each stop number once: same-numbered mirrors are navigated
        // as one logical field, so skip past the whole group.
        if (m_tabStopInfos[i].number != currentNumber && stopRank(m_tabStopInfos[i].number) > stopRank(currentNumber))
        {
            nextIdx = i;
            break;
        }
    }

    if (nextIdx < 0)
    {
        int el, ec;
        m_editor->lineIndexFromPosition(m_snippetEndPos, &el, &ec);
        m_editor->setCursorPosition(el, ec);
        exitSnippetMode();
        return;
    }

    m_currentTabStopIdx = nextIdx;
    updateMirrorSnapshot();
    selectTabStopRange(m_tabStopInfos[nextIdx].pos, m_tabStopInfos[nextIdx].length);
}

void SnippetEngine::retreatTabStop()
{
    if (!m_snippetActive || m_tabStopInfos.isEmpty())
        return;

    int currentNumber = m_tabStopInfos[m_currentTabStopIdx].number;
    int prevIdx = -1;

    for (int i = m_tabStopInfos.size() - 1; i >= 0; --i)
    {
        if (m_tabStopInfos[i].number != currentNumber && stopRank(m_tabStopInfos[i].number) < stopRank(currentNumber))
        {
            prevIdx = i;
            break;
        }
    }

    if (prevIdx < 0)
    {
        int sl, sc;
        m_editor->lineIndexFromPosition(m_snippetStartPos, &sl, &sc);
        m_editor->setCursorPosition(sl, sc);
        return;
    }

    m_currentTabStopIdx = prevIdx;
    updateMirrorSnapshot();
    selectTabStopRange(m_tabStopInfos[prevIdx].pos, m_tabStopInfos[prevIdx].length);
}

QList<int> SnippetEngine::mirrorMemberIndices(int number) const
{
    QList<int> members;
    for (int i = 0; i < m_tabStopInfos.size(); ++i)
    {
        if (m_tabStopInfos[i].number == number)
            members.append(i);
    }
    return members;
}

void SnippetEngine::updateMirrorSnapshot()
{
    if (!m_snippetActive || m_currentTabStopIdx < 0 || m_currentTabStopIdx >= m_tabStopInfos.size())
    {
        m_mirrorSnapshot.clear();
        return;
    }
    const int number = m_tabStopInfos[m_currentTabStopIdx].number;
    const QList<int> members = mirrorMemberIndices(number);
    if (members.size() < 2)
    {
        m_mirrorSnapshot.clear();
        return;
    }
    // Snapshot from the first member. It may be empty; empty mirrors are
    // still valid members (their positions simply do not move while empty).
    const auto& info = m_tabStopInfos[members.first()];
    m_mirrorSnapshot = m_editor->text(info.pos, info.pos + info.length);
}

void SnippetEngine::syncMirrorEdit()
{
    if (!m_snippetActive || m_applyingMirrorEdit || m_currentTabStopIdx < 0 || m_currentTabStopIdx >= m_tabStopInfos.size())
    {
        return;
    }
    const int number = m_tabStopInfos[m_currentTabStopIdx].number;
    const QList<int> members = mirrorMemberIndices(number);
    if (members.size() < 2)
    {
        updateMirrorSnapshot();
        return;
    }
    if (m_mirrorSnapshot.isNull())
    {
        // Group has never been written; baseline it.
        updateMirrorSnapshot();
        return;
    }

    // Members that differ from the snapshot are candidates for "edited one".
    // Typically exactly one member differs; if the whole field was replaced
    // they all differ and any of them is a valid source.
    QList<int> differing;
    for (int idx : members)
    {
        const auto& info = m_tabStopInfos[idx];
        const QString current = m_editor->text(info.pos, info.pos + info.length);
        if (current != m_mirrorSnapshot)
            differing.append(idx);
    }

    if (differing.isEmpty())
    {
        updateMirrorSnapshot();
        return;
    }

    int edited = -1;
    if (differing.size() == 1 || differing.size() == members.size())
        edited = differing.first();
    else
    {
        // Some but not all members differ and more than one changed its text:
        // ambiguous, re-baseline.
        updateMirrorSnapshot();
        return;
    }

    const auto& editedInfo = m_tabStopInfos[edited];
    applyMirrorEdit(edited, m_editor->text(editedInfo.pos, editedInfo.pos + editedInfo.length), members, number);
}

void SnippetEngine::applyMirrorEdit(int changedIdx, const QString& after, const QList<int>& memberIdx, int /*number*/)
{
    const QString before = m_mirrorSnapshot;
    int prefix = 0;
    while (prefix < before.size() && prefix < after.size() && before.at(prefix) == after.at(prefix))
        ++prefix;
    const int suffixLimit = qMin(before.size() - prefix, after.size() - prefix);
    int suffix = 0;
    while (suffix < suffixLimit && before.at(before.size() - 1 - suffix) == after.at(after.size() - 1 - suffix))
        ++suffix;

    // The edit inside the mirror is (before[prefix .. size-suffix)) replaced
    // by after[prefix .. size-suffix)). Apply the same edit to the siblings,
    // rightmost first, so lower siblings' positions stay valid.
    const QString inserted = after.mid(prefix, after.size() - prefix - suffix);
    const int deleteLen = before.size() - prefix - suffix;

    m_applyingMirrorEdit = true;
    m_editor->beginUndoAction();
    for (int k = memberIdx.size() - 1; k >= 0; --k)
    {
        const int idx = memberIdx.at(k);
        if (idx == changedIdx)
            continue;
        const auto& info = m_tabStopInfos[idx];
        // Siblings should still equal `before`; if one was edited out of
        // band, skip it instead of corrupting it.
        QString sibText = m_editor->text(info.pos, info.pos + info.length);
        if (sibText != before)
            continue;
        const int start = info.pos + prefix;
        int l1 = 0, c1 = 0, l2 = 0, c2 = 0;
        m_editor->lineIndexFromPosition(start, &l1, &c1);
        m_editor->lineIndexFromPosition(start + deleteLen, &l2, &c2);
        m_editor->setSelection(l1, c1, l2, c2);
        // Same public path as user edits; nested edits from inside the
        // SCN_MODIFIED notification would be silently dropped, but we run
        // from a deferred timer, outside the notification.
        m_editor->replaceSelectedText(inserted);
    }
    m_editor->endUndoAction();
    m_applyingMirrorEdit = false;

    // All members were re-positioned by the edit notifications already
    // (positions are adjusted for every edit, ours included), so there is
    // nothing to realign. Just re-baseline the snapshot.
    updateMirrorSnapshot();
}

void SnippetEngine::exitSnippetMode()
{
    if (!m_snippetActive)
        return;

    clearSnippetMarkers();
    m_snippetActive = false;
    m_currentTabStopIdx = -1;
    m_tabStopInfos.clear();
    m_mirrorSnapshot.clear();
    emit snippetModeChanged(false);
}

void SnippetEngine::clearSnippetMarkers()
{
    // Clear across the whole document. The tracked m_snippetEndPos does not
    // follow edits the user makes inside the snippet, and stale markers would
    // otherwise linger as unreachable invisible state.
    int len = static_cast<int>(m_editor->SendScintilla(QsciScintillaBase::SCI_GETLENGTH));
    if (len > 0)
    {
        int lf, cf, lt, ct;
        m_editor->lineIndexFromPosition(0, &lf, &cf);
        m_editor->lineIndexFromPosition(len, &lt, &ct);
        m_editor->clearIndicatorRange(lf, cf, lt, ct, SNIPPET_INDICATOR);
    }
}

void SnippetEngine::selectTabStopRange(int pos, int len)
{
    if (len > 0)
    {
        int lf, cf, lt, ct;
        m_editor->lineIndexFromPosition(pos, &lf, &cf);
        m_editor->lineIndexFromPosition(pos + len, &lt, &ct);
        m_editor->setSelection(lf, cf, lt, ct);
    }
    else
    {
        int l, c;
        m_editor->lineIndexFromPosition(pos, &l, &c);
        m_editor->setCursorPosition(l, c);
    }
}

void SnippetEngine::adjustMemberPositionsForEdit(int pos, int mtype, int length)
{
    const bool isInsert = (mtype & QsciScintillaBase::SC_MOD_INSERTTEXT) != 0;
    for (auto& info : m_tabStopInfos)
    {
        if (isInsert)
        {
            const int end = info.pos + info.length;
            if (info.length == 0 && info.pos == pos)
            {
                // Zero-width member exactly at the insertion point: it grows
                // to cover the inserted text.
                info.length += length;
            }
            else if (info.pos >= pos)
            {
                // Member lies at or after the insertion start: it shifts.
                info.pos += length;
            }
            else if (end <= pos)
            {
                // Member ends before the insertion point: unchanged.
            }
            else
            {
                // Insertion inside the member: it grows.
                info.length += length;
            }
        }
        else // SC_MOD_DELETETEXT
        {
            if (info.pos >= pos + length)
            {
                info.pos -= length;
            }
            else
            {
                const int end = info.pos + info.length;
                const int overlapStart = std::max(info.pos, pos);
                const int overlapEnd = std::min(end, pos + length);
                const int overlap = std::max(0, overlapEnd - overlapStart);
                info.length -= overlap;
                if (info.pos > pos)
                    info.pos = pos; // member started inside the deleted region
            }
        }
    }
}

bool SnippetEngine::handleKeyPress(int key, Qt::KeyboardModifiers modifiers)
{
    if (!m_snippetActive)
        return false;

    if (key == Qt::Key_Tab && !(modifiers & ~Qt::ShiftModifier))
    {
        if (modifiers & Qt::ShiftModifier)
            retreatTabStop();
        else
            advanceTabStop();
        return true;
    }

    if (key == Qt::Key_Escape)
    {
        exitSnippetMode();
        return true;
    }

    return false;
}
