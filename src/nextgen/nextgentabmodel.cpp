/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "nextgentabmodel.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>

NextgenTabModel::NextgenTabModel(QObject* parent) : QObject(parent)
{
    m_tabs.append(QString()); // Untitled
    m_tabPanes.append(0);
}

QStringList NextgenTabModel::tabs() const
{
    return m_tabs;
}
int NextgenTabModel::currentIndex() const
{
    return m_current;
}
void NextgenTabModel::setCurrentIndex(int idx)
{
    if (idx < 0 || idx >= m_tabs.size())
        return;
    if (m_current == idx)
        return;
    m_current = idx;
    emit currentIndexChanged();
}
int NextgenTabModel::pane2Index() const
{
    return m_pane2Current;
}
void NextgenTabModel::setPane2Index(int idx)
{
    if (idx < 0 || idx >= m_tabs.size())
        return;
    if (m_pane2Current == idx)
        return;
    m_pane2Current = idx;
    emit pane2IndexChanged();
}
bool NextgenTabModel::splitVisible() const
{
    return m_splitVisible;
}
void NextgenTabModel::setSplitVisible(bool v)
{
    if (m_splitVisible != v)
    {
        m_splitVisible = v;
        emit splitVisibleChanged();
    }
}
int NextgenTabModel::count() const
{
    return m_tabs.size();
}

QString NextgenTabModel::tabAt(int idx) const
{
    if (idx < 0 || idx >= m_tabs.size())
        return {};
    return m_tabs.at(idx);
}
QString NextgenTabModel::displayName(int idx) const
{
    QString p = tabAt(idx);
    if (p.isEmpty())
        return QStringLiteral("Untitled");
    return QFileInfo(p).fileName();
}
void NextgenTabModel::addTab(const QString& filePath)
{
    // If file already open, just switch
    for (int i = 0; i < m_tabs.size(); ++i)
        if (m_tabs.at(i) == filePath)
        {
            setCurrentIndex(i);
            if (m_splitVisible)
                setPane2Index(i);
            return;
        }
    if (m_tabs.size() == 1 && m_tabs.at(0).isEmpty())
    {
        m_tabs[0] = filePath;
    }
    else
    {
        m_tabs.append(filePath);
        m_tabPanes.append(0); // new tabs go to left pane by default
        m_current = m_tabs.size() - 1;
    }
    emit tabsChanged();
    emit currentIndexChanged();
}

void NextgenTabModel::closeTab(int idx)
{
    if (idx < 0 || idx >= m_tabs.size())
        return;
    m_tabs.removeAt(idx);
    if (m_tabPanes.size() > idx)
        m_tabPanes.removeAt(idx);
    if (m_tabs.isEmpty())
        m_tabs.append(QString());
    if (m_current >= m_tabs.size())
        m_current = m_tabs.size() - 1;
    if (m_pane2Current >= m_tabs.size())
        m_pane2Current = m_tabs.size() - 1;
    emit tabsChanged();
    emit tabClosed(idx);
    emit currentIndexChanged();
    emit pane2IndexChanged();
}

void NextgenTabModel::moveTab(int from, int to)
{
    if (from < 0 || from >= m_tabs.size() || to < 0 || to >= m_tabs.size() || from == to)
        return;
    m_tabs.move(from, to);
    // adjust current indices
    if (m_current == from)
        m_current = to;
    else if (m_current > from && m_current <= to)
        m_current--;
    else if (m_current < from && m_current >= to)
        m_current++;
    emit tabsChanged();
    emit tabMoved(from, to);
    emit currentIndexChanged();
}

void NextgenTabModel::moveTabToPane(int fromPane, int fromIdx, int toPane, int toIdx)
{
    Q_UNUSED(fromPane);
    Q_UNUSED(toPane);
    // For MVP, panes share same tab list; dragging between panes just changes selection
    // fromPane 0 = left, 1 = right; toPane determines which pane's current index to set
    if (fromIdx < 0 || fromIdx >= m_tabs.size() || toIdx < 0 || toIdx > m_tabs.size())
        return;
    // If dragging within same list, reorder
    if (fromPane == toPane)
    {
        moveTab(fromIdx, toIdx);
    }
    else
    {
        // Cross-pane: just set target pane's current to dragged tab
        if (toPane == 0)
            setCurrentIndex(fromIdx);
        else
            setPane2Index(fromIdx);
        // Assign moved tab to target pane
        setTabPane(fromIdx, toPane);
        // Optionally show split
        if (!m_splitVisible)
            setSplitVisible(true);
    }
}

QString NextgenTabModel::currentFile() const
{
    return tabAt(m_current);
}
QString NextgenTabModel::pane2File() const
{
    return tabAt(m_pane2Current);
}
void NextgenTabModel::setCurrentFile(const QString& path)
{
    addTab(path);
}
void NextgenTabModel::detachTab(int idx)
{
    QString path = tabAt(idx);
    if (path.isEmpty())
        return;
    QString appPath = QCoreApplication::applicationFilePath();
    QStringList args;
    args << QStringLiteral("--nextgen") << QStringLiteral("--transfer") << path;
    QProcess::startDetached(appPath, args);
    closeTab(idx);
}

QVariantList NextgenTabModel::tabCursors() const
{
    QVariantList result;
    for (const auto& c : m_tabCursors)
    {
        QVariantList entry;
        for (int v : c) entry.append(v);
        result.append(entry);
    }
    return result;
}

void NextgenTabModel::setTabCursors(const QVariantList& cursors)
{
    m_tabCursors.clear();
    for (const auto& v : cursors)
    {
        QList<int> entry;
        for (const auto& item : v.toList())
            entry.append(item.toInt());
        m_tabCursors.append(entry);
    }
}

void NextgenTabModel::setCursorForTab(int index, int line, int col)
{
    if (index < 0 || index >= m_tabs.size())
        return;
    if (m_tabCursors.size() <= index)
        m_tabCursors.resize(index + 1);
    m_tabCursors[index] = {line, col};
}

QVariantList NextgenTabModel::cursorForTab(int index) const
{
    QVariantList result;
    if (index < 0 || index >= m_tabCursors.size())
    {
        result.append(0);
        result.append(0);
        return result;
    }
    for (int v : m_tabCursors[index])
        result.append(v);
    return result;
}

int NextgenTabModel::tabPane(int index) const
{
    if (index < 0 || index >= m_tabPanes.size())
        return 0;
    return m_tabPanes.at(index);
}

void NextgenTabModel::setTabPane(int index, int pane)
{
    if (index < 0 || index >= m_tabPanes.size())
        return;
    if (m_tabPanes.at(index) == pane)
        return;
    m_tabPanes[index] = pane;
}

QVariantList NextgenTabModel::tabPanes() const
{
    QVariantList result;
    for (int v : m_tabPanes)
        result.append(v);
    return result;
}

void NextgenTabModel::setTabPanes(const QVariantList& panes)
{
    m_tabPanes.clear();
    for (const auto& v : panes)
        m_tabPanes.append(v.toInt());
}
