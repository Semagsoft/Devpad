#include "recentfileshelper.h"

RecentFilesHelper::RecentFilesHelper(QObject *parent)
    : QObject(parent)
{
}

QMenu* RecentFilesHelper::createMenu(QWidget *parentWidget)
{
    m_menu = new QMenu(tr("Recent Files"), parentWidget);
    m_menu->setIcon(QIcon(":/icons/File/recentfiles.svg"));

    for (int i = 0; i < 10; ++i) {
        m_fileActions[i] = new QAction(this);
        m_fileActions[i]->setVisible(false);
        connect(m_fileActions[i], &QAction::triggered, this, [this, i]() {
            if (m_fileActions[i]) {
                emit openRecentFileTriggered(m_fileActions[i]->data().toString());
            }
        });
        m_menu->addAction(m_fileActions[i]);
    }

    m_menu->addSeparator();

    m_clearAction = new QAction(QIcon(":/icons/Common/clear.svg"), tr("Clear Recent Files"), this);
    connect(m_clearAction, &QAction::triggered, this, [this]() {
        emit clearRecentFilesTriggered();
    });
    m_menu->addAction(m_clearAction);

    return m_menu;
}

QAction* RecentFilesHelper::fileAction(int index) const
{
    return (index >= 0 && index < 10) ? m_fileActions[index] : nullptr;
}
