#include "encodingmenuhelper.h"

EncodingMenuHelper::EncodingMenuHelper(QObject* parent) : QObject(parent)
{
}

QMenu* EncodingMenuHelper::createReopenMenu(QWidget* parentWidget)
{
    m_reopenMenu = new QMenu(tr("Reopen with Encoding"), parentWidget);
    return m_reopenMenu;
}

QMenu* EncodingMenuHelper::createSaveMenu(QWidget* parentWidget)
{
    m_saveMenu = new QMenu(tr("Save with Encoding"), parentWidget);
    return m_saveMenu;
}
