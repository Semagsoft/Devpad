#ifndef FILETREEMODEL_H
#define FILETREEMODEL_H

#include <QAbstractListModel>
#include <QStringList>
#include <QFileInfo>
#include "gitignore.h"

// FileTreeModel: flat list model of files in a directory, gitignore-aware.
// Used by NextGen ProjectTree.qml for file browsing.
class FileTreeModel : public QAbstractListModel
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

    QString rootPath() const { return m_rootPath; }
    void setRootPath(const QString& path);

    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool v);

    bool useGitIgnore() const { return m_useGitIgnore; }
    void setUseGitIgnore(bool v);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString filePath(int row) const;
    Q_INVOKABLE bool newFolder(const QString& name);
    Q_INVOKABLE bool rename(const QString& oldPath, const QString& newPath);
    Q_INVOKABLE bool remove(const QString& path);

signals:
    void rootPathChanged();
    void showHiddenChanged();
    void useGitIgnoreChanged();

private:
    void rebuild();

    QString m_rootPath;
    bool m_showHidden = false;
    bool m_useGitIgnore = true;
    GitIgnore m_gitIgnore;
    struct Entry {
        QString path;
        QString name;
        bool isDir;
    };
    QList<Entry> m_entries;
};

#endif // FILETREEMODEL_H