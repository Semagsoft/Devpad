#ifndef ENCODINGMENUHELPER_H
#define ENCODINGMENUHELPER_H

#include <QMenu>
#include <QObject>

class EncodingMenuHelper : public QObject
{
    Q_OBJECT

public:
    explicit EncodingMenuHelper(QObject* parent = nullptr);

    QMenu* createReopenMenu(QWidget* parentWidget);
    QMenu* createSaveMenu(QWidget* parentWidget);

    [[nodiscard]] QMenu* reopenMenu() const
    {
        return m_reopenMenu;
    }
    [[nodiscard]] QMenu* saveMenu() const
    {
        return m_saveMenu;
    }

private:
    QMenu* m_reopenMenu = nullptr;
    QMenu* m_saveMenu = nullptr;
};

#endif
