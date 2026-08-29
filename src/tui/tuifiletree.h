/*
 * Devpad - A C++/Qt6 code editor
 * Copyright (C) 2026 Semagsoft
 *
 * TUI file tree model - headless counterpart to ProjectPanel.
 */

#ifndef TUIFILETREE_H
#define TUIFILETREE_H

#include "gitignore.h"

#include <QDir>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

struct TuiFileNode
{
    QString name;
    QString absolutePath;
    QString relativePath;
    bool isDir = false;
    int depth = 0;
    bool expanded = false;
};

class TuiFileTree
{
public:
    explicit TuiFileTree();

    void setRootPath(const QString& path);
    QString rootPath() const { return m_rootPath; }
    bool hasRoot() const { return !m_rootPath.isEmpty() && QDir(m_rootPath).exists(); }

    void setShowHidden(bool show);
    bool showHidden() const { return m_showHidden; }

    void setFilter(const QString& filter);
    QString filter() const { return m_filter; }

    void setExpanded(const QString& path, bool expanded);
    bool isExpanded(const QString& path) const;
    void toggleExpanded(const QString& path);

    void refresh();

    // Visible nodes in order, considering expand state and filter
    QList<TuiFileNode> visibleNodes() const;

    int cursorIndex() const { return m_cursor; }
    void setCursor(int idx);
    void moveCursor(int delta);
    TuiFileNode* nodeAt(int idx);
    const TuiFileNode* nodeAt(int idx) const;
    TuiFileNode* currentNode();
    const TuiFileNode* currentNode() const;

private:
    void buildVisible(QList<TuiFileNode>& out, const QString& dirPath, int depth) const;
    bool isVisible(const TuiFileNode& node) const;
    QStringList listEntries(const QString& dirPath) const;

    QString m_rootPath;
    bool m_showHidden = false;
    QString m_filter;
    QSet<QString> m_expanded;
    GitIgnore m_gitIgnore;
    mutable QList<TuiFileNode> m_cachedVisible;
    mutable bool m_dirty = true;
    int m_cursor = 0;
};

#endif // TUIFILETREE_H
