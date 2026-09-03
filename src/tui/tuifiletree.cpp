/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 */

#include "tuifiletree.h"

#include <QDir>
#include <QFileInfo>

TuiFileTree::TuiFileTree() = default;

void TuiFileTree::setRootPath(const QString& path)
{
    QString clean = QDir::cleanPath(path);
    if (clean == m_rootPath)
        return;
    m_rootPath = clean;
    m_expanded.clear();
    m_cursor = 0;
    m_dirty = true;
    m_gitIgnore.clear();
    if (!m_rootPath.isEmpty() && QDir(m_rootPath).exists())
    {
        m_gitIgnore.setRootPath(m_rootPath);
        m_gitIgnore.scanDirectory(m_rootPath);
        // Auto-expand root
        m_expanded.insert(m_rootPath);
    }
}

void TuiFileTree::setShowHidden(bool show)
{
    if (m_showHidden == show)
        return;
    m_showHidden = show;
    m_dirty = true;
}

void TuiFileTree::setFilter(const QString& filter)
{
    if (m_filter == filter)
        return;
    m_filter = filter;
    m_dirty = true;
}

void TuiFileTree::setExpanded(const QString& path, bool expanded)
{
    QString clean = QDir::cleanPath(path);
    if (expanded)
        m_expanded.insert(clean);
    else
        m_expanded.remove(clean);
    m_dirty = true;
}

bool TuiFileTree::isExpanded(const QString& path) const
{
    return m_expanded.contains(QDir::cleanPath(path));
}

void TuiFileTree::toggleExpanded(const QString& path)
{
    QString clean = QDir::cleanPath(path);
    if (m_expanded.contains(clean))
        m_expanded.remove(clean);
    else
        m_expanded.insert(clean);
    m_dirty = true;
}

void TuiFileTree::refresh()
{
    if (!m_rootPath.isEmpty())
    {
        m_gitIgnore.scanDirectory(m_rootPath);
    }
    m_dirty = true;
}

QStringList TuiFileTree::listEntries(const QString& dirPath) const
{
    QDir dir(dirPath);
    QFileInfoList infos = dir.entryInfoList(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

    QStringList out;
    for (const QFileInfo& fi : infos)
    {
        QString abs = fi.absoluteFilePath();
        bool isDir = fi.isDir();
        // Gitignore filtering
        if (!m_gitIgnore.isEmpty() && m_gitIgnore.isIgnored(abs, isDir))
            continue;
        // Name filter
        if (!m_filter.isEmpty() && !fi.fileName().contains(m_filter, Qt::CaseInsensitive))
        {
            // For dirs, keep if any descendant matches filter (rough: keep dir to allow children)
            // To keep tree navigable, we keep dirs even if filter doesn't match name, if filter present we will scan anyway and child may match.
            // So for now, if isDir, don't filter out; children filtering will happen recursively.
            if (!isDir)
                continue;
        }
        // Hide . and .. and hidden files when m_showHidden is false
        if (!m_showHidden && fi.fileName().startsWith('.'))
            continue;
        out.append(abs);
    }
    return out;
}

void TuiFileTree::buildVisible(QList<TuiFileNode>& out, const QString& dirPath, int depth) const
{
    QStringList entries = listEntries(dirPath);
    for (const QString& abs : entries)
    {
        QFileInfo fi(abs);
        TuiFileNode node;
        node.name = fi.fileName();
        node.absolutePath = abs;
        node.relativePath = QDir(m_rootPath).relativeFilePath(abs);
        node.isDir = fi.isDir();
        node.depth = depth;
        node.expanded = node.isDir && isExpanded(abs);
        // When filter active, dirs that don't match themselves but have no visible children should be hidden
        // We do recursive check: if filter present and isDir, we peek children
        bool shouldAdd = true;
        if (!m_filter.isEmpty() && !node.isDir)
        {
            // Already filtered above
            shouldAdd = true;
        }
        else if (!m_filter.isEmpty() && node.isDir)
        {
            // Check if this dir or any descendant would be visible; if filter set and dir name doesn't contain filter, we still want to show if
            // descendants match. Build temp child list to see if any descendant visible
            QList<TuiFileNode> temp;
            buildVisible(temp, abs, depth + 1);
            shouldAdd = !temp.isEmpty() || fi.fileName().contains(m_filter, Qt::CaseInsensitive);
        }

        if (!shouldAdd)
            continue;

        out.append(node);
        if (node.isDir && node.expanded)
        {
            buildVisible(out, abs, depth + 1);
        }
    }
}

QList<TuiFileNode> TuiFileTree::visibleNodes() const
{
    if (!m_dirty && !m_cachedVisible.isEmpty())
        return m_cachedVisible;

    m_cachedVisible.clear();
    if (hasRoot())
    {
        buildVisible(m_cachedVisible, m_rootPath, 0);
    }
    // Clamp cursor
    if (m_cursor >= m_cachedVisible.size())
        const_cast<TuiFileTree*>(this)->m_cursor = qMax(0, m_cachedVisible.size() - 1);
    if (m_cursor < 0)
        const_cast<TuiFileTree*>(this)->m_cursor = 0;
    m_dirty = false;
    return m_cachedVisible;
}

void TuiFileTree::setCursor(int idx)
{
    auto nodes = visibleNodes();
    if (nodes.isEmpty())
    {
        m_cursor = 0;
        return;
    }
    m_cursor = qBound(0, idx, nodes.size() - 1);
}

void TuiFileTree::moveCursor(int delta)
{
    setCursor(m_cursor + delta);
}

TuiFileNode* TuiFileTree::nodeAt(int idx)
{
    auto nodes = visibleNodes();
    if (idx < 0 || idx >= nodes.size())
        return nullptr;
    // Return pointer to cached visible - need mutable
    // We return a copy via static? Simplify: return nullptr and use value API
    // For now, we provide value access via currentNode; direct pointer not needed for mutable toggle
    // To satisfy API, we return pointer to cached element address (unsafe if reallocated but okay for immediate use)
    return &m_cachedVisible[idx];
}

const TuiFileNode* TuiFileTree::nodeAt(int idx) const
{
    auto nodes = visibleNodes();
    if (idx < 0 || idx >= nodes.size())
        return nullptr;
    return &m_cachedVisible[idx];
}

TuiFileNode* TuiFileTree::currentNode()
{
    return nodeAt(m_cursor);
}

const TuiFileNode* TuiFileTree::currentNode() const
{
    return nodeAt(m_cursor);
}
