#ifndef FILETREEMODEL_H
#define FILETREEMODEL_H

#include <QAbstractItemModel>
#include <QStringList>
#include <QFileInfo>
#include "gitignore.h"

// FileTreeModel: tree model of files in a directory, gitignore-aware.
// Used by NextGen ProjectTree.qml for file browsing (lazy-loaded).
class FileTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)
    Q_PROPERTY(bool useGitIgnore READ useGitIgnore WRITE setUseGitIgnore NOTIFY useGitIgnoreChanged)

public:
    enum Roles {
        FilePathRole = Qt::UserRole + 1,
        FileNameRole,
        IsDirRole,
    };
    Q_ENUM(Roles)

    explicit FileTreeModel(QObject* parent = nullptr);
    ~FileTreeModel() override;

    QString rootPath() const { return m_rootPath; }
    void setRootPath(const QString& path);

    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool v);

    bool useGitIgnore() const { return m_useGitIgnore; }
    void setUseGitIgnore(bool v);

    // QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool canFetchMore(const QModelIndex& parent) const override;
    void fetchMore(const QModelIndex& parent) override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString filePath(const QModelIndex& idx) const;
    Q_INVOKABLE QString fileName(const QModelIndex& idx) const;
    Q_INVOKABLE bool isDir(const QModelIndex& idx) const;
    Q_INVOKABLE bool newFolder(const QString& name, const QModelIndex& parent = QModelIndex());
    Q_INVOKABLE bool newFile(const QString& name, const QModelIndex& parent = QModelIndex());
    Q_INVOKABLE bool rename(const QModelIndex& idx, const QString& newName);
    Q_INVOKABLE bool renamePath(const QString& oldPath, const QString& newPath);
    Q_INVOKABLE bool remove(const QModelIndex& idx);

signals:
    void rootPathChanged();
    void showHiddenChanged();
    void useGitIgnoreChanged();

private:
    struct Node {
        QString path;
        QString name;
        bool isDir = false;
        bool loaded = false;
        Node* parent = nullptr;
        QList<Node*> children;
    };
    Node* nodeFor(const QModelIndex& idx) const;
    void deleteTree(Node* n);
    void loadChildren(Node* n);
    Node* m_root = nullptr;
    QString m_rootPath;
    bool m_showHidden = false;
    bool m_useGitIgnore = true;
    GitIgnore m_gitIgnore;
};

#endif // FILETREEMODEL_H