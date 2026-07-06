#include "snippetengine.h"
#include "codeeditor.h"

#include <QKeyEvent>

#include <Qsci/qsciscintilla.h>
#include <algorithm>

SnippetEngine::SnippetEngine(CodeEditor* editor)
    : QObject(editor), m_editor(editor)
{
}

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
        int triggerStart = insertPos - triggerText.length();
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

    m_snippetStartPos = insertPos;
    m_snippetEndPos = insertPos + textToInsert.length();

    m_tabStopInfos.clear();
    for (const auto& ts : expanded.tabStops)
    {
        for (int offset : ts.positions)
        {
            TabStopInfo info;
            info.number = ts.number;
            info.pos = insertPos + offset;
            info.length = ts.length;
            info.defaultValue = ts.defaultValue;
            m_tabStopInfos.append(info);
        }
    }

    std::sort(m_tabStopInfos.begin(), m_tabStopInfos.end(),
              [](const TabStopInfo& a, const TabStopInfo& b) { return a.pos < b.pos; });

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
        if (info.length > 0 || info.defaultValue.isEmpty())
        {
            int len = qMax(info.length, 1);
            int lf, cf, lt, ct;
            m_editor->lineIndexFromPosition(info.pos, &lf, &cf);
            m_editor->lineIndexFromPosition(info.pos + len, &lt, &ct);
            m_editor->fillIndicatorRange(lf, cf, lt, ct, SNIPPET_INDICATOR);
        }
    }

    m_snippetActive = true;
    m_currentTabStopIdx = 0;
    emit snippetModeChanged(true);

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
        if (m_tabStopInfos[i].number > currentNumber)
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
        if (m_tabStopInfos[i].number < currentNumber)
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
    selectTabStopRange(m_tabStopInfos[prevIdx].pos, m_tabStopInfos[prevIdx].length);
}

void SnippetEngine::exitSnippetMode()
{
    if (!m_snippetActive)
        return;

    clearSnippetMarkers();
    m_snippetActive = false;
    m_currentTabStopIdx = -1;
    m_tabStopInfos.clear();
    emit snippetModeChanged(false);
}

void SnippetEngine::clearSnippetMarkers()
{
    if (m_snippetEndPos > m_snippetStartPos)
    {
        int lf, cf, lt, ct;
        m_editor->lineIndexFromPosition(m_snippetStartPos, &lf, &cf);
        m_editor->lineIndexFromPosition(m_snippetEndPos, &lt, &ct);
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

void SnippetEngine::recalculateTabStopPositions()
{
    for (int i = 0; i < m_tabStopInfos.size(); ++i)
    {
        auto& info = m_tabStopInfos[i];
        int start = static_cast<int>(m_editor->SendScintilla(QsciScintillaBase::SCI_INDICATORSTART, static_cast<unsigned long>(SNIPPET_INDICATOR), static_cast<unsigned long>(info.pos)));
        int end = static_cast<int>(m_editor->SendScintilla(QsciScintillaBase::SCI_INDICATOREND, static_cast<unsigned long>(SNIPPET_INDICATOR), static_cast<unsigned long>(info.pos)));
        if (start >= 0 && end > start)
        {
            info.pos = start;
            info.length = end - start;
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
