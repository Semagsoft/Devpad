#include "filetreemodel.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QFile>

FileTreeModel::FileTreeModel(QObject* parent) : QAbstractItemModel(parent)
{
    m_root = new FileTreeModel::Node();
    m_root->isDir = true;
    m_root->loaded = true;
}

FileTreeModel::~FileTreeModel()
{
    deleteTree(m_root);
}

void FileTreeModel::deleteTree(FileTreeModel::Node* n)
{
    if (!n) return;
    for (FileTreeModel::Node* c : n->children) deleteTree(c);
    delete n;
}

FileTreeModel::Node* FileTreeModel::nodeFor(const QModelIndex& idx) const
{
    if (!idx.isValid()) return m_root;
    return static_cast<FileTreeModel::Node*>(idx.internalPointer());
}

void FileTreeModel::setRootPath(const QString& path)
{
    if (m_rootPath == path) return;
    m_rootPath = path;
    if (m_useGitIgnore && !m_rootPath.isEmpty())
        m_gitIgnore = GitIgnore(m_rootPath);
    else
        m_gitIgnore.clear();
    beginResetModel();
    deleteTree(m_root);
    m_root = new FileTreeModel::Node();
    m_root->isDir = true;
    m_root->loaded = false;
    m_root->path = m_rootPath;
    m_root->name = QFileInfo(m_rootPath).fileName();
    loadChildren(m_root);
    m_root->loaded = true;
    endResetModel();
    emit rootPathChanged();
}

void FileTreeModel::setShowHidden(bool v)
{
    if (m_showHidden == v) return;
    m_showHidden = v;
    refresh();
    emit showHiddenChanged();
}

void FileTreeModel::setUseGitIgnore(bool v)
{
    if (m_useGitIgnore == v) return;
    m_useGitIgnore = v;
    if (m_useGitIgnore && !m_rootPath.isEmpty())
        m_gitIgnore = GitIgnore(m_rootPath);
    else
        m_gitIgnore.clear();
    refresh();
    emit useGitIgnoreChanged();
}

void FileTreeModel::loadChildren(FileTreeModel::Node* n)
{
    if (!n || n->loaded) return;
    n->loaded = true;
    QDir dir(n->path.isEmpty() ? m_rootPath : n->path);
    if (n->path.isEmpty()) dir = QDir(m_rootPath);
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    QStringList files = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    std::sort(files.begin(), files.end(), [](const QString& a, const QString& b) {
        QFileInfo fa(a), fb(b);
        if (fa.isDir() && !fb.isDir()) return true;
        if (!fa.isDir() && fb.isDir()) return false;
        return a < b;
    });
    for (const QString& name : files) {
        if (!m_showHidden && name.startsWith(QLatin1Char('.'))) continue;
        QString path = dir.absoluteFilePath(name);
        QFileInfo fi(path);
        if (m_useGitIgnore && m_gitIgnore.isIgnored(path, fi.isDir())) continue;
        FileTreeModel::Node* c = new FileTreeModel::Node();
        c->path = path;
        c->name = name;
        c->isDir = fi.isDir();
        c->parent = n;
        c->loaded = false;
        n->children.append(c);
    }
}

QModelIndex FileTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0) return QModelIndex();
    FileTreeModel::Node* p = nodeFor(parent);
    if (!p || row < 0 || row >= p->children.size()) return QModelIndex();
    return createIndex(row, column, p->children.at(row));
}

QModelIndex FileTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) return QModelIndex();
    FileTreeModel::Node* n = nodeFor(index);
    if (!n || !n->parent || n->parent == m_root) return QModelIndex();
    FileTreeModel::Node* p = n->parent;
    if (!p->parent) return QModelIndex();
    int row = p->parent->children.indexOf(p);
    return createIndex(row, 0, p);
}

int FileTreeModel::rowCount(const QModelIndex& parent) const
{
    FileTreeModel::Node* n = nodeFor(parent);
    if (!n) return 0;
    if (!n->loaded && n != m_root) return 0; // not fetched yet
    if (n == m_root && !n->loaded) {
        // root children not loaded; report 0 until fetchMore
    }
    return n->children.size();
}

int FileTreeModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 1;
}

QVariant FileTreeModel::data(const QModelIndex& index, int role) const
{
    FileTreeModel::Node* n = nodeFor(index);
    if (!n || n == m_root) return {};
    switch (role) {
    case FilePathRole: return n->path;
    case FileNameRole: return n->name;
    case IsDirRole: return n->isDir;
    case Qt::DisplayRole: return n->name;
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

bool FileTreeModel::canFetchMore(const QModelIndex& parent) const
{
    FileTreeModel::Node* n = nodeFor(parent);
    if (!n) return false;
    if (n == m_root) return !n->loaded;
    return n->isDir && !n->loaded;
}

void FileTreeModel::fetchMore(const QModelIndex& parent)
{
    FileTreeModel::Node* n = nodeFor(parent);
    if (!n || n->loaded) return;
    if (n == m_root) {
        n->path = m_rootPath;
        n->name = QFileInfo(m_rootPath).fileName();
    }
    int before = n->children.size();
    loadChildren(n);
    int after = n->children.size();
    if (after > before) {
        beginInsertRows(parent, before, after - 1);
        endInsertRows();
    }
}

void FileTreeModel::refresh()
{
    beginResetModel();
    deleteTree(m_root);
    m_root = new FileTreeModel::Node();
    m_root->isDir = true;
    m_root->loaded = false;
    m_root->path = m_rootPath;
    m_root->name = QFileInfo(m_rootPath).fileName();
    loadChildren(m_root);
    m_root->loaded = true;
    endResetModel();
}

QString FileTreeModel::filePath(const QModelIndex& idx) const
{
    FileTreeModel::Node* n = nodeFor(idx);
    if (!n || n == m_root) return {};
    return n->path;
}

QString FileTreeModel::fileName(const QModelIndex& idx) const
{
    FileTreeModel::Node* n = nodeFor(idx);
    if (!n || n == m_root) return {};
    return n->name;
}

bool FileTreeModel::isDir(const QModelIndex& idx) const
{
    FileTreeModel::Node* n = nodeFor(idx);
    if (!n || n == m_root) return false;
    return n->isDir;
}

bool FileTreeModel::newFolder(const QString& name, const QModelIndex& parent)
{
    FileTreeModel::Node* p = nodeFor(parent);
    if (!p) p = m_root;
    QString basePath = (p == m_root) ? m_rootPath : p->path;
    if (basePath.isEmpty() || name.isEmpty()) return false;
    QString path = basePath + QLatin1Char('/') + name;
    QDir dir(basePath);
    if (!dir.mkdir(name)) return false;
    // insert child
    FileTreeModel::Node* c = new FileTreeModel::Node();
    c->path = path;
    c->name = name;
    c->isDir = true;
    c->parent = p;
    int row = p->children.size();
    beginInsertRows(parent, row, row);
    p->children.append(c);
    endInsertRows();
    return true;
}

bool FileTreeModel::newFile(const QString& name, const QModelIndex& parent)
{
    FileTreeModel::Node* p = nodeFor(parent);
    if (!p) p = m_root;
    QString basePath = (p == m_root) ? m_rootPath : p->path;
    if (basePath.isEmpty() || name.isEmpty()) return false;
    QString path = basePath + QLatin1Char('/') + name;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.close();
    FileTreeModel::Node* c = new FileTreeModel::Node();
    c->path = path;
    c->name = name;
    c->isDir = false;
    c->parent = p;
    c->loaded = true;
    int row = p->children.size();
    beginInsertRows(parent, row, row);
    p->children.append(c);
    endInsertRows();
    return true;
}

bool FileTreeModel::rename(const QModelIndex& idx, const QString& newName)
{
    FileTreeModel::Node* n = nodeFor(idx);
    if (!n || n == m_root || newName.isEmpty()) return false;
    QFileInfo fi(n->path);
    if (!fi.exists()) return false;
    QString dir = n->path.left(n->path.lastIndexOf('/') + 1);
    QString newPath = dir + newName;
    bool ok = QFile::rename(n->path, newPath);
    if (ok) {
        n->path = newPath;
        n->name = newName;
        emit dataChanged(idx, idx, {FileNameRole, FilePathRole, Qt::DisplayRole});
    }
    return ok;
}

bool FileTreeModel::renamePath(const QString& oldPath, const QString& newPath)
{
    QFileInfo fi(oldPath);
    if (oldPath.isEmpty() || newPath.isEmpty() || !fi.exists()) return false;
    bool ok = QFile::rename(oldPath, newPath);
    if (ok) refresh();
    return ok;
}

bool FileTreeModel::remove(const QModelIndex& idx)
{
    FileTreeModel::Node* n = nodeFor(idx);
    if (!n || n == m_root) return false;
    QFileInfo fi(n->path);
    if (!fi.exists()) return false;
    bool ok = QFile::moveToTrash(n->path);
    if (!ok && fi.isDir()) ok = QDir(n->path).removeRecursively();
    if (!ok && !fi.isDir()) ok = QFile::remove(n->path);
    if (ok) {
        FileTreeModel::Node* p = n->parent;
        int row = p->children.indexOf(n);
        beginRemoveRows(parent(idx), row, row);
        p->children.removeAt(row);
        delete n;
        endRemoveRows();
    }
    return ok;
}