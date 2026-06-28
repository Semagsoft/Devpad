#ifndef RECENTFILESHELPER_H
#define RECENTFILESHELPER_H

#include <QObject>
#include <QAction>
#include <QMenu>

class RecentFilesHelper : public QObject {
    Q_OBJECT

public:
    explicit RecentFilesHelper(QObject *parent = nullptr);

    QMenu* createMenu(QWidget *parentWidget);
    QMenu* menu() const { return m_menu; }
    QAction* fileAction(int index) const;
    QAction* clearAction() const { return m_clearAction; }

signals:
    void openRecentFileTriggered(const QString &filePath);
    void clearRecentFilesTriggered();

private:
    QMenu *m_menu = nullptr;
    QAction *m_fileActions[10] = {};
    QAction *m_clearAction = nullptr;
};

#endif
