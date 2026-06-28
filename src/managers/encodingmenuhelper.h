#ifndef ENCODINGMENUHELPER_H
#define ENCODINGMENUHELPER_H

#include <QObject>
#include <QMenu>

class EncodingMenuHelper : public QObject {
    Q_OBJECT

public:
    explicit EncodingMenuHelper(QObject *parent = nullptr);

    QMenu* createReopenMenu(QWidget *parentWidget);
    QMenu* createSaveMenu(QWidget *parentWidget);

    QMenu* reopenMenu() const { return m_reopenMenu; }
    QMenu* saveMenu() const { return m_saveMenu; }

private:
    QMenu *m_reopenMenu = nullptr;
    QMenu *m_saveMenu = nullptr;
};

#endif
