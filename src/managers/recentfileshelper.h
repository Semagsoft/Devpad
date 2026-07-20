#ifndef RECENTFILESHELPER_H
#define RECENTFILESHELPER_H

#include <QAction>
#include <QMenu>
#include <QObject>

#include <array>

class RecentFilesHelper : public QObject
{
    Q_OBJECT

public:
    explicit RecentFilesHelper(QObject* parent = nullptr);

    QMenu* createMenu(QWidget* parentWidget);
    QMenu* menu() const
    {
        return m_menu;
    }
    QAction* fileAction(int index) const;
    QAction* clearAction() const
    {
        return m_clearAction;
    }

signals:
    void openRecentFileTriggered(const QString& filePath);
    void clearRecentFilesTriggered();

private:
    QMenu* m_menu = nullptr;
    std::array<QAction*, 10> m_fileActions = {};
    QAction* m_clearAction = nullptr;
};

#endif
