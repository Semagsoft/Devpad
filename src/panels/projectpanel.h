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
#ifndef PROJECTPANEL_H
#define PROJECTPANEL_H

#include <QDockWidget>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolBar>
#include <QToolButton>
#include <QSplitter>
#include <QMenu>
#include <QSet>
#include <QFileIconProvider>
#include <QClipboard>

class FileFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT

public:
    explicit FileFilterProxyModel(QObject *parent = nullptr);
    void setFilterText(const QString &text);
    QVariant data(const QModelIndex &index, int role) const override;

    void setFolderIcons(const QIcon &closed, const QIcon &open);
    void setExpandedFolders(const QSet<QString> &paths);
    const QSet<QString>& expandedFolders() const { return m_expandedFolders; }
    void notifyDataChanged(const QModelIndex &index);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString m_filterText;
    QIcon m_closedFolderIcon;
    QIcon m_openFolderIcon;
    QSet<QString> m_expandedFolders;
    QFileIconProvider m_fileIconProvider;
};

class ProjectPanel : public QDockWidget {
    Q_OBJECT

public:
    explicit ProjectPanel(QWidget *parent = nullptr);

    void setRootPath(const QString &path);
    void clear();
    QString rootPath() const;

    static QIcon iconForFile(const QString &filePath);

signals:
    void fileDoubleClicked(const QString &filePath);
    void folderSelected(const QString &folderPath);

private slots:
    void onItemClicked(const QModelIndex &index);
    void onFilterTextChanged(const QString &text);
    void onContextMenu(const QPoint &pos);
    void onRecentFolderSelected();
    void onClearRecentFolders();
    void showRecentFoldersMenu();
    void onFolderExpanded(const QModelIndex &index);
    void onFolderCollapsed(const QModelIndex &index);

private:
    void setupUI();
    void applyFilter();
    void buildRecentFoldersMenu(QMenu *menu);
    QString filePathFromIndex(const QModelIndex &index) const;
    bool isWithinRoot(const QString &path) const;
    void newFile(const QString &dirPath);
    void newFolder(const QString &dirPath);
    void renameItem(const QString &filePath);
    void deleteItem(const QString &filePath);
    void copyPath(const QString &filePath);
    void openInEditor(const QString &filePath);
    void showInFileManager(const QString &filePath);
    void openInTerminal(const QString &dirPath);

    QWidget *panelWidget;
    QVBoxLayout *mainLayout;
    QLineEdit *filterEdit;
    QTreeView *treeView;
    QFileSystemModel *fileModel;
    FileFilterProxyModel *filterProxyModel;
    QToolButton *headerButton;
    QLabel *titleLabel;
    QMenu *recentFoldersMenu = nullptr;
    QString currentRootPath;
    QString currentFilter;
};

#endif
