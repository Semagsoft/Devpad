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
#include "projectpanel.h"

#include "settingsmanager.h"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QProcess>
#include <QStandardPaths>
#include <QToolButton>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

static const QHash<QString, QString>& fileIconMap();

bool ProjectPanel::isWithinRoot(const QString& path) const
{
    if (currentRootPath.isEmpty())
        return false;
    QString canonicalPath = QFileInfo(path).absoluteFilePath();
    QString canonicalRoot = QFileInfo(currentRootPath).absoluteFilePath();
    return canonicalPath.startsWith(canonicalRoot + '/') || canonicalPath == canonicalRoot;
}

FileFilterProxyModel::FileFilterProxyModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    m_closedFolderIcon = QIcon(":/icons/Common/folder.svg");
    m_openFolderIcon = QIcon(":/icons/File/openfolder.svg");
    m_hiddenFolderIcon = QIcon(":/icons/Common/hiddenfolder.svg");
}

void FileFilterProxyModel::setFolderIcons(const QIcon& closed, const QIcon& open)
{
    m_closedFolderIcon = closed;
    m_openFolderIcon = open;
}

void FileFilterProxyModel::setHiddenFolderIcon(const QIcon& icon)
{
    m_hiddenFolderIcon = icon;
}

void FileFilterProxyModel::setExpandedFolders(const QSet<QString>& paths)
{
    m_expandedFolders = paths;
}

void FileFilterProxyModel::notifyDataChanged(const QModelIndex& index)
{
    emit dataChanged(index, index, {Qt::DecorationRole});
}

QVariant FileFilterProxyModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole)
    {
        QModelIndex sourceIndex = mapToSource(index);
        if (sourceModel()->hasChildren(sourceIndex))
        {
            QString path = sourceModel()->data(sourceIndex, QFileSystemModel::FilePathRole).toString();
            if (QFileInfo(path).isHidden())
            {
                return m_hiddenFolderIcon;
            }
            if (m_expandedFolders.contains(path))
            {
                return m_openFolderIcon;
            }
            return m_closedFolderIcon;
        }
        else
        {
            QString filePath = sourceModel()->data(sourceIndex, QFileSystemModel::FilePathRole).toString();
            QFileInfo fileInfo(filePath);
            QString ext = fileInfo.suffix().toLower();

            const auto& iconMap = fileIconMap();
            if (iconMap.contains(ext))
            {
                return QIcon(iconMap.value(ext));
            }
            else if (ext.isEmpty() && fileInfo.isExecutable())
            {
                return QIcon(":/icons/Common/filetypes/executable.svg");
            }

            return m_fileIconProvider.icon(fileInfo);
        }
    }
    return QSortFilterProxyModel::data(index, role);
}

static const QHash<QString, QString>& fileIconMap()
{
    static QHash<QString, QString> map;
    if (map.isEmpty())
    {
        QFile f(QStringLiteral(":/fileicons.json"));
        if (f.open(QIODevice::ReadOnly))
        {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            if (doc.isObject())
            {
                const QJsonObject obj = doc.object();
                for (auto it = obj.begin(); it != obj.end(); ++it)
                {
                    const QString iconPath = it.key();
                    const QJsonArray exts = it.value().toArray();
                    for (const auto& extVal : exts)
                    {
                        map.insert(extVal.toString(), iconPath);
                    }
                }
            }
        }
    }
    return map;
}

QIcon ProjectPanel::iconForFile(const QString& filePath)
{
    QString ext = QFileInfo(filePath).suffix().toLower();
    const auto& map = fileIconMap();
    if (map.contains(ext))
    {
        return QIcon(map.value(ext));
    }
    return QFileIconProvider().icon(QFileInfo(filePath));
}

void FileFilterProxyModel::setFilterText(const QString& text)
{
    m_filterText = text;
    invalidateFilter();
}

void FileFilterProxyModel::setGitIgnoreEnabled(bool enabled)
{
    m_gitIgnoreEnabled = enabled;
    invalidateFilter();
}

void FileFilterProxyModel::setGitIgnoreRootPath(const QString& rootPath)
{
    if (rootPath.isEmpty())
    {
        m_gitIgnore.reset();
    }
    else
    {
        m_gitIgnore = std::make_unique<GitIgnore>(rootPath);
    }
    invalidateFilter();
}

void FileFilterProxyModel::scanGitIgnoreDirectory(const QString& dirPath)
{
    if (m_gitIgnore)
    {
        m_gitIgnore->scanDirectory(dirPath);
        invalidateFilter();
    }
}

bool FileFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QString filePath = sourceModel()->data(index, QFileSystemModel::FilePathRole).toString();
    bool isDir = sourceModel()->hasChildren(index);

    if (m_gitIgnoreEnabled && m_gitIgnore && !m_gitIgnore->isEmpty())
    {
        if (m_gitIgnore->isIgnored(filePath, isDir))
        {
            if (isDir)
            {
                for (int i = 0; i < sourceModel()->rowCount(index); ++i)
                {
                    if (filterAcceptsRow(i, index))
                    {
                        return true;
                    }
                }
            }
            return false;
        }
    }

    if (m_filterText.isEmpty())
    {
        return true;
    }

    QString fileName = sourceModel()->data(index, Qt::DisplayRole).toString();

    if (fileName.contains(m_filterText, Qt::CaseInsensitive))
    {
        return true;
    }

    if (sourceModel()->hasChildren(index))
    {
        for (int i = 0; i < sourceModel()->rowCount(index); ++i)
        {
            if (filterAcceptsRow(i, index))
            {
                return true;
            }
        }
    }

    return false;
}

bool FileFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    bool leftIsDir = sourceModel()->hasChildren(left);
    bool rightIsDir = sourceModel()->hasChildren(right);

    if (leftIsDir != rightIsDir)
    {
        return leftIsDir;
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

ProjectPanel::ProjectPanel(QWidget* parent) : QDockWidget(tr("Project"), parent), currentRootPath(""), currentFilter("")
{
    setObjectName("ProjectPanel");
    setFeatures(QDockWidget::DockWidgetFeature::DockWidgetMovable | QDockWidget::DockWidgetFeature::DockWidgetFloatable);
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    setTitleBarWidget(new QWidget(this));

    setupUI();
}

void ProjectPanel::setupUI()
{
    panelWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(panelWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget* headerWidget = new QWidget(panelWidget);
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(4, 2, 4, 2);
    headerLayout->setSpacing(2);

    titleLabel = new QLabel(tr("No folder opened"), headerWidget);
    titleLabel->setStyleSheet("font-weight: bold; padding: 4px;");

    headerButton = new QToolButton(headerWidget);
    headerButton->setFixedSize(16, 16);
    headerButton->setText(QChar(0x25BC));
    headerButton->setStyleSheet("QToolButton { border: none; background: transparent; font-size: 8px; padding: 0; } QToolButton:hover { "
                                "background-color: rgba(128,128,128,0.2); border-radius: 3px; }");
    headerButton->setCursor(Qt::PointingHandCursor);
    headerButton->setPopupMode(QToolButton::InstantPopup);
    connect(headerButton, &QToolButton::clicked, this, &ProjectPanel::showRecentFoldersMenu);

    headerLayout->addWidget(titleLabel, 1);
    headerLayout->addWidget(headerButton);

    filterEdit = new QLineEdit(panelWidget);
    filterEdit->setPlaceholderText(tr("Filter files..."));
    filterEdit->setClearButtonEnabled(true);
    connect(filterEdit, &QLineEdit::textChanged, this, &ProjectPanel::onFilterTextChanged);

    treeView = new QTreeView(panelWidget);
    treeView->setHeaderHidden(true);
    treeView->setUniformRowHeights(true);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setSortingEnabled(true);
    treeView->sortByColumn(0, Qt::AscendingOrder);
    treeView->setAnimated(true);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    fileModel = new QFileSystemModel(this);
    reloadFilter();

    filterProxyModel = new FileFilterProxyModel(this);
    filterProxyModel->setSourceModel(fileModel);

    treeView->setModel(filterProxyModel);
    for (int i = 1; i < fileModel->columnCount(); ++i)
    {
        treeView->setColumnHidden(i, true);
    }

    connect(treeView, &QTreeView::clicked, this, &ProjectPanel::onItemClicked);
    connect(treeView, &QTreeView::customContextMenuRequested, this, &ProjectPanel::onContextMenu);
    connect(treeView, &QTreeView::expanded, this, &ProjectPanel::onFolderExpanded);
    connect(treeView, &QTreeView::collapsed, this, &ProjectPanel::onFolderCollapsed);

    mainLayout->addWidget(headerWidget);
    mainLayout->addWidget(filterEdit);
    mainLayout->addWidget(treeView);

    setWidget(panelWidget);
}

void ProjectPanel::setRootPath(const QString& path)
{
    if (path.isEmpty())
    {
        clear();
        return;
    }

    currentRootPath = path;
    QModelIndex rootIndex = fileModel->setRootPath(path);
    treeView->setRootIndex(filterProxyModel->mapFromSource(rootIndex));
    treeView->expand(filterProxyModel->mapFromSource(rootIndex));

    QFileInfo fi(path);
    titleLabel->setText(fi.fileName());

    SettingsManager::instance().addRecentFolder(path);

    updateGitIgnore();
    applyFilter();
}

void ProjectPanel::clear()
{
    currentRootPath.clear();
    currentFilter.clear();
    filterEdit->clear();
    fileModel->setRootPath(QString());
    treeView->setRootIndex(QModelIndex());
    titleLabel->setText(tr("No folder opened"));
}

QString ProjectPanel::rootPath() const
{
    return currentRootPath;
}

void ProjectPanel::onItemClicked(const QModelIndex& index)
{
    QModelIndex sourceIndex = filterProxyModel->mapToSource(index);
    QString filePath = fileModel->filePath(sourceIndex);
    QFileInfo fi(filePath);
    if (fi.isFile())
    {
        emit fileDoubleClicked(filePath);
    }
}

void ProjectPanel::onFilterTextChanged(const QString& text)
{
    currentFilter = text;
    applyFilter();
}

void ProjectPanel::applyFilter()
{
    filterProxyModel->setFilterText(currentFilter);
}

void ProjectPanel::reloadFilter()
{
    QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
    if (SettingsManager::instance().showHiddenFiles())
        filters |= QDir::Hidden;
    fileModel->setFilter(filters);
}

void ProjectPanel::updateGitIgnore()
{
    bool useGitIgnore = SettingsManager::instance().useGitIgnore();
    filterProxyModel->setGitIgnoreEnabled(useGitIgnore);
    if (useGitIgnore && !currentRootPath.isEmpty())
    {
        filterProxyModel->setGitIgnoreRootPath(currentRootPath);
    }
    else
    {
        filterProxyModel->setGitIgnoreRootPath(QString());
    }
}

void ProjectPanel::showRecentFoldersMenu()
{
    if (!recentFoldersMenu)
    {
        recentFoldersMenu = new QMenu(this);
    }
    else
    {
        recentFoldersMenu->clear();
    }
    buildRecentFoldersMenu(recentFoldersMenu);
    recentFoldersMenu->exec(headerButton->mapToGlobal(QPoint(0, headerButton->height())));
}

void ProjectPanel::buildRecentFoldersMenu(QMenu* menu)
{
    QStringList folders = SettingsManager::instance().recentFolders();

    if (folders.isEmpty())
    {
        QAction* emptyAct = new QAction(tr("(No recent folders)"), menu);
        emptyAct->setEnabled(false);
        menu->addAction(emptyAct);
        return;
    }

    for (int i = 0; i < folders.size(); ++i)
    {
        QFileInfo fi(folders[i]);
        QString displayText = tr("&%1 %2").arg(i + 1).arg(fi.fileName());
        QAction* act = new QAction(displayText, menu);
        act->setData(folders[i]);
        act->setToolTip(folders[i]);
        connect(act, &QAction::triggered, this, &ProjectPanel::onRecentFolderSelected);
        menu->addAction(act);
    }

    menu->addSeparator();
    QAction* clearAct = new QAction(QIcon(":/icons/Common/clear.svg"), tr("Clear Recent Folders"), menu);
    connect(clearAct, &QAction::triggered, this, &ProjectPanel::onClearRecentFolders);
    menu->addAction(clearAct);
}

void ProjectPanel::onRecentFolderSelected()
{
    QAction* act = qobject_cast<QAction*>(sender());
    if (act)
    {
        QString folderPath = act->data().toString();
        if (!folderPath.isEmpty() && QDir(folderPath).exists())
        {
            emit folderSelected(folderPath);
        }
    }
}

void ProjectPanel::onClearRecentFolders()
{
    SettingsManager::instance().clearRecentFolders();
}

void ProjectPanel::onFolderExpanded(const QModelIndex& index)
{
    QModelIndex sourceIndex = filterProxyModel->mapToSource(index);
    QString path = fileModel->filePath(sourceIndex);
    QSet<QString> expanded = filterProxyModel->expandedFolders();
    expanded.insert(path);
    filterProxyModel->setExpandedFolders(expanded);
    filterProxyModel->notifyDataChanged(index);

    filterProxyModel->scanGitIgnoreDirectory(path);
}

QString ProjectPanel::filePathFromIndex(const QModelIndex& index) const
{
    QModelIndex sourceIndex = filterProxyModel->mapToSource(index);
    return fileModel->filePath(sourceIndex);
}

void ProjectPanel::onContextMenu(const QPoint& pos)
{
    if (currentRootPath.isEmpty())
        return;

    QModelIndex index = treeView->indexAt(pos);
    QMenu menu(this);

    if (index.isValid())
    {
        QString path = filePathFromIndex(index);
        QFileInfo fi(path);

        if (fi.isDir())
        {
            QAction* newAct = menu.addAction(QIcon(":/icons/File/newfile.svg"), tr("New File"));
            connect(newAct, &QAction::triggered, this, [this, path]() { newFile(path); });

            QAction* newFolderAct = menu.addAction(QIcon(":/icons/Common/newfolder.svg"), tr("New Folder"));
            connect(newFolderAct, &QAction::triggered, this, [this, path]() { newFolder(path); });

            menu.addSeparator();

            QAction* renameAct = menu.addAction(QIcon(":/icons/Common/rename.svg"), tr("Rename"));
            connect(renameAct, &QAction::triggered, this, [this, path]() { renameItem(path); });

            QAction* deleteAct = menu.addAction(QIcon(":/icons/Edit/delete.svg"), tr("Delete"));
            connect(deleteAct, &QAction::triggered, this, [this, path]() { deleteItem(path); });

            menu.addSeparator();

            QAction* copyPathAct = menu.addAction(QIcon(":/icons/Edit/copy.svg"), tr("Copy Path"));
            connect(copyPathAct, &QAction::triggered, this, [this, path]() { copyPath(path); });

            QAction* showInFmAct = menu.addAction(QIcon(":/icons/Common/openinfolder.svg"), tr("Show in File Manager"));
            connect(showInFmAct, &QAction::triggered, this, [this, path]() { showInFileManager(path); });

            QAction* openTermAct = menu.addAction(QIcon(":/icons/Common/openinterminal.svg"), tr("Open in Terminal"));
            connect(openTermAct, &QAction::triggered, this, [this, path]() { openInTerminal(path); });
        }
        else
        {
            QAction* openAct = menu.addAction(QIcon(":/icons/File/open.svg"), tr("Open"));
            connect(openAct, &QAction::triggered, this, [this, path]() { openInEditor(path); });

            menu.addSeparator();

            QAction* renameAct = menu.addAction(QIcon(":/icons/Common/rename.svg"), tr("Rename"));
            connect(renameAct, &QAction::triggered, this, [this, path]() { renameItem(path); });

            QAction* deleteAct = menu.addAction(QIcon(":/icons/Edit/delete.svg"), tr("Delete"));
            connect(deleteAct, &QAction::triggered, this, [this, path]() { deleteItem(path); });

            menu.addSeparator();

            QAction* copyPathAct = menu.addAction(QIcon(":/icons/Edit/copy.svg"), tr("Copy Path"));
            connect(copyPathAct, &QAction::triggered, this, [this, path]() { copyPath(path); });

            QAction* showInFmAct = menu.addAction(QIcon(":/icons/Common/openinfolder.svg"), tr("Show in File Manager"));
            connect(showInFmAct, &QAction::triggered, this, [this, path]() { showInFileManager(path); });
        }
    }
    else
    {
        QAction* newAct = menu.addAction(QIcon(":/icons/File/newfile.svg"), tr("New File"));
        connect(newAct, &QAction::triggered, this, [this]() { newFile(currentRootPath); });

        QAction* newFolderAct = menu.addAction(QIcon(":/icons/Common/newfolder.svg"), tr("New Folder"));
        connect(newFolderAct, &QAction::triggered, this, [this]() { newFolder(currentRootPath); });

        menu.addSeparator();

        QAction* copyPathAct = menu.addAction(QIcon(":/icons/Edit/copy.svg"), tr("Copy Root Path"));
        connect(copyPathAct, &QAction::triggered, this, [this]() { copyPath(currentRootPath); });

        QAction* showInFmAct = menu.addAction(tr("Show in File Manager"));
        connect(showInFmAct, &QAction::triggered, this, [this]() { showInFileManager(currentRootPath); });

        QAction* openTermAct = menu.addAction(tr("Open in Terminal"));
        connect(openTermAct, &QAction::triggered, this, [this]() { openInTerminal(currentRootPath); });

        menu.addSeparator();

        bool hiddenVisible = SettingsManager::instance().showHiddenFiles();
        QAction* showHiddenAct = menu.addAction(hiddenVisible ? tr("Hide Hidden Files and Folders") : tr("Show Hidden Files and Folders"));
        connect(showHiddenAct, &QAction::triggered, this,
                [this, hiddenVisible]()
                {
                    SettingsManager::instance().setShowHiddenFiles(!hiddenVisible);
                    reloadFilter();
                });

        bool gitIgnoreEnabled = SettingsManager::instance().useGitIgnore();
        QAction* gitIgnoreAct = menu.addAction(gitIgnoreEnabled ? tr("Disable .gitignore Filtering") : tr("Enable .gitignore Filtering"));
        gitIgnoreAct->setCheckable(true);
        gitIgnoreAct->setChecked(gitIgnoreEnabled);
        connect(gitIgnoreAct, &QAction::triggered, this,
                [this]()
                {
                    bool enabled = !SettingsManager::instance().useGitIgnore();
                    SettingsManager::instance().setUseGitIgnore(enabled);
                    updateGitIgnore();
                });
    }

    menu.exec(treeView->viewport()->mapToGlobal(pos));
}

void ProjectPanel::newFile(const QString& dirPath)
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("New File"), tr("File name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;

    QString filePath = QDir(dirPath).filePath(name.trimmed());
    if (!isWithinRoot(filePath))
    {
        QMessageBox::warning(this, tr("New File"), tr("Invalid path."));
        return;
    }

    QFile file(filePath);
    if (file.exists())
    {
        QMessageBox::warning(this, tr("New File"), tr("File already exists."));
        return;
    }
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, tr("New File"), tr("Failed to create file."));
        return;
    }
    file.close();
}

void ProjectPanel::newFolder(const QString& dirPath)
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("New Folder"), tr("Folder name:"), QLineEdit::Normal, QString(), &ok);
    if (!ok || name.trimmed().isEmpty())
        return;

    QString folderPath = QDir(dirPath).filePath(name.trimmed());
    if (!isWithinRoot(folderPath))
    {
        QMessageBox::warning(this, tr("New Folder"), tr("Invalid path."));
        return;
    }

    QDir dir(dirPath);
    if (!dir.mkdir(name.trimmed()))
    {
        QMessageBox::warning(this, tr("New Folder"), tr("Failed to create folder."));
        return;
    }
}

void ProjectPanel::renameItem(const QString& filePath)
{
    QFileInfo fi(filePath);
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename"), tr("New name:"), QLineEdit::Normal, fi.fileName(), &ok);
    if (!ok || newName.trimmed().isEmpty() || newName.trimmed() == fi.fileName())
        return;

    QString newPath = fi.dir().filePath(newName.trimmed());
    if (!isWithinRoot(newPath))
    {
        QMessageBox::warning(this, tr("Rename"), tr("Invalid path."));
        return;
    }
    if (QFile::exists(newPath))
    {
        QMessageBox::warning(this, tr("Rename"), tr("A file or folder with that name already exists."));
        return;
    }
    QFile file(filePath);
    if (!file.rename(newPath))
    {
        QMessageBox::warning(this, tr("Rename"), tr("Failed to rename."));
        return;
    }
}

void ProjectPanel::deleteItem(const QString& filePath)
{
    if (!isWithinRoot(filePath))
    {
        QMessageBox::warning(this, tr("Delete"), tr("Invalid path."));
        return;
    }

    QFileInfo fi(filePath);
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, tr("Delete"), tr("Are you sure you want to move \"%1\" to the trash?").arg(fi.fileName()), QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    if (!QFile::moveToTrash(filePath))
    {
        QMessageBox::warning(this, tr("Delete"), tr("Failed to move to trash."));
        return;
    }
}

void ProjectPanel::copyPath(const QString& filePath)
{
    QApplication::clipboard()->setText(filePath);
}

void ProjectPanel::openInEditor(const QString& filePath)
{
    emit fileDoubleClicked(filePath);
}

void ProjectPanel::showInFileManager(const QString& filePath)
{
    QFileInfo fi(filePath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(fi.isDir() ? filePath : fi.path()));
}

void ProjectPanel::openInTerminal(const QString& dirPath)
{
    QString terminal = qEnvironmentVariable("TERMINAL");
    if (terminal.isEmpty())
    {
        static const QStringList terminals = {"x-terminal-emulator", "gnome-terminal", "konsole", "xfce4-terminal", "lxterminal", "urxvt", "xterm"};
        for (const QString& t : terminals)
        {
            if (!QStandardPaths::findExecutable(t).isEmpty())
            {
                terminal = t;
                break;
            }
        }
    }
    if (terminal.isEmpty())
    {
        QMessageBox::warning(this, tr("Open in Terminal"), tr("No terminal emulator found."));
        return;
    }

    QStringList termArgs;
    QString terminalBase = QFileInfo(terminal).baseName();
    if (terminalBase == "konsole")
    {
        termArgs << "--workdir" << dirPath;
    }
    else if (terminalBase == "urxvt" || terminalBase == "xterm")
    {
        QString escapedDir = dirPath;
        escapedDir.replace('\'', "'\\''");
        termArgs << "-e" << QString("cd '%1' && exec $SHELL").arg(escapedDir);
    }
    else if (terminalBase == "gnome-terminal" || terminalBase == "kgx" || terminalBase == "xfce4-terminal" || terminalBase == "alacritty" ||
             terminalBase == "kitty" || terminalBase == "ghostty" || terminalBase == "lxterminal" || terminalBase == "foot")
    {
        termArgs << "--working-directory" << dirPath;
    }
    QProcess::startDetached(terminal, termArgs, dirPath);
}

void ProjectPanel::onFolderCollapsed(const QModelIndex& index)
{
    QModelIndex sourceIndex = filterProxyModel->mapToSource(index);
    QString path = fileModel->filePath(sourceIndex);
    QSet<QString> expanded = filterProxyModel->expandedFolders();
    expanded.remove(path);
    filterProxyModel->setExpandedFolders(expanded);
    filterProxyModel->notifyDataChanged(index);
}
