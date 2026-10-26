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

#include "gitignore.h"
#include "remotefsentry.h"

#include <QDockWidget>
#include <QFileIconProvider>
#include <QSet>
#include <QSortFilterProxyModel>
#include <memory>

class QFileSystemModel;
class QLabel;
class QLineEdit;
class QMenu;
class QStandardItem;
class QStandardItemModel;
class QToolButton;
class QTreeView;
class QVBoxLayout;

class FileFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit FileFilterProxyModel(QObject* parent = nullptr);
    void setFilterText(const QString& text);
    QVariant data(const QModelIndex& index, int role) const override;

    void setFolderIcons(const QIcon& closed, const QIcon& open);
    void setHiddenFolderIcon(const QIcon& icon);
    void setExpandedFolders(const QSet<QString>& paths);
    const QSet<QString>& expandedFolders() const
    {
        return m_expandedFolders;
    }
    void notifyDataChanged(const QModelIndex& index);

    void setGitIgnoreEnabled(bool enabled);
    void setGitIgnoreRootPath(const QString& rootPath);
    void scanGitIgnoreDirectory(const QString& dirPath);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    void invalidateFilterCompat();
    QString m_filterText;
    QIcon m_closedFolderIcon;
    QIcon m_openFolderIcon;
    QIcon m_hiddenFolderIcon;
    QSet<QString> m_expandedFolders;
    QFileIconProvider m_fileIconProvider;
    std::unique_ptr<GitIgnore> m_gitIgnore;
    bool m_gitIgnoreEnabled = true;
};

class ProjectPanel : public QDockWidget
{
    Q_OBJECT

public:
    explicit ProjectPanel(QWidget* parent = nullptr);

    void setRootPath(const QString& path);
    void clear();
    QString rootPath() const;

    // Remote (SSH) project browsing: the tree is populated lazily — expanding a
    // folder requests a listing via remoteDirExpandRequested(), and
    // applyRemoteListing() feeds the result back.
    void applyRemoteListing(const QString& url, const QString& parentPath, const QList<RemoteFsEntry>& entries);
    void onRemoteListingFailed(const QString& remotePath);
    void clearRemote();
    bool isRemote() const
    {
        return m_remoteMode;
    }

    static QIcon iconForFile(const QString& filePath);

    // Item roles for the remote tree model.
    static constexpr int RemotePathRole = Qt::UserRole + 1;
    static constexpr int RemoteIsDirRole = Qt::UserRole + 2;

signals:
    void fileActivated(const QString& filePath);
    void folderSelected(const QString& folderPath);
    void remoteFileActivated(const QString& url);
    void remoteDirExpandRequested(const QString& url, const QString& remotePath);
    void remoteFolderSelected(const QString& url);

private slots:
    void onItemClicked(const QModelIndex& index);
    void onItemDoubleClicked(const QModelIndex& index);
    void onFilterTextChanged(const QString& text);
    void onContextMenu(const QPoint& pos);
    void onRecentFolderSelected();
    void onClearRecentFolders();
    void showRecentFoldersMenu();
    void onFolderExpanded(const QModelIndex& index);
    void onFolderCollapsed(const QModelIndex& index);

private:
    void setupUI();
    void applyFilter();
    void reloadFilter();
    void buildRecentFoldersMenu(QMenu* menu);
    QString filePathFromIndex(const QModelIndex& index) const;
    bool isWithinRoot(const QString& path) const;
    void newFile(const QString& dirPath);
    void newFolder(const QString& dirPath);
    void renameItem(const QString& filePath);
    void deleteItem(const QString& filePath);
    void copyPath(const QString& filePath);
    void openInEditor(const QString& filePath);
    void showInFileManager(const QString& filePath);
    void openInTerminal(const QString& dirPath);
    void updateGitIgnore();

    void enterRemoteMode(const QString& url, const QString& rootPath);
    void populateRemoteDir(QStandardItem* parentItem, const QString& parentPath, const QList<RemoteFsEntry>& entries);
    void purgeRemoteChildren(QStandardItem* item);
    void requestRemoteDir(const QString& remotePath);
    void activateRemoteItem(const QModelIndex& index);
    void remoteContextMenu(const QPoint& pos);
    void applyRemoteFilter();
    bool remoteFilterTree(const QModelIndex& parent, const QString& text, bool& anyVisible);
    QString remoteUrlFor(const QString& remotePath) const;

    QWidget* panelWidget = nullptr;
    QVBoxLayout* mainLayout = nullptr;
    QLineEdit* filterEdit = nullptr;
    QTreeView* treeView = nullptr;
    QFileSystemModel* fileModel = nullptr;
    FileFilterProxyModel* filterProxyModel = nullptr;
    QStandardItemModel* remoteModel = nullptr;
    QToolButton* headerButton = nullptr;
    QLabel* titleLabel = nullptr;
    QMenu* recentFoldersMenu = nullptr;
    QString currentRootPath;
    QString currentFilter;

    // Remote (SSH) tree state.
    bool m_remoteMode = false;
    QString m_remoteBaseUrl;
    QString m_remoteRootPath;
    QHash<QString, QStandardItem*> m_remoteItemByPath;
    QSet<QString> m_remoteLoadedDirs;
    QSet<QString> m_remotePendingDirs;
};

#endif
