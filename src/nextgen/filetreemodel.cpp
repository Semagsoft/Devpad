#include "filetreemodel.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>

FileTreeModel::FileTreeModel(QObject* parent) : QAbstractListModel(parent)
{
}

void FileTreeModel::setRootPath(const QString& path)
{
    if (m_rootPath == path)
        return;
    m_rootPath = path;
    if (m_useGitIgnore && !m_rootPath.isEmpty())
        m_gitIgnore = GitIgnore(m_rootPath);
    else
        m_gitIgnore.clear();
    beginResetModel();
    rebuild();
    endResetModel();
    emit rootPathChanged();
}

void FileTreeModel::setShowHidden(bool v)
{
    if (m_showHidden == v)
        return;
    m_showHidden = v;
    beginResetModel();
    rebuild();
    endResetModel();
    emit showHiddenChanged();
}

void FileTreeModel::setUseGitIgnore(bool v)
{
    if (m_useGitIgnore == v)
        return;
    m_useGitIgnore = v;
    if (m_useGitIgnore && !m_rootPath.isEmpty())
        m_gitIgnore = GitIgnore(m_rootPath);
    else
        m_gitIgnore.clear();
    beginResetModel();
    rebuild();
    endResetModel();
    emit useGitIgnoreChanged();
}

int FileTreeModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_entries.size();
}

QVariant FileTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const Entry& e = m_entries[index.row()];
    switch (role) {
    case FilePathRole: return e.path;
    case FileNameRole: return e.name;
    case IsDirRole: return e.isDir;
    case Qt::DisplayRole: return e.name;
    default: return {};
    }
}

QHash<int, QByteArray> FileTreeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FilePathRole] = "filePath";
    roles[FileNameRole] = "fileName";
    roles[IsDirRole] = "isDir";
    return roles;
}

void FileTreeModel::refresh()
{
    beginResetModel();
    rebuild();
    endResetModel();
}

QString FileTreeModel::filePath(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return {};
    return m_entries[row].path;
}

void FileTreeModel::rebuild()
{
    m_entries.clear();
    if (m_rootPath.isEmpty())
        return;

    QDir dir(m_rootPath);
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    if (!m_showHidden)
        dir.setFilter(dir.filter() | QDir::NoDotAndDotDot); // hidden files excluded by QDir unless we add them
    // Actually QDir::AllEntries includes hidden on some platforms; let's manually filter
    QStringList files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    std::sort(files.begin(), files.end(), [](const QString& a, const QString& b) {
        // directories first, then alphabetical
        QFileInfo fa(a), fb(b);
        if (fa.isDir() && !fb.isDir()) return true;
        if (!fa.isDir() && fb.isDir()) return false;
        return a < b;
    });

    for (const QString& name : files) {
        QString path = dir.absoluteFilePath(name);
        QFileInfo fi(path);

        // Skip hidden if not showing hidden
        if (!m_showHidden && name.startsWith(QLatin1Char('.')))
            continue;

        // Skip gitignored if using gitignore
        if (m_useGitIgnore && m_gitIgnore.isIgnored(path, fi.isDir()))
            continue;

        m_entries.append({path, name, fi.isDir()});
    }
}

bool FileTreeModel::newFolder(const QString& name)
{
    if (m_rootPath.isEmpty() || name.isEmpty())
        return false;
    QString path = m_rootPath + QLatin1Char('/') + name;
    QDir dir(m_rootPath);
    if (!dir.mkdir(name))
        return false;
    refresh();
    return true;
}

bool FileTreeModel::rename(const QString& oldPath, const QString& newPath)
{
    if (oldPath.isEmpty() || newPath.isEmpty())
        return false;
    QFileInfo fi(oldPath);
    if (!fi.exists())
        return false;
    bool ok = QFile::rename(oldPath, newPath);
    if (ok)
        refresh();
    return ok;
}

bool FileTreeModel::remove(const QString& path)
{
    if (path.isEmpty())
        return false;
    QFileInfo fi(path);
    if (!fi.exists())
        return false;
    bool ok = QFile::moveToTrash(path);
    if (!ok && fi.isDir())
        ok = QDir(path).removeRecursively();
    if (!ok && !fi.isDir())
        ok = QFile::remove(path);
    if (ok)
        refresh();
    return ok;
}
